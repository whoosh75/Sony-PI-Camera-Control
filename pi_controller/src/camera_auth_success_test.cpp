#include "CameraRemote_SDK.h"
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <arpa/inet.h>

int main() {
    std::cout << "🚀 Sony Camera Test with CORRECT Fingerprint\n";
    std::cout << "============================================\n";

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

    // Use the EXACT fingerprint from camera screen
    std::string fingerprint = "qVAzJJVe91CzJ5VfAGSInaCRezSMWIHW9vhXoMGg";
    std::cout << "\n🔐 Using fingerprint from camera screen: " << fingerprint << "\n";

    // Connect with correct credentials
    std::cout << "\n🔗 Connecting with admin/Password1 + correct fingerprint...\n";
    
    SCRSDK::CrDeviceHandle deviceHandle = 0;
    SCRSDK::CrError connect_result = SCRSDK::Connect(pCamera, nullptr, &deviceHandle, 
        SCRSDK::CrSdkControlMode_Remote, SCRSDK::CrReconnecting_OFF, 
        "admin", "Password1", fingerprint.c_str(), (CrInt32u)fingerprint.length());
    
    if (SCRSDK::CrError_None == connect_result && deviceHandle != 0) {
        std::cout << "🎉 AUTHENTICATION SUCCESS! Device handle: " << deviceHandle << "\n";
        std::cout << "🚀 Camera remote control is now ACTIVE!\n";
        
        // Test movie recording
        std::cout << "\n📹 Testing movie recording...\n";
        
        auto start_result = SCRSDK::SendCommand(deviceHandle, 
            SCRSDK::CrCommandId::CrCommandId_MovieRecord, 
            SCRSDK::CrCommandParam::CrCommandParam_Down);
        
        if (SCRSDK::CrError_None == start_result) {
            std::cout << "✅ Movie recording STARTED successfully!\n";
            std::cout << "🎬 Recording for 5 seconds...\n";
            
            std::this_thread::sleep_for(std::chrono::seconds(5));
            
            // Stop recording
            auto stop_result = SCRSDK::SendCommand(deviceHandle, 
                SCRSDK::CrCommandId::CrCommandId_MovieRecord, 
                SCRSDK::CrCommandParam::CrCommandParam_Up);
            
            if (SCRSDK::CrError_None == stop_result) {
                std::cout << "⏹️  Movie recording STOPPED successfully!\n";
                std::cout << "🎊 SUCCESS: Full camera control working!\n";
            } else {
                std::cout << "❌ Stop command failed: 0x" << std::hex << stop_result << std::dec << std::endl;
            }
        } else {
            std::cout << "❌ Start recording failed: 0x" << std::hex << start_result << std::dec << std::endl;
        }
        
        // Test photo capture
        std::cout << "\n📸 Testing photo capture...\n";
        auto photo_result = SCRSDK::SendCommand(deviceHandle, 
            SCRSDK::CrCommandId::CrCommandId_Release, 
            SCRSDK::CrCommandParam::CrCommandParam_Down);
        
        if (SCRSDK::CrError_None == photo_result) {
            std::cout << "✅ Photo shutter pressed!\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            
            SCRSDK::SendCommand(deviceHandle, 
                SCRSDK::CrCommandId::CrCommandId_Release, 
                SCRSDK::CrCommandParam::CrCommandParam_Up);
            std::cout << "📸 Photo capture completed!\n";
        } else {
            std::cout << "❌ Photo capture failed: 0x" << std::hex << photo_result << std::dec << std::endl;
        }
        
        std::cout << "\n🎊 Camera control test completed successfully!\n";
        
        // Disconnect
        std::cout << "\n📡 Disconnecting...\n";
        auto disconnect_status = SCRSDK::Disconnect(deviceHandle);
        if (SCRSDK::CrError_None == disconnect_status) {
            std::cout << "✅ Disconnected successfully\n";
        }
        
    } else {
        std::cout << "❌ Authentication failed: 0x" << std::hex << connect_result << std::dec << std::endl;
        std::cout << "🔍 Check that the fingerprint on camera screen matches:\n";
        std::cout << "    Expected: " << fingerprint << std::endl;
    }

    // Cleanup
    if (pCamera) pCamera->Release();
    SCRSDK::Release();
    
    std::cout << "\n🏁 Test completed!\n";
    return (connect_result == SCRSDK::CrError_None) ? 0 : 1;
}