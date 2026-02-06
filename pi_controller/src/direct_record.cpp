#include <iostream>
#include <thread>
#include <chrono>
#include <arpa/inet.h>
#include "CameraRemote_SDK.h"

int main() {
    std::cout << "🚀 Direct Sony Camera Record Control (Fast API)" << std::endl;
    
    // Initialize SDK
    auto init_result = SCRSDK::Init();
    if (SCRSDK::CrError_None != init_result) {
        std::cout << "❌ Failed to initialize SDK: 0x" << std::hex << init_result << std::endl;
        return 1;
    }
    std::cout << "✅ Sony CRSDK initialized successfully\n";

    // Convert IP string to integer
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

    SCRSDK::CrDeviceHandle deviceHandle = 0;
    
    std::cout << "\n🔐 Connecting with direct authentication...\n";
    
    // Connect using fast authentication
    SCRSDK::CrError connect_result = SCRSDK::Connect(pCamera, nullptr, &deviceHandle, 
        SCRSDK::CrSdkControlMode_Remote, SCRSDK::CrReconnecting_OFF, 
        "admin", "Password1", "", 0);
    
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