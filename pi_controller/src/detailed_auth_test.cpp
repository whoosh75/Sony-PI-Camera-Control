#include "CameraRemote_SDK.h"
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <arpa/inet.h>

int main() {
    std::cout << "🔍 Detailed Sony Camera Authentication Analysis\n";
    std::cout << "===============================================\n";

    auto sdk_result = SCRSDK::Init();
    if (SCRSDK::CrError_None != sdk_result) {
        std::cout << "❌ SDK Init failed: 0x" << std::hex << sdk_result << std::endl;
        return 1;
    }
    std::cout << "✅ Sony CRSDK initialized\n";

    // Create camera connection
    struct in_addr ina;
    inet_aton("192.168.1.110", &ina);
    CrInt32u ipAddr = (CrInt32u)ina.s_addr;
    CrInt8u macBuf[6] = {0};
    SCRSDK::ICrCameraObjectInfo* pCamera = nullptr;
    
    auto err = SCRSDK::CreateCameraObjectInfoEthernetConnection(&pCamera, 
        SCRSDK::CrCameraDeviceModelList::CrCameraDeviceModel_MPC_2610, 
        ipAddr, macBuf, 1);
    
    if (SCRSDK::CrError_None != err || !pCamera) {
        std::cout << "❌ Camera creation failed: 0x" << std::hex << err << std::endl;
        SCRSDK::Release();
        return 1;
    }
    
    std::cout << "📷 Camera: " << pCamera->GetModel() << std::endl;

    // Test 1: Try to get the actual fingerprint from camera
    std::cout << "\n🔍 Test 1: Getting fingerprint from camera...\n";
    char retrieved_fp[256] = {0};
    CrInt32u fp_size = sizeof(retrieved_fp);
    auto fp_result = SCRSDK::GetFingerprint(pCamera, retrieved_fp, &fp_size);
    
    if (SCRSDK::CrError_None == fp_result && fp_size > 0) {
        std::string camera_fingerprint(retrieved_fp, fp_size);
        std::cout << "✅ Retrieved fingerprint: " << camera_fingerprint << " (length: " << fp_size << ")\n";
        
        // Test with retrieved fingerprint
        std::cout << "\n🔐 Test 2: Connecting with retrieved fingerprint...\n";
        SCRSDK::CrDeviceHandle deviceHandle = 0;
        auto connect_result = SCRSDK::Connect(pCamera, nullptr, &deviceHandle, 
            SCRSDK::CrSdkControlMode_Remote, SCRSDK::CrReconnecting_OFF, 
            "admin", "Password1", camera_fingerprint.c_str(), (CrInt32u)camera_fingerprint.length());
            
        std::cout << "📊 Connection result: 0x" << std::hex << connect_result << std::dec;
        std::cout << " (handle: " << deviceHandle << ")\n";
        
        if (SCRSDK::CrError_None == connect_result && deviceHandle != 0) {
            std::cout << "🎉 SUCCESS with retrieved fingerprint!\n";
            
            // Try a simple command
            std::cout << "\n📹 Testing recording command...\n";
            auto cmd_result = SCRSDK::SendCommand(deviceHandle, 
                SCRSDK::CrCommandId::CrCommandId_MovieRecord, 
                SCRSDK::CrCommandParam::CrCommandParam_Down);
                
            std::cout << "📊 Command result: 0x" << std::hex << cmd_result << std::dec << std::endl;
            
            if (SCRSDK::CrError_None == cmd_result) {
                std::cout << "🎊 RECORDING COMMAND SUCCESSFUL!\n";
                std::this_thread::sleep_for(std::chrono::seconds(3));
                
                // Stop recording
                SCRSDK::SendCommand(deviceHandle, 
                    SCRSDK::CrCommandId::CrCommandId_MovieRecord, 
                    SCRSDK::CrCommandParam::CrCommandParam_Up);
                std::cout << "⏹️  Recording stopped\n";
            } else {
                std::cout << "❌ Command failed even with good connection\n";
            }
            
            SCRSDK::Disconnect(deviceHandle);
        } else {
            std::cout << "❌ Connection failed with retrieved fingerprint\n";
        }
    } else {
        std::cout << "⚠️  GetFingerprint failed: 0x" << std::hex << fp_result << std::dec << std::endl;
    }

    // Test 3: Try with hardcoded fingerprint from camera screen
    std::cout << "\n🔐 Test 3: Connecting with camera screen fingerprint...\n";
    std::string screen_fingerprint = "qVAzJJVe91CzJ5VfAGSInaCRezSMWIHW9vhXoMGg";
    SCRSDK::CrDeviceHandle deviceHandle2 = 0;
    
    auto connect_result2 = SCRSDK::Connect(pCamera, nullptr, &deviceHandle2, 
        SCRSDK::CrSdkControlMode_Remote, SCRSDK::CrReconnecting_OFF, 
        "admin", "Password1", screen_fingerprint.c_str(), (CrInt32u)screen_fingerprint.length());
        
    std::cout << "📊 Connection result: 0x" << std::hex << connect_result2 << std::dec;
    std::cout << " (handle: " << deviceHandle2 << ")\n";
    
    if (SCRSDK::CrError_None == connect_result2 && deviceHandle2 != 0) {
        std::cout << "🎉 SUCCESS with screen fingerprint!\n";
        SCRSDK::Disconnect(deviceHandle2);
    }

    // Test 4: Try without fingerprint but with correct password
    std::cout << "\n🔐 Test 4: Connecting without fingerprint...\n";
    SCRSDK::CrDeviceHandle deviceHandle3 = 0;
    
    auto connect_result3 = SCRSDK::Connect(pCamera, nullptr, &deviceHandle3, 
        SCRSDK::CrSdkControlMode_Remote, SCRSDK::CrReconnecting_OFF, 
        "admin", "Password1", "", 0);
        
    std::cout << "📊 Connection result: 0x" << std::hex << connect_result3 << std::dec;
    std::cout << " (handle: " << deviceHandle3 << ")\n";

    // Cleanup
    if (pCamera) pCamera->Release();
    SCRSDK::Release();
    
    std::cout << "\n🏁 Authentication analysis complete!\n";
    std::cout << "👀 Check camera screen for any authentication dialogs during tests\n";
    return 0;
}