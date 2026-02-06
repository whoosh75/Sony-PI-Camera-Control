#include <iostream>
#include <thread>
#include <chrono>
#include <arpa/inet.h>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <string>
#include "CameraRemote_SDK.h"

namespace {
bool parse_mac(const std::string& mac_str, CrInt8u mac[6]) {
    unsigned int bytes[6];
    if (std::sscanf(mac_str.c_str(), "%02x:%02x:%02x:%02x:%02x:%02x",
                    &bytes[0], &bytes[1], &bytes[2], &bytes[3], &bytes[4], &bytes[5]) != 6) {
        return false;
    }
    for (int i = 0; i < 6; ++i) {
        mac[i] = static_cast<CrInt8u>(bytes[i]);
    }
    return true;
}

std::string get_env_or_default(const char* key, const char* fallback) {
    const char* val = std::getenv(key);
    return val ? std::string(val) : std::string(fallback);
}
} // namespace

int main() {
    std::cout << "🚀 Direct Sony Camera Record Control (Fast API)" << std::endl;
    
    // Initialize SDK
    const bool init_ok = SCRSDK::Init();
    if (!init_ok) {
        std::cout << "❌ Failed to initialize SDK (Init returned false)." << std::endl;
        return 1;
    }
    std::cout << "✅ Sony CRSDK initialized successfully\n";

    const std::string ip_str = get_env_or_default("SONY_CAMERA_IP", "192.168.33.94");
    const std::string mac_str = get_env_or_default("SONY_CAMERA_MAC", "");
    const std::string user = get_env_or_default("SONY_USER", "admin");
    const std::string pass = get_env_or_default("SONY_PASS", "Password1");
    const std::string accept_fp = get_env_or_default("SONY_ACCEPT_FINGERPRINT", "0");

    // Convert IP string to integer
    struct in_addr ina;
    if (inet_aton(ip_str.c_str(), &ina) == 0) {
        std::cout << "❌ Invalid IP address format\n";
        SCRSDK::Release();
        return 1;
    }
    CrInt32u ipAddr = (CrInt32u)ina.s_addr;
    
    CrInt8u macBuf[6] = {0};
    if (!mac_str.empty()) {
        if (!parse_mac(mac_str, macBuf)) {
            std::cout << "❌ Invalid MAC address format: " << mac_str << std::endl;
            SCRSDK::Release();
            return 1;
        }
    }
    SCRSDK::ICrCameraObjectInfo* pCamera = nullptr;
    
    auto err = SCRSDK::CreateCameraObjectInfoEthernetConnection(&pCamera, 
        SCRSDK::CrCameraDeviceModelList::CrCameraDeviceModel_MPC_2610, 
        ipAddr, macBuf, 1);
    
    if (SCRSDK::CrError_None != err || !pCamera) {
        std::cout << "❌ Failed to create camera connection. Error: 0x" << std::hex << err << std::endl;
        SCRSDK::Release();
        return 1;
    }
    
    std::cout << "📷 Camera: " << pCamera->GetModel() << " (" << pCamera->GetId() << ")\n";

    SCRSDK::CrDeviceHandle deviceHandle = 0;
    
    std::cout << "\n🔐 Connecting with direct authentication...\n";

    std::string fingerprint;
    CrInt32u fp_size = 0;
    if (accept_fp == "1") {
        char fp_buf[1024] = {0};
        auto fp_result = SCRSDK::GetFingerprint(pCamera, fp_buf, &fp_size);
        if (fp_result == SCRSDK::CrError_None && fp_size > 0) {
            fingerprint.assign(fp_buf, fp_size);
            std::cout << "✅ Retrieved fingerprint (length: " << fp_size << ")" << std::endl;
        } else {
            std::cout << "⚠️  Failed to retrieve fingerprint: 0x" << std::hex << fp_result << std::dec << std::endl;
        }
    }

    const char* fp_ptr = fingerprint.empty() ? nullptr : fingerprint.c_str();
    const CrInt32u fp_len = fingerprint.empty() ? 0 : static_cast<CrInt32u>(fingerprint.size());

    // Connect using direct authentication
    SCRSDK::CrError connect_result = SCRSDK::Connect(pCamera, nullptr, &deviceHandle, 
        SCRSDK::CrSdkControlMode_Remote, SCRSDK::CrReconnecting_OFF, 
        user.c_str(), pass.c_str(), fp_ptr, fp_len);
    
    if (SCRSDK::CrError_None == connect_result && deviceHandle != 0) {
        std::cout << "🎉 Successfully connected! Handle: " << deviceHandle << std::endl;
        
        // Method 1: Direct movie recording command (fastest)
        std::cout << "\n📹 STARTING recording with direct API call...\n";
        auto start_result = SCRSDK::SendCommand(deviceHandle, 
            SCRSDK::CrCommandId::CrCommandId_MovieRecord, 
            SCRSDK::CrCommandParam::CrCommandParam_Down);
        
        if (SCRSDK::CrError_None == start_result) {
            std::cout << "✅ Recording STARTED instantly!\n";
            
            // Record for 3 seconds
            std::this_thread::sleep_for(std::chrono::seconds(3));
            
            // Stop recording
            std::cout << "⏹️  STOPPING recording...\n";
            auto stop_result = SCRSDK::SendCommand(deviceHandle, 
                SCRSDK::CrCommandId::CrCommandId_MovieRecord, 
                SCRSDK::CrCommandParam::CrCommandParam_Up);
            
            if (SCRSDK::CrError_None == stop_result) {
                std::cout << "✅ Recording STOPPED instantly!\n";
            } else {
                std::cout << "❌ Stop command failed: 0x" << std::hex << stop_result << std::endl;
            }
            
        } else {
            std::cout << "❌ Start command failed: 0x" << std::hex << start_result << std::endl;
        }
        
        // Disconnect
        auto disconnect_status = SCRSDK::Disconnect(deviceHandle);
        if (SCRSDK::CrError_None == disconnect_status) {
            std::cout << "🔌 Disconnected successfully\n";
        }
        
    } else {
        std::cout << "❌ Failed to connect: 0x" << std::hex << connect_result << std::endl;
    }

    // Cleanup
    if (pCamera) pCamera->Release();
    SCRSDK::Release();

    return (connect_result == SCRSDK::CrError_None) ? 0 : 1;
}