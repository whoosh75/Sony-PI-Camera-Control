#include "camera_controller.hpp"
#include "CRSDK/CrDeviceProperty.h"
#include <chrono>
#include <thread>
#include <cstdio>
#include <arpa/inet.h>
#include <cstring>

CameraController::CameraController(const std::string& name) 
    : camera_info(nullptr), device_handle(0), is_connected(false), camera_name(name) {
}

CameraController::~CameraController() {
    disconnect();
}

bool CameraController::connectViaIP(const std::string& ip, const std::string& mac, 
                                   SCRSDK::CrCameraDeviceModelList model) {
    std::printf("[%s] Connecting via IP: %s\n", camera_name.c_str(), ip.c_str());
    
    // Parse IP address
    in_addr ina;
    if (inet_pton(AF_INET, ip.c_str(), &ina) != 1) {
        last_error = "Invalid IP address: " + ip;
        return false;
    }
    CrInt32u ipAddr = (CrInt32u)ina.s_addr;
    
    // Parse MAC address
    CrInt8u macBuf[6] = {0};
    if (!mac.empty()) {
        unsigned int ma[6] = {0};
        if (std::sscanf(mac.c_str(), "%x:%x:%x:%x:%x:%x", 
                       &ma[0], &ma[1], &ma[2], &ma[3], &ma[4], &ma[5]) == 6) {
            for (int i = 0; i < 6; ++i) {
                macBuf[i] = (CrInt8u)(ma[i] & 0xFF);
            }
        }
    }
    
    // Create camera object using our proven method
    auto err = SCRSDK::CreateCameraObjectInfoEthernetConnection(&camera_info, model, 
                                                               ipAddr, macBuf, 1);
    if (CR_FAILED(err) || !camera_info) {
        last_error = "CreateCameraObjectInfoEthernetConnection failed: " + std::to_string(err);
        return false;
    }
    
    // Connect to camera
    err = SCRSDK::Connect(camera_info, nullptr, &device_handle, 
                         SCRSDK::CrSdkControlMode_Remote, 
                         SCRSDK::CrReconnecting_ON);
    if (CR_FAILED(err)) {
        last_error = "Camera connection failed: " + std::to_string(err);
        camera_info->Release();
        camera_info = nullptr;
        return false;
    }
    
    is_connected = true;
    std::printf("[%s] ✅ Connected successfully! Device handle: %u\n", camera_name.c_str(), device_handle);
    return true;
}

bool CameraController::startRecording() {
    if (!is_connected) {
        last_error = "Camera not connected";
        return false;
    }
    
    std::printf("[%s] 🔴 Starting recording...\n", camera_name.c_str());
    
    // Send record start command
    auto err = SCRSDK::SendCommand(device_handle, 
                                  SCRSDK::CrCommandId::CrCommandId_MovieRecord, 
                                  SCRSDK::CrCommandParam::CrCommandParam_Down);
    
    if (CR_FAILED(err)) {
        last_error = "Record start failed: " + std::to_string(err);
        std::printf("[%s] ❌ Record start failed: 0x%08X\n", camera_name.c_str(), err);
        return false;
    }
    
    std::printf("[%s] ✅ Recording started\n", camera_name.c_str());
    return true;
}

bool CameraController::stopRecording() {
    if (!is_connected) {
        last_error = "Camera not connected";
        return false;
    }
    
    std::printf("[%s] ⏹️  Stopping recording...\n", camera_name.c_str());
    
    // Send record stop command  
    auto err = SCRSDK::SendCommand(device_handle,
                                  SCRSDK::CrCommandId::CrCommandId_MovieRecord,
                                  SCRSDK::CrCommandParam::CrCommandParam_Up);
    
    if (CR_FAILED(err)) {
        last_error = "Record stop failed: " + std::to_string(err);
        std::printf("[%s] ❌ Record stop failed: 0x%08X\n", camera_name.c_str(), err);
        return false;
    }
    
    std::printf("[%s] ✅ Recording stopped\n", camera_name.c_str());
    return true;
}

bool CameraController::capturePhoto() {
    if (!is_connected) {
        last_error = "Camera not connected";
        return false;
    }
    
    std::printf("[%s] 📸 Capturing photo...\n", camera_name.c_str());
    
    // Send shutter press (half press first, then full press)
    auto err1 = SCRSDK::SendCommand(device_handle,
                                   SCRSDK::CrCommandId::CrCommandId_Release,
                                   SCRSDK::CrCommandParam::CrCommandParam_HalfPress);
    
    std::this_thread::sleep_for(std::chrono::milliseconds(500)); // Focus time
    
    auto err2 = SCRSDK::SendCommand(device_handle,
                                   SCRSDK::CrCommandId::CrCommandId_Release,
                                   SCRSDK::CrCommandParam::CrCommandParam_Down);
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    auto err3 = SCRSDK::SendCommand(device_handle,
                                   SCRSDK::CrCommandId::CrCommandId_Release,
                                   SCRSDK::CrCommandParam::CrCommandParam_Up);
    
    if (CR_FAILED(err1) || CR_FAILED(err2) || CR_FAILED(err3)) {
        last_error = "Photo capture failed";
        return false;
    }
    
    std::printf("[%s] ✅ Photo captured\n", camera_name.c_str());
    return true;
}

bool CameraController::setISO(int iso_value) {
    if (!is_connected) {
        last_error = "Camera not connected";
        return false;
    }
    
    std::printf("[%s] 🎚️  Setting ISO to %d...\n", camera_name.c_str(), iso_value);
    
    CrInt32u iso_mapped = mapISOValue(iso_value);
    if (iso_mapped == 0) {
        last_error = "Invalid ISO value: " + std::to_string(iso_value);
        return false;
    }
    
    auto err = SCRSDK::SetDeviceProperty(device_handle, 
                                        SCRSDK::CrDeviceProperty::CrDeviceProperty_ISO,
                                        &iso_mapped, sizeof(iso_mapped));
    
    if (CR_FAILED(err)) {
        last_error = "ISO setting failed: " + std::to_string(err);
        std::printf("[%s] ❌ ISO setting failed: 0x%08X\n", camera_name.c_str(), err);
        return false;
    }
    
    std::printf("[%s] ✅ ISO set to %d\n", camera_name.c_str(), iso_value);
    return true;
}

bool CameraController::setWhiteBalance(const std::string& wb) {
    if (!is_connected) {
        last_error = "Camera not connected";
        return false;
    }
    
    std::printf("[%s] ⚪ Setting white balance to %s...\n", camera_name.c_str(), wb.c_str());
    
    CrInt32u wb_mapped = mapWhiteBalance(wb);
    if (wb_mapped == 0) {
        last_error = "Invalid white balance: " + wb;
        return false;
    }
    
    auto err = SCRSDK::SetDeviceProperty(device_handle,
                                        SCRSDK::CrDeviceProperty::CrDeviceProperty_WhiteBalance,
                                        &wb_mapped, sizeof(wb_mapped));
    
    if (CR_FAILED(err)) {
        last_error = "White balance setting failed: " + std::to_string(err);
        std::printf("[%s] ❌ White balance setting failed: 0x%08X\n", camera_name.c_str(), err);
        return false;
    }
    
    std::printf("[%s] ✅ White balance set to %s\n", camera_name.c_str(), wb.c_str());
    return true;
}

bool CameraController::setFrameRate(const std::string& fps) {
    if (!is_connected) {
        last_error = "Camera not connected";
        return false;
    }
    
    std::printf("[%s] 🎬 Setting frame rate to %s...\n", camera_name.c_str(), fps.c_str());
    
    // Frame rate is often part of recording settings
    CrInt32u fps_mapped = mapFrameRate(fps);
    if (fps_mapped == 0) {
        last_error = "Invalid frame rate: " + fps;
        return false;
    }
    
    auto err = SCRSDK::SetDeviceProperty(device_handle,
                                        SCRSDK::CrDeviceProperty::CrDeviceProperty_RecordingSetting,
                                        &fps_mapped, sizeof(fps_mapped));
    
    if (CR_FAILED(err)) {
        last_error = "Frame rate setting failed: " + std::to_string(err);
        std::printf("[%s] ❌ Frame rate setting failed: 0x%08X\n", camera_name.c_str(), err);
        return false;
    }
    
    std::printf("[%s] ✅ Frame rate set to %s\n", camera_name.c_str(), fps.c_str());
    return true;
}

// Mapping functions for Sony CRSDK values
CrInt32u CameraController::mapISOValue(int iso) {
    // Common ISO values mapping
    switch (iso) {
        case 50:   return SCRSDK::CrISO::CrISO_50;
        case 64:   return SCRSDK::CrISO::CrISO_64;
        case 80:   return SCRSDK::CrISO::CrISO_80;
        case 100:  return SCRSDK::CrISO::CrISO_100;
        case 125:  return SCRSDK::CrISO::CrISO_125;
        case 160:  return SCRSDK::CrISO::CrISO_160;
        case 200:  return SCRSDK::CrISO::CrISO_200;
        case 250:  return SCRSDK::CrISO::CrISO_250;
        case 320:  return SCRSDK::CrISO::CrISO_320;
        case 400:  return SCRSDK::CrISO::CrISO_400;
        case 500:  return SCRSDK::CrISO::CrISO_500;
        case 640:  return SCRSDK::CrISO::CrISO_640;
        case 800:  return SCRSDK::CrISO::CrISO_800;
        case 1000: return SCRSDK::CrISO::CrISO_1000;
        case 1250: return SCRSDK::CrISO::CrISO_1250;
        case 1600: return SCRSDK::CrISO::CrISO_1600;
        case 2000: return SCRSDK::CrISO::CrISO_2000;
        case 2500: return SCRSDK::CrISO::CrISO_2500;
        case 3200: return SCRSDK::CrISO::CrISO_3200;
        case 4000: return SCRSDK::CrISO::CrISO_4000;
        case 5000: return SCRSDK::CrISO::CrISO_5000;
        case 6400: return SCRSDK::CrISO::CrISO_6400;
        case 8000: return SCRSDK::CrISO::CrISO_8000;
        case 10000: return SCRSDK::CrISO::CrISO_10000;
        case 12800: return SCRSDK::CrISO::CrISO_12800;
        case 16000: return SCRSDK::CrISO::CrISO_16000;
        case 20000: return SCRSDK::CrISO::CrISO_20000;
        case 25600: return SCRSDK::CrISO::CrISO_25600;
        default: return 0;
    }
}

CrInt32u CameraController::mapWhiteBalance(const std::string& wb) {
    if (wb == "Auto" || wb == "AWB") return SCRSDK::CrWhiteBalanceSet::CrWhiteBalance_AWB;
    if (wb == "Daylight" || wb == "5600K") return SCRSDK::CrWhiteBalanceSet::CrWhiteBalance_Daylight;
    if (wb == "Shade" || wb == "7000K") return SCRSDK::CrWhiteBalanceSet::CrWhiteBalance_Shade;
    if (wb == "Cloudy" || wb == "6000K") return SCRSDK::CrWhiteBalanceSet::CrWhiteBalance_Cloudy;
    if (wb == "Tungsten" || wb == "3200K") return SCRSDK::CrWhiteBalanceSet::CrWhiteBalance_Tungsten;
    if (wb == "Fluorescent_WarmWhite") return SCRSDK::CrWhiteBalanceSet::CrWhiteBalance_Fluorescent_WarmWhite;
    if (wb == "Fluorescent_CoolWhite") return SCRSDK::CrWhiteBalanceSet::CrWhiteBalance_Fluorescent_CoolWhite;
    if (wb == "Fluorescent_DayWhite") return SCRSDK::CrWhiteBalanceSet::CrWhiteBalance_Fluorescent_DayWhite;
    if (wb == "Fluorescent_Daylight") return SCRSDK::CrWhiteBalanceSet::CrWhiteBalance_Fluorescent_Daylight;
    if (wb == "Flash") return SCRSDK::CrWhiteBalanceSet::CrWhiteBalance_Flash;
    return 0;
}

CrInt32u CameraController::mapFrameRate(const std::string& fps) {
    // Frame rate mapping depends on recording format
    // These are common values, actual values may vary by camera model
    if (fps == "24p" || fps == "23.98p") return 0x01;
    if (fps == "25p") return 0x02;
    if (fps == "30p" || fps == "29.97p") return 0x03;
    if (fps == "50p") return 0x04;
    if (fps == "60p" || fps == "59.94p") return 0x05;
    if (fps == "120p" || fps == "119.88p") return 0x06;
    return 0;
}

// Batch operations
bool CameraController::syncRecordStart(const std::vector<CameraController*>& cameras) {
    std::printf("🔴 Starting synchronized recording on %zu cameras...\n", cameras.size());
    
    bool all_success = true;
    for (auto* camera : cameras) {
        if (camera && !camera->startRecording()) {
            all_success = false;
        }
    }
    
    if (all_success) {
        std::printf("✅ All cameras recording\n");
    } else {
        std::printf("⚠️  Some cameras failed to start recording\n");
    }
    
    return all_success;
}

bool CameraController::syncRecordStop(const std::vector<CameraController*>& cameras) {
    std::printf("⏹️  Stopping synchronized recording on %zu cameras...\n", cameras.size());
    
    bool all_success = true;
    for (auto* camera : cameras) {
        if (camera && !camera->stopRecording()) {
            all_success = false;
        }
    }
    
    if (all_success) {
        std::printf("✅ All cameras stopped recording\n");
    } else {
        std::printf("⚠️  Some cameras failed to stop recording\n");
    }
    
    return all_success;
}

bool CameraController::setAllISO(const std::vector<CameraController*>& cameras, int iso) {
    std::printf("🎚️  Setting ISO %d on %zu cameras...\n", iso, cameras.size());
    
    bool all_success = true;
    for (auto* camera : cameras) {
        if (camera && !camera->setISO(iso)) {
            all_success = false;
        }
    }
    
    return all_success;
}

void CameraController::disconnect() {
    if (is_connected && device_handle != 0) {
        SCRSDK::Disconnect(device_handle);
        is_connected = false;
        device_handle = 0;
    }
    
    if (camera_info) {
        camera_info->Release();
        camera_info = nullptr;
    }
}