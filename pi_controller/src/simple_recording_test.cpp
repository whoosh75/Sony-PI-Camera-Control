#include "CRSDK/CameraRemote_SDK.h"  
#include "OpenCVWrapper.h"
#include <iostream>
#include <memory>
#include <chrono>
#include <thread>

int main() {
    std::cout << "🎯 Sony A74 Recording Test (Direct SDK)" << std::endl;
    std::cout << "========================================" << std::endl;
    
    // Initialize SDK exactly like working_sdk_test.cpp
    std::cout << "🚀 Initialize Remote SDK..." << std::endl;
    auto init_result = SCRSDK::Init();
    
    if (init_result != SCRSDK::CrError_None) {
        std::cout << "❌ Failed to initialize SDK with error: " << static_cast<int>(init_result) << std::endl;
        return -1;
    }
    
    std::cout << "✅ Remote SDK successfully initialized." << std::endl;
    
    // Enumerate cameras exactly like working_sdk_test.cpp
    std::cout << "📡 Enumerate connected camera devices..." << std::endl;
    SCRSDK::ICrEnumCameraObjectInfo* camera_list = nullptr;
    auto enum_result = SCRSDK::EnumCameraObjects(&camera_list);
    
    if (enum_result != SCRSDK::CrError_None || !camera_list) {
        std::cout << "❌ Failed to enumerate cameras" << std::endl;
        SCRSDK::Release();
        return -1;
    }
    
    auto num_cameras = camera_list->GetCount();
    std::cout << "📷 Found " << num_cameras << " camera(s)" << std::endl;
    
    // Find Sony A74 (ILCE-7M4)
    SCRSDK::ICrCameraObjectInfo* sony_a74 = nullptr;
    for (int i = 0; i < num_cameras; ++i) {
        auto camera_info = camera_list->GetCameraObjectInfo(i);
        if (camera_info) {
            std::string model = camera_info->GetModel();
            std::cout << "[" << (i+1) << "] " << model << std::endl;
            
            if (model == "ILCE-7M4") {
                sony_a74 = camera_info;
                std::cout << "✅ Found Sony A74!" << std::endl;
            }
        }
    }
    
    if (!sony_a74) {
        std::cout << "❌ Sony A74 (ILCE-7M4) not found" << std::endl;
        SCRSDK::Release();
        return -1;
    }
    
    // Connect to Sony A74 (simple version without callback)
    std::cout << "🔗 Connecting to Sony A74..." << std::endl;
    SCRSDK::CrDeviceHandle device_handle = 0;
    auto connect_result = SCRSDK::Connect(sony_a74, nullptr, &device_handle);
    
    if (connect_result != SCRSDK::CrError_None) {
        std::cout << "❌ Failed to connect: " << static_cast<int>(connect_result) << std::endl;
        SCRSDK::Release();
        return -1;
    }
    
    std::cout << "✅ Connected to Sony A74!" << std::endl;
    
    // Test direct API recording commands
    std::cout << "\n⚡ Testing DIRECT API MovieRecord commands..." << std::endl;
    
    // Start recording
    std::cout << "🎬 Starting recording (API: SendCommand MovieRecord Down)..." << std::endl;
    auto start_result = SCRSDK::SendCommand(device_handle, SCRSDK::CrCommandId_MovieRecord, SCRSDK::CrCommandParam_Down);
    
    if (start_result != SCRSDK::CrError_None) {
        std::cout << "❌ Start recording failed: " << static_cast<int>(start_result) << std::endl;
    } else {
        std::cout << "✅ Start recording command sent successfully!" << std::endl;
        
        // Record for 5 seconds
        std::cout << "📹 Recording for 5 seconds..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(5));
        
        // Stop recording
        std::cout << "⏹️ Stopping recording (API: SendCommand MovieRecord Up)..." << std::endl;
        auto stop_result = SCRSDK::SendCommand(device_handle, SCRSDK::CrCommandId_MovieRecord, SCRSDK::CrCommandParam_Up);
        
        if (stop_result != SCRSDK::CrError_None) {
            std::cout << "❌ Stop recording failed: " << static_cast<int>(stop_result) << std::endl;
        } else {
            std::cout << "✅ Stop recording command sent successfully!" << std::endl;
        }
    }
    
    // Disconnect
    std::cout << "🔌 Disconnecting..." << std::endl;
    SCRSDK::Disconnect(device_handle);
    
    std::cout << "🧹 Releasing SDK..." << std::endl;
    SCRSDK::Release();
    
    std::cout << "\n🎉 Direct SDK recording test completed!" << std::endl;
    return 0;
}