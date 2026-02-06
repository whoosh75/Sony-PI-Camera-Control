#include "CameraRemote_SDK.h"
#include <iostream>
#include <thread>
#include <chrono>

using namespace SCRSDK;

void print_recording_status(CrDeviceHandle device_handle) {
    CrDeviceProperty* prop_list = nullptr;
    CrInt32 num_props = 0;
    
    auto ret = SCRSDK::GetDeviceProperties(device_handle, &prop_list, &num_props);
    if (ret == CrError_None) {
        bool found_recorder = false;
        bool found_toggle = false;
        
        for (int i = 0; i < num_props; i++) {
            if (prop_list[i].GetCode() == CrDeviceProperty_RecorderMainStatus) {
                std::cout << "📹 Recorder Status: " << std::hex << prop_list[i].GetCurrentValue() 
                         << (prop_list[i].GetCurrentValue() == 0x00000001 ? " (Recording)" : " (Not Recording)") << std::endl;
                found_recorder = true;
            }
            else if (prop_list[i].GetCode() == CrDeviceProperty_MovieRecButtonToggleEnableStatus) {
                std::cout << "🔘 Movie Rec Button Toggle: " << std::hex << prop_list[i].GetCurrentValue()
                         << (prop_list[i].GetCurrentValue() == 0x00000001 ? " (Enabled)" : " (Disabled)") << std::endl;
                found_toggle = true;
            }
        }
        
        if (!found_recorder) std::cout << "❌ RecorderMainStatus property not found" << std::endl;
        if (!found_toggle) std::cout << "❌ MovieRecButtonToggleEnableStatus property not found" << std::endl;
        
        SCRSDK::ReleaseDeviceProperties(device_handle, prop_list);
    } else {
        std::cout << "❌ Failed to get device properties: " << std::hex << ret << std::endl;
    }
}

int main() {
    std::cout << "🎬 Sony A74 USB Recording Test\n";
    std::cout << "===============================\n";
    
    // Initialize SDK
    auto ret = SCRSDK::Init();
    if (ret != CrError_None) {
        std::cerr << "❌ Failed to initialize SDK: " << std::hex << ret << std::endl;
        return -1;
    }
    std::cout << "✅ Sony SDK initialized\n";

    // Wait a bit for USB devices to be detected
    std::cout << "⏳ Waiting for USB camera detection...\n";
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // Enumerate cameras
    ICrEnumCameraObjectInfo* camera_list = nullptr;
    ret = SCRSDK::EnumCameraObjects(&camera_list);
    if (ret != CrError_None || !camera_list || camera_list->GetCount() == 0) {
        std::cerr << "❌ No cameras found via USB" << std::endl;
        std::cerr << "   Make sure:\n";
        std::cerr << "   - Sony A74 is connected via USB\n";
        std::cerr << "   - Camera is powered on\n";
        std::cerr << "   - Camera is in PC Remote mode\n";
        std::cerr << "   - USB cable supports data transfer\n";
        SCRSDK::Release();
        return -1;
    }
    
    std::cout << "✅ Found " << camera_list->GetCount() << " camera(s)\n";

    // Connect to first camera
    auto camera_info = camera_list->GetCameraObjectInfo(0);
    CrDeviceHandle device_handle = 0;

    std::cout << "🔗 Connecting to Sony A74...\n";
    ret = SCRSDK::Connect(const_cast<ICrCameraObjectInfo*>(camera_info), nullptr, &device_handle);
    if (ret != CrError_None) {
        std::cerr << "❌ Failed to connect to camera: " << std::hex << ret << std::endl;
        camera_list->Release();
        SCRSDK::Release();
        return -1;
    }

    std::cout << "✅ Connected to Sony A74!\n\n";

    // Check initial status
    std::cout << "📊 Initial Status:\n";
    print_recording_status(device_handle);
    std::cout << "\n";

    // Test direct recording using CrCommandId_MovieRecord
    std::cout << "🎬 Testing Direct Recording Commands...\n";
    
    // Start recording
    std::cout << "▶️  Starting recording with CrCommandId_MovieRecord...\n";
    ret = SCRSDK::SendCommand(device_handle, CrCommandId_MovieRecord, CrCommandParam_Down);
    if (ret == CrError_None) {
        std::cout << "✅ Record start command sent\n";
        
        // Wait and check status
        std::this_thread::sleep_for(std::chrono::milliseconds(1500));
        std::cout << "📊 Status after start command:\n";
        print_recording_status(device_handle);
        
        // Record for 3 seconds
        std::cout << "\n⏱️  Recording for 3 seconds...\n";
        std::this_thread::sleep_for(std::chrono::seconds(3));

        // Stop recording
        std::cout << "⏹️  Stopping recording...\n";
        ret = SCRSDK::SendCommand(device_handle, CrCommandId_MovieRecord, CrCommandParam_Up);
        if (ret == CrError_None) {
            std::cout << "✅ Record stop command sent\n";
            
            // Wait and check final status
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            std::cout << "📊 Final Status:\n";
            print_recording_status(device_handle);
            
        } else {
            std::cerr << "❌ Failed to stop recording: " << std::hex << ret << std::endl;
        }
        
    } else {
        std::cerr << "❌ Failed to start recording: " << std::hex << ret << std::endl;
        std::cerr << "   This could be due to:\n";
        std::cerr << "   - Camera not in movie mode\n";
        std::cerr << "   - Movie rec button toggle disabled\n";
        std::cerr << "   - Memory card issues\n";
        std::cerr << "   - Camera settings preventing recording\n";
    }

    std::cout << "\n🔌 Disconnecting...\n";
    SCRSDK::Disconnect(device_handle);
    camera_list->Release();
    SCRSDK::Release();
    
    std::cout << "✅ Test completed!\n";
    return 0;
}