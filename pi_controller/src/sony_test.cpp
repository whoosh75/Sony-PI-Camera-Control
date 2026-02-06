#include "CameraRemote_SDK.h"
#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <chrono>
#include <arpa/inet.h>

int main() {
    std::cout << "🚀 Sony Camera Test with Direct SDK Commands\n";
    std::cout << "===========================================\n";

    // Initialize SDK
    auto sdk_result = SCRSDK::Init();
    if (SCRSDK::CrError_None != sdk_result) {
        std::cout << "❌ Failed to initialize SDK: 0x" << std::hex << sdk_result << std::endl;
        return 1;
    }
    std::cout << "✅ Sony CRSDK initialized successfully\n";

    // Convert IP string to integer (using pattern from connection_test.cpp)
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
    
    std::cout << "\n🔐 Attempting to connect with admin/Password1...\n";
    
    // Connect using pattern from connection_test.cpp with password authentication
    SCRSDK::CrError connect_result = SCRSDK::Connect(pCamera, nullptr, &deviceHandle, 
        SCRSDK::CrSdkControlMode_Remote, SCRSDK::CrReconnecting_OFF, 
        "admin", "Password1", "", 0);
    
    if (SCRSDK::CrError_None == connect_result && deviceHandle != 0) {
        std::cout << "🎉 Successfully connected to camera! (handle: " << deviceHandle << ")\n";
        
        // Wait for connection to stabilize
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        // Try to send a recording command
        std::cout << "\n📹 Attempting to send movie recording start command...\n";
        
        // Send movie recording start command (Down = press button)
        auto rec_status = SCRSDK::SendCommand(deviceHandle, SCRSDK::CrCommandId::CrCommandId_MovieRecord, SCRSDK::CrCommandParam::CrCommandParam_Down);
        if (SCRSDK::CrError_None == rec_status) {
            std::cout << "✅ Recording START command sent successfully!\n";
        } else {
            std::cout << "❌ Recording START command failed: 0x" << std::hex << rec_status << std::dec << std::endl;
        }
        
        // Wait for recording to start
        std::this_thread::sleep_for(std::chrono::seconds(5));
        
        // Send stop command (Up = release button)
        std::cout << "⏹️  Sending STOP recording command...\n";
        auto stop_status = SCRSDK::SendCommand(deviceHandle, SCRSDK::CrCommandId::CrCommandId_MovieRecord, SCRSDK::CrCommandParam::CrCommandParam_Up);
        if (SCRSDK::CrError_None == stop_status) {
            std::cout << "✅ Recording STOP command sent successfully!\n";
        } else {
            std::cout << "❌ Recording STOP command failed: 0x" << std::hex << stop_status << std::dec << std::endl;
        }
        
        // Disconnect
        std::cout << "\n📡 Disconnecting...\n";
        auto disconnect_status = SCRSDK::Disconnect(deviceHandle);
        if (SCRSDK::CrError_None == disconnect_status) {
            std::cout << "✅ Disconnected successfully\n";
        } else {
            std::cout << "⚠️  Disconnect status: 0x" << std::hex << disconnect_status << std::dec << std::endl;
        }
    } else {
        std::cout << "❌ Failed to connect to camera: 0x" << std::hex << connect_result << std::dec << std::endl;
        std::cout << "   Device handle: " << deviceHandle << std::endl;
    }

    // Cleanup
    if (pCamera) pCamera->Release();
    SCRSDK::Release();

    return (connect_result == SCRSDK::CrError_None) ? 0 : 1;
}