#include "camera_controller.hpp"
#include "CRSDK/CrDeviceProperty.h"
#include <chrono>
#include <thread>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <cctype>
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
    
    // Send shutter press (full press then release)
    auto err1 = SCRSDK::SendCommand(device_handle,
                                   SCRSDK::CrCommandId::CrCommandId_Release,
                                   SCRSDK::CrCommandParam::CrCommandParam_Down);
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    auto err2 = SCRSDK::SendCommand(device_handle,
                                   SCRSDK::CrCommandId::CrCommandId_Release,
                                   SCRSDK::CrCommandParam::CrCommandParam_Up);
    
    if (CR_FAILED(err1) || CR_FAILED(err2)) {
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

    if (!ensurePropertySettable(SCRSDK::CrDeviceProperty_IsoSensitivity, "ISO")) {
        return false;
    }
    
    std::printf("[%s] 🎚️  Setting ISO to %d...\n", camera_name.c_str(), iso_value);
    
    CrInt32u iso_mapped = mapISOValue(iso_value);
    if (iso_mapped == 0) {
        last_error = "Invalid ISO value: " + std::to_string(iso_value);
        return false;
    }
    
    if (!setCameraProperty(SCRSDK::CrDeviceProperty_IsoSensitivity,
                           &iso_mapped,
                           sizeof(iso_mapped),
                           SCRSDK::CrDataType_UInt32Array)) {
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

    if (!ensurePropertySettable(SCRSDK::CrDeviceProperty_WhiteBalance, "WhiteBalance")) {
        return false;
    }
    
    std::printf("[%s] ⚪ Setting white balance to %s...\n", camera_name.c_str(), wb.c_str());
    
    CrInt32u wb_mapped = mapWhiteBalance(wb);
    if (wb_mapped == 0) {
        last_error = "Invalid white balance: " + wb;
        return false;
    }
    
    if (!setCameraProperty(SCRSDK::CrDeviceProperty_WhiteBalance,
                           &wb_mapped,
                           sizeof(wb_mapped),
                           SCRSDK::CrDataType_UInt16Array)) {
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

    if (!ensurePropertySettable(SCRSDK::CrDeviceProperty_Movie_Recording_FrameRateSetting, "FrameRate")) {
        return false;
    }
    
    std::printf("[%s] 🎬 Setting frame rate to %s...\n", camera_name.c_str(), fps.c_str());
    
    // Frame rate is often part of recording settings
    CrInt32u fps_mapped = mapFrameRate(fps);
    if (fps_mapped == 0) {
        last_error = "Invalid frame rate: " + fps;
        return false;
    }
    
    if (!setCameraProperty(SCRSDK::CrDeviceProperty_Movie_Recording_FrameRateSetting,
                           &fps_mapped,
                           sizeof(fps_mapped),
                           SCRSDK::CrDataType_UInt8Array)) {
        return false;
    }
    
    std::printf("[%s] ✅ Frame rate set to %s\n", camera_name.c_str(), fps.c_str());
    return true;
}

bool CameraController::setShutterSpeed(const std::string& speed) {
    if (!is_connected) {
        last_error = "Camera not connected";
        return false;
    }

    if (!ensurePropertySettable(SCRSDK::CrDeviceProperty_ShutterSpeed, "ShutterSpeed")) {
        return false;
    }
    
    std::printf("[%s] ⏱️  Setting shutter speed to %s...\n", camera_name.c_str(), speed.c_str());
    
    CrInt32u speed_mapped = mapShutterSpeed(speed);
    if (speed_mapped == 0) {
        last_error = "Invalid shutter speed: " + speed;
        return false;
    }
    
    if (!setCameraProperty(SCRSDK::CrDeviceProperty_ShutterSpeed,
                           &speed_mapped,
                           sizeof(speed_mapped),
                           SCRSDK::CrDataType_UInt32Array)) {
        return false;
    }
    
    std::printf("[%s] ✅ Shutter speed set to %s\n", camera_name.c_str(), speed.c_str());
    return true;
}

bool CameraController::setCameraProperty(CrInt32u property, const void* value, CrInt32u size, SCRSDK::CrDataType type) {
    if (!is_connected) {
        last_error = "Camera not connected";
        return false;
    }
    if (value == nullptr || size <= 0) {
        last_error = "Invalid property value";
        return false;
    }

    CrInt64u v = 0;
    switch (size) {
        case sizeof(CrInt8u):
            v = *static_cast<const CrInt8u*>(value);
            break;
        case sizeof(CrInt16u):
            v = *static_cast<const CrInt16u*>(value);
            break;
        case sizeof(CrInt32u):
            v = *static_cast<const CrInt32u*>(value);
            break;
        case sizeof(CrInt64u):
            v = *static_cast<const CrInt64u*>(value);
            break;
        default:
            last_error = "Unsupported property value size";
            return false;
    }

    SCRSDK::CrDeviceProperty prop;
    prop.SetCode(property);
    prop.SetValueType(type);
    prop.SetCurrentValue(v);

    auto err = SCRSDK::SetDeviceProperty(device_handle, &prop);
    if (CR_FAILED(err)) {
        last_error = "SetDeviceProperty failed: " + std::to_string(err);
        std::printf("[%s] ❌ SetDeviceProperty failed: 0x%08X\n", camera_name.c_str(), err);
        return false;
    }

    return true;
}

bool CameraController::ensurePropertySettable(CrInt32u property, const char* label) {
    if (!is_connected) {
        last_error = "Camera not connected";
        return false;
    }

    SCRSDK::CrDeviceProperty* props = nullptr;
    CrInt32 num_props = 0;
    const CrInt32u code = property;
    auto err = SCRSDK::GetSelectDeviceProperties(device_handle, 1, const_cast<CrInt32u*>(&code), &props, &num_props);
    if (CR_FAILED(err) || !props || num_props <= 0) {
        last_error = std::string(label) + " not supported (GetSelectDeviceProperties failed)";
        std::printf("[%s] ❌ %s not supported: 0x%08X\n", camera_name.c_str(), label, err);
        if (props) SCRSDK::ReleaseDeviceProperties(device_handle, props);
        return false;
    }

    const bool can_set = props[0].IsSetEnableCurrentValue();
    SCRSDK::ReleaseDeviceProperties(device_handle, props);

    if (!can_set) {
        last_error = std::string(label) + " is read-only on this camera";
        std::printf("[%s] ❌ %s is read-only on this camera\n", camera_name.c_str(), label);
        return false;
    }

    return true;
}

// Mapping functions for Sony CRSDK values
CrInt32u CameraController::mapISOValue(int iso) {
    // Based on Sony CRSDK documentation:
    // ISO sensitivity format: bit 28-31 extension, bit 24-27 ISO mode, bit 0-23 ISO value
    // For normal ISO mode, we just return the ISO value in the lower 24 bits
    
    // Validate common ISO values
    switch (iso) {
        case 50:   
        case 64:   
        case 80:   
        case 100:  
        case 125:  
        case 160:  
        case 200:  
        case 250:  
        case 320:  
        case 400:  
        case 500:  
        case 640:  
        case 800:  
        case 1000: 
        case 1250: 
        case 1600: 
        case 2000: 
        case 2500: 
        case 3200: 
        case 4000: 
        case 5000: 
        case 6400: 
        case 8000: 
        case 10000:
        case 12800:
        case 16000:
        case 20000:
        case 25600:
            // Return ISO value with normal mode in bits 24-27 (0x00)
            return (SCRSDK::CrISO_Normal << 24) | (iso & 0xFFFFFF);
        default: 
            return 0; // Invalid ISO
    }
}

CrInt32u CameraController::mapWhiteBalance(const std::string& wb) {
    if (wb == "Auto" || wb == "AWB") return SCRSDK::CrWhiteBalance_AWB;
    if (wb == "Daylight" || wb == "5600K") return SCRSDK::CrWhiteBalance_Daylight;
    if (wb == "Shade" || wb == "7000K") return SCRSDK::CrWhiteBalance_Shadow;
    if (wb == "Cloudy" || wb == "6000K") return SCRSDK::CrWhiteBalance_Cloudy;
    if (wb == "Tungsten" || wb == "3200K") return SCRSDK::CrWhiteBalance_Tungsten;
    if (wb == "Fluorescent_WarmWhite") return SCRSDK::CrWhiteBalance_Fluorescent_WarmWhite;
    if (wb == "Fluorescent_CoolWhite") return SCRSDK::CrWhiteBalance_Fluorescent_CoolWhite;
    if (wb == "Fluorescent_DayWhite") return SCRSDK::CrWhiteBalance_Fluorescent_DayWhite;
    if (wb == "Fluorescent_Daylight") return SCRSDK::CrWhiteBalance_Fluorescent_Daylight;
    if (wb == "Flash") return SCRSDK::CrWhiteBalance_Flush;
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

CrInt32u CameraController::mapShutterSpeed(const std::string& speed) {
    // Common shutter speed mappings
    // Note: These values are approximate and may need adjustment based on actual Sony CRSDK constants
    if (speed == "1/4000" || speed == "1/4000s") return 0x00000001;
    if (speed == "1/3200" || speed == "1/3200s") return 0x00000002;
    if (speed == "1/2500" || speed == "1/2500s") return 0x00000003;
    if (speed == "1/2000" || speed == "1/2000s") return 0x00000004;
    if (speed == "1/1600" || speed == "1/1600s") return 0x00000005;
    if (speed == "1/1250" || speed == "1/1250s") return 0x00000006;
    if (speed == "1/1000" || speed == "1/1000s") return 0x00000007;
    if (speed == "1/800" || speed == "1/800s") return 0x00000008;
    if (speed == "1/640" || speed == "1/640s") return 0x00000009;
    if (speed == "1/500" || speed == "1/500s") return 0x0000000A;
    if (speed == "1/400" || speed == "1/400s") return 0x0000000B;
    if (speed == "1/320" || speed == "1/320s") return 0x0000000C;
    if (speed == "1/250" || speed == "1/250s") return 0x0000000D;
    if (speed == "1/200" || speed == "1/200s") return 0x0000000E;
    if (speed == "1/160" || speed == "1/160s") return 0x0000000F;
    if (speed == "1/125" || speed == "1/125s") return 0x00000010;
    if (speed == "1/100" || speed == "1/100s") return 0x00000011;
    if (speed == "1/80" || speed == "1/80s") return 0x00000012;
    if (speed == "1/60" || speed == "1/60s") return 0x00000013;
    if (speed == "1/50" || speed == "1/50s") return 0x00000014;
    if (speed == "1/40" || speed == "1/40s") return 0x00000015;
    if (speed == "1/30" || speed == "1/30s") return 0x00000016;
    if (speed == "1/25" || speed == "1/25s") return 0x00000017;
    if (speed == "1/20" || speed == "1/20s") return 0x00000018;
    if (speed == "1/15" || speed == "1/15s") return 0x00000019;
    if (speed == "1/13" || speed == "1/13s") return 0x0000001A;
    if (speed == "1/10" || speed == "1/10s") return 0x0000001B;
    if (speed == "1/8" || speed == "1/8s") return 0x0000001C;
    if (speed == "1/6" || speed == "1/6s") return 0x0000001D;
    if (speed == "1/5" || speed == "1/5s") return 0x0000001E;
    if (speed == "1/4" || speed == "1/4s") return 0x0000001F;
    if (speed == "1/3" || speed == "1/3s") return 0x00000020;
    if (speed == "1/2.5" || speed == "1/2.5s") return 0x00000021;
    if (speed == "1/2" || speed == "1/2s") return 0x00000022;
    if (speed == "1/1.6" || speed == "1/1.6s") return 0x00000023;
    if (speed == "1" || speed == "1s") return 0x00000024;
    if (speed == "1.3" || speed == "1.3s") return 0x00000025;
    if (speed == "1.6" || speed == "1.6s") return 0x00000026;
    if (speed == "2" || speed == "2s") return 0x00000027;
    if (speed == "2.5" || speed == "2.5s") return 0x00000028;
    if (speed == "3.2" || speed == "3.2s") return 0x00000029;
    if (speed == "4" || speed == "4s") return 0x0000002A;
    if (speed == "5" || speed == "5s") return 0x0000002B;
    if (speed == "6" || speed == "6s") return 0x0000002C;
    if (speed == "8" || speed == "8s") return 0x0000002D;
    if (speed == "10" || speed == "10s") return 0x0000002E;
    if (speed == "13" || speed == "13s") return 0x0000002F;
    if (speed == "15" || speed == "15s") return 0x00000030;
    if (speed == "20" || speed == "20s") return 0x00000031;
    if (speed == "25" || speed == "25s") return 0x00000032;
    if (speed == "30" || speed == "30s") return 0x00000033;
    if (speed == "Bulb") return SCRSDK::CrShutterSpeed_Bulb;
    return 0;  // Unknown speed
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

bool CameraController::connectViaUSB() {
    std::printf("[%s] Connecting via USB...\n", camera_name.c_str());

    static bool sdk_inited = false;
    if (!sdk_inited) {
        if (!SCRSDK::Init()) {
            last_error = "SCRSDK::Init failed";
            return false;
        }
        sdk_inited = true;
    }

    SCRSDK::ICrEnumCameraObjectInfo* camera_list = nullptr;
    auto err = SCRSDK::EnumCameraObjects(&camera_list);
    if (CR_FAILED(err) || !camera_list || camera_list->GetCount() == 0) {
        last_error = "No USB cameras found";
        if (camera_list) camera_list->Release();
        return false;
    }

    auto info = camera_list->GetCameraObjectInfo(0);
    if (!info) {
        last_error = "Failed to get camera info";
        camera_list->Release();
        return false;
    }

    SCRSDK::CrDeviceHandle handle = 0;
    err = SCRSDK::Connect(const_cast<SCRSDK::ICrCameraObjectInfo*>(info),
                          &callback,
                          &handle);
    camera_list->Release();

    if (CR_FAILED(err)) {
        last_error = "Camera USB connection failed: " + std::to_string(err);
        return false;
    }

    device_handle = handle;
    is_connected = true;
    std::printf("[%s] ✅ Connected via USB! Device handle: %u\n", camera_name.c_str(), device_handle);
    return true;
}

bool CameraController::sendCameraCommand(CrInt32u command, CrInt32u param) {
    if (!is_connected) {
        last_error = "Camera not connected";
        return false;
    }

    auto err = SCRSDK::SendCommand(device_handle,
                                   static_cast<SCRSDK::CrCommandId>(command),
                                   static_cast<SCRSDK::CrCommandParam>(param));
    if (CR_FAILED(err)) {
        last_error = "SendCommand failed: " + std::to_string(err);
        return false;
    }

    return true;
}

bool CameraController::getCameraProperty(CrInt32u property, void* value, CrInt32u* size) {
    if (!is_connected) {
        last_error = "Camera not connected";
        return false;
    }
    if (!value || !size || *size <= 0) {
        last_error = "Invalid property buffer";
        return false;
    }

    SCRSDK::CrDeviceProperty* props = nullptr;
    CrInt32 num_props = 0;
    CrInt32u code = property;
    auto err = SCRSDK::GetSelectDeviceProperties(device_handle, 1, &code, &props, &num_props);
    if (CR_FAILED(err) || !props || num_props <= 0) {
        last_error = "GetSelectDeviceProperties failed: " + std::to_string(err);
        if (props) SCRSDK::ReleaseDeviceProperties(device_handle, props);
        return false;
    }

    const CrInt64u current = props[0].GetCurrentValue();
    SCRSDK::ReleaseDeviceProperties(device_handle, props);

    if (*size >= static_cast<CrInt32u>(sizeof(CrInt64u))) {
        *static_cast<CrInt64u*>(value) = current;
        *size = sizeof(CrInt64u);
        return true;
    }
    if (*size >= static_cast<CrInt32u>(sizeof(CrInt32u))) {
        *static_cast<CrInt32u*>(value) = static_cast<CrInt32u>(current & 0xFFFFFFFFu);
        *size = sizeof(CrInt32u);
        return true;
    }
    if (*size >= static_cast<CrInt32u>(sizeof(CrInt16u))) {
        *static_cast<CrInt16u*>(value) = static_cast<CrInt16u>(current & 0xFFFFu);
        *size = sizeof(CrInt16u);
        return true;
    }
    if (*size >= static_cast<CrInt32u>(sizeof(CrInt8u))) {
        *static_cast<CrInt8u*>(value) = static_cast<CrInt8u>(current & 0xFFu);
        *size = sizeof(CrInt8u);
        return true;
    }

    last_error = "Property buffer too small";
    return false;
}

RecordingState CameraController::getRecordingState() {
    CrInt32u value = 0;
    CrInt32u size = sizeof(value);
    if (!getCameraProperty(SCRSDK::CrDeviceProperty_RecordingState, &value, &size)) {
        return RecordingState::UNKNOWN;
    }

    if (value == 0) return RecordingState::STOPPED;
    return RecordingState::RECORDING;
}

CameraStatus CameraController::getCameraStatus() {
    CameraStatus status{};
    status.recording_state = getRecordingState();
    status.camera_ready = is_connected;
    status.battery_level = -1;

    CrInt32u value32 = 0;
    CrInt32u size32 = sizeof(value32);

    if (getCameraProperty(SCRSDK::CrDeviceProperty_IsoSensitivity, &value32, &size32)) {
        const CrInt32u iso = value32 & 0xFFFFFFu;
        status.iso_value = std::to_string(iso);
    }

    size32 = sizeof(value32);
    if (getCameraProperty(SCRSDK::CrDeviceProperty_WhiteBalance, &value32, &size32)) {
        status.white_balance = std::to_string(value32);
    }

    size32 = sizeof(value32);
    if (getCameraProperty(SCRSDK::CrDeviceProperty_Movie_Recording_FrameRateSetting, &value32, &size32)) {
        status.frame_rate = std::to_string(value32);
    }

    size32 = sizeof(value32);
    if (getCameraProperty(SCRSDK::CrDeviceProperty_FNumber, &value32, &size32)) {
        status.aperture = std::to_string(value32);
    }

    size32 = sizeof(value32);
    if (getCameraProperty(SCRSDK::CrDeviceProperty_ShutterSpeed, &value32, &size32)) {
        status.shutter_speed = std::to_string(value32);
    }

    return status;
}

bool CameraController::isCameraReady() {
    return is_connected;
}

bool CameraController::setAperture(float f_stop) {
    if (!is_connected) {
        last_error = "Camera not connected";
        return false;
    }

    if (!ensurePropertySettable(SCRSDK::CrDeviceProperty_FNumber, "Aperture")) {
        return false;
    }

    const CrInt16u ap = mapAperture(f_stop);
    if (ap == 0) {
        last_error = "Invalid aperture value";
        return false;
    }

    if (!setCameraProperty(SCRSDK::CrDeviceProperty_FNumber,
                           &ap,
                           sizeof(ap),
                           SCRSDK::CrDataType_UInt16Array)) {
        return false;
    }

    return true;
}

CrInt16u CameraController::mapAperture(float f_stop) {
    if (f_stop <= 0.0f) return 0;
    return static_cast<CrInt16u>(std::lround(f_stop * 100.0f));
}