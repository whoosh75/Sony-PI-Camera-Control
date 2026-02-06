#include "CameraRemote_SDK.h"
#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <chrono>
#include <arpa/inet.h>

int main() {
    std::cout << "🚀 Sony Camera Recording Test (Fingerprint + Auth Flow)\n";
    std::cout << "========================================================\n";

    // Initialize SDK
    auto sdk_result = SCRSDK::Init();
    if (SCRSDK::CrError_None != sdk_result) {
        std::cout << "❌ Failed to initialize SDK: 0x" << std::hex << sdk_result << std::endl;
        return 1;
    }
    std::cout << "✅ Sony CRSDK initialized successfully\n";

    // Create camera connection
    struct in_addr ina;
    if (inet_aton("192.168.1.110", &ina) == 0) {
        std::cout << "❌ Invalid IP address format\n";
        SCRSDK::Release();
        return 1;
    }
    CrInt32u ipAddr = (CrInt32u)ina.s_addr;
    
    CrInt8u macBuf[6] = {0};
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

    // Step 1: Get fingerprint first (Sony's approach)
    std::cout << "\n🔐 Getting camera fingerprint...\n";
    char fpBuff[256] = {0};
    CrInt32u fpLen = sizeof(fpBuff);
    
    auto fp_result = SCRSDK::GetFingerprint(pCamera, fpBuff, &fpLen);
    
    std::string fingerprint;
    if (SCRSDK::CrError_None == fp_result) {
        fingerprint = std::string(fpBuff, fpLen);
        std::cout << "✅ Fingerprint retrieved successfully (length: " << fpLen << ")\n";
    } else {
        std::cout << "⚠️  Fingerprint retrieval failed: 0x" << std::hex << fp_result << std::dec << std::endl;
        // Continue without fingerprint
    }

    // Step 2: Connect with admin, password, and fingerprint
    std::cout << "\n🔗 Connecting with admin/Password1 + fingerprint...\n";
    
    SCRSDK::CrDeviceHandle deviceHandle = 0;
    SCRSDK::CrError connect_result = SCRSDK::Connect(pCamera, nullptr, &deviceHandle, 
        SCRSDK::CrSdkControlMode_Remote, SCRSDK::CrReconnecting_OFF, 
        "admin", "Password1", fingerprint.c_str(), (CrInt32u)fingerprint.length());
    
    if (SCRSDK::CrError_None == connect_result && deviceHandle != 0) {
        std::cout << "🎉 Authentication successful! Device handle: " << deviceHandle << "\n";
        
        // Step 3: Test camera commands
        std::cout << "\n📹 Testing movie recording commands...\n";
        
        // Start recording
        auto start_result = SCRSDK::SendCommand(deviceHandle, 
            SCRSDK::CrCommandId::CrCommandId_MovieRecord, 
            SCRSDK::CrCommandParam::CrCommandParam_Down);
        
        if (SCRSDK::CrError_None == start_result) {
            std::cout << "✅ Movie recording START command sent!\n";
            
            // Record for 5 seconds
            std::cout << "🎬 Recording for 5 seconds...\n";
            std::this_thread::sleep_for(std::chrono::seconds(5));
            
            // Stop recording
            auto stop_result = SCRSDK::SendCommand(deviceHandle, 
                SCRSDK::CrCommandId::CrCommandId_MovieRecord, 
                SCRSDK::CrCommandParam::CrCommandParam_Up);
            
            if (SCRSDK::CrError_None == stop_result) {
                std::cout << "✅ Movie recording STOP command sent!\n";
                std::cout << "🎊 SUCCESS: Camera commands working!\n";
            } else {
                std::cout << "❌ Stop command failed: 0x" << std::hex << stop_result << std::dec << std::endl;
            }
        } else {
            std::cout << "❌ Start recording command failed: 0x" << std::hex << start_result << std::dec << std::endl;
        }
        
        // Test photo capture
        std::cout << "\n📸 Testing photo capture...\n";
        auto photo_result = SCRSDK::SendCommand(deviceHandle, 
            SCRSDK::CrCommandId::CrCommandId_Release, 
            SCRSDK::CrCommandParam::CrCommandParam_Down);
        
        if (SCRSDK::CrError_None == photo_result) {
            std::cout << "✅ Photo capture command sent!\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            SCRSDK::SendCommand(deviceHandle, 
                SCRSDK::CrCommandId::CrCommandId_Release, 
                SCRSDK::CrCommandParam::CrCommandParam_Up);
        } else {
            std::cout << "❌ Photo capture failed: 0x" << std::hex << photo_result << std::dec << std::endl;
        }
        
        // Disconnect
        std::cout << "\n📡 Disconnecting...\n";
        auto disconnect_status = SCRSDK::Disconnect(deviceHandle);
        if (SCRSDK::CrError_None == disconnect_status) {
            std::cout << "✅ Disconnected successfully\n";
        }
        
    } else {
        std::cout << "❌ Authentication still failed: 0x" << std::hex << connect_result << std::dec << std::endl;
        std::cout << "📝 This suggests the camera may require a different authentication approach\n";
        std::cout << "📝 or there may be camera-specific security settings to configure.\n";
    }

    // Cleanup
    if (pCamera) pCamera->Release();
    SCRSDK::Release();
    
    std::cout << "\n🏁 Test completed!\n";
    return (connect_result == SCRSDK::CrError_None) ? 0 : 1;
}