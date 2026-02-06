#include "CameraRemote_SDK.h"
#include <iostream>
#include <thread>
#include <chrono>

using namespace SCRSDK;

int main() {
    // Initialize SDK
    auto ret = SCRSDK::Init();
    if (ret != CrError_None) {
        std::cerr << "Failed to initialize SDK: " << std::hex << ret << std::endl;
        return -1;
    }

    // Enumerate cameras
    ICrEnumCameraObjectInfo* camera_list = nullptr;
    ret = SCRSDK::EnumCameraObjects(&camera_list);
    if (ret != CrError_None || !camera_list || camera_list->GetCount() == 0) {
        std::cerr << "No cameras found" << std::endl;
        SCRSDK::Release();
        return -1;
    }

    // Get first camera
    auto camera_info = camera_list->GetCameraObjectInfo(0);
    CrDeviceHandle device_handle = 0;

    // Connect to camera  
    ret = SCRSDK::Connect(const_cast<ICrCameraObjectInfo*>(camera_info), nullptr, &device_handle);
    if (ret != CrError_None) {
        std::cerr << "Failed to connect to camera: " << std::hex << ret << std::endl;
        camera_list->Release();
        SCRSDK::Release();
        return -1;
    }

    std::cout << "Connected to camera successfully!" << std::endl;

    // Check current recording status
    CrDeviceProperty* prop_list = nullptr;
    CrInt32 num_props = 0;
    
    ret = SCRSDK::GetDeviceProperties(device_handle, &prop_list, &num_props);
    if (ret == CrError_None) {
        for (int i = 0; i < num_props; i++) {
            if (prop_list[i].GetCode() == CrDeviceProperty_RecorderMainStatus) {
                std::cout << "Current recorder status: " << std::hex << prop_list[i].GetCurrentValue() << std::endl;
                break;
            }
        }
        SCRSDK::ReleaseDeviceProperties(device_handle, prop_list);
    }

    // Start recording using CrCommandId_MovieRecord with CrCommandParam_Down
    std::cout << "Starting recording..." << std::endl;
    ret = SCRSDK::SendCommand(device_handle, CrCommandId_MovieRecord, CrCommandParam_Down);
    if (ret == CrError_None) {
        std::cout << "Record start command sent successfully!" << std::endl;
        
        // Wait a bit then check status
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        
        // Check if recording started
        ret = SCRSDK::GetDeviceProperties(device_handle, &prop_list, &num_props);
        if (ret == CrError_None) {
            for (int i = 0; i < num_props; i++) {
                if (prop_list[i].GetCode() == CrDeviceProperty_RecorderMainStatus) {
                    std::cout << "Recording status after start: " << std::hex << prop_list[i].GetCurrentValue() << std::endl;
                    if (prop_list[i].GetCurrentValue() != 0x00000001) { // Not recording
                        std::cout << "Recording may not have started - status is not 0x00000001" << std::endl;
                    }
                    break;
                }
            }
            SCRSDK::ReleaseDeviceProperties(device_handle, prop_list);
        }

        // Record for 5 seconds
        std::cout << "Recording for 5 seconds..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(5));

        // Stop recording using CrCommandId_MovieRecord with CrCommandParam_Up
        std::cout << "Stopping recording..." << std::endl;
        ret = SCRSDK::SendCommand(device_handle, CrCommandId_MovieRecord, CrCommandParam_Up);
        if (ret == CrError_None) {
            std::cout << "Record stop command sent successfully!" << std::endl;
        } else {
            std::cerr << "Failed to stop recording: " << std::hex << ret << std::endl;
        }
        
    } else {
        std::cerr << "Failed to start recording: " << std::hex << ret << std::endl;
    }

    // Disconnect and cleanup
    SCRSDK::Disconnect(device_handle);
    camera_list->Release();
    SCRSDK::Release();
    
    return 0;
}