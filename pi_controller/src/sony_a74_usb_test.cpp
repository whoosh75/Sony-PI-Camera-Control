#include "CRSDK/CameraRemote_SDK.h"
#include <iostream>
#include <vector>
#include <thread>
#include <chrono>

int main() {
    std::cout << "=== Sony A74 USB Connection Test ===\n\n";
    
    // Initialize SDK
    const bool init_ok = SCRSDK::Init();
    if (!init_ok) {
        std::cout << "Failed to initialize SDK" << std::endl;
        return -1;
    }
    std::cout << "✓ SDK initialized successfully\n";
    
    // Enumerate USB connected cameras
    SCRSDK::ICrEnumCameraObjectInfo* camera_list = nullptr;
    auto enum_result = SCRSDK::EnumCameraObjects(&camera_list);
    if (CR_FAILED(enum_result) || !camera_list) {
        std::cout << "Failed to enumerate cameras: " << std::hex << enum_result << std::endl;
        SCRSDK::Release();
        return -1;
    }
    
    auto camera_count = camera_list->GetCount();
    std::cout << "Found " << camera_count << " camera(s)\n";
    
    if (camera_count == 0) {
        std::cout << "\nNo cameras detected. Please check:\n";
        std::cout << "1. Sony A74 is connected via USB cable\n";
        std::cout << "2. Camera is powered on\n";
        std::cout << "3. USB Connection Mode is set to 'Remote Shooting' or 'PC Remote'\n";
        std::cout << "4. USB drivers are installed (Windows only)\n";
        camera_list->Release();
        SCRSDK::Release();
        return -1;
    }
    
    // Display camera information
    for (int i = 0; i < camera_count; ++i) {
        auto camera_info = camera_list->GetCameraObjectInfo(i);
        if (camera_info) {
            std::cout << "\nCamera " << (i + 1) << ":\n";
            
            auto model_ptr = camera_info->GetModel();
            if (model_ptr) {
                std::cout << "  Model: " << model_ptr << std::endl;
            }
            
            auto conn_type_ptr = camera_info->GetConnectionTypeName();
            if (conn_type_ptr) {
                std::cout << "  Connection Type: " << conn_type_ptr << std::endl;
                
                std::string conn_type(conn_type_ptr);
                if (conn_type.find("USB") != std::string::npos || conn_type.find("usb") != std::string::npos) {
                    std::cout << "  ✓ USB Connection detected\n";
                }
            }
        }
    }
    
    // Test connection to first camera
    if (camera_count > 0) {
        std::cout << "\n=== Testing Connection to Camera 1 ===\n";
        
        auto camera_info = camera_list->GetCameraObjectInfo(0);
        SCRSDK::ICrCameraDevice* camera = nullptr;
        
        auto connect_result = SCRSDK::Connect(camera_info, &camera);
        if (CR_FAILED(connect_result) || !camera) {
            std::cout << "Failed to connect to camera: " << std::hex << connect_result << std::endl;
        } else {
            std::cout << "✓ Successfully connected to Sony A74\n";
            
            // Test getting device properties
            std::cout << "\n=== Testing Device Properties ===\n";
            
            // Check MovieRecButtonToggleEnableStatus
            SCRSDK::CrDeviceProperty prop;
            prop.SetCode(SCRSDK::CrDevicePropertyCode::CrDeviceProperty_MovieRecButtonToggleEnableStatus);
            
            auto prop_result = camera->GetDeviceProperty(&prop);
            if (CR_SUCCEEDED(prop_result)) {
                auto* values = static_cast<CrInt8u*>(prop.GetValue());
                if (values && prop.GetValueSize() > 0) {
                    std::cout << "Movie Rec Button Toggle Status: ";
                    if (values[0] == SCRSDK::CrMovieRecButtonToggle::CrMovieRecButtonToggle_Enable) {
                        std::cout << "ENABLED ✓\n";
                    } else if (values[0] == SCRSDK::CrMovieRecButtonToggle::CrMovieRecButtonToggle_Disable) {
                        std::cout << "DISABLED (This is why recording failed!)\n";
                        
                        // Try to enable it
                        std::cout << "Attempting to enable Movie Rec Button Toggle...\n";
                        SCRSDK::CrDeviceProperty enable_prop;
                        enable_prop.SetCode(SCRSDK::CrDevicePropertyCode::CrDeviceProperty_MovieRecButtonToggleEnableStatus);
                        CrInt8u enable_value = SCRSDK::CrMovieRecButtonToggle::CrMovieRecButtonToggle_Enable;
                        enable_prop.SetValue(&enable_value);
                        enable_prop.SetValueSize(sizeof(enable_value));
                        
                        auto set_result = camera->SetDeviceProperty(&enable_prop);
                        if (CR_SUCCEEDED(set_result)) {
                            std::cout << "✓ Movie Rec Button Toggle enabled!\n";
                        } else {
                            std::cout << "Failed to enable Movie Rec Button Toggle: " << std::hex << set_result << std::endl;
                            std::cout << "This property may be read-only on this camera model.\n";
                        }
                    } else {
                        std::cout << "UNKNOWN VALUE (" << (int)values[0] << ")\n";
                    }
                }
            } else {
                std::cout << "Failed to get Movie Rec Button Toggle status: " << std::hex << prop_result << std::endl;
            }
            
            // Check Recording State
            prop.SetCode(SCRSDK::CrDevicePropertyCode::CrDeviceProperty_RecordingState);
            prop_result = camera->GetDeviceProperty(&prop);
            if (CR_SUCCEEDED(prop_result)) {
                auto* values = static_cast<CrInt8u*>(prop.GetValue());
                if (values && prop.GetValueSize() > 0) {
                    std::cout << "Recording State: ";
                    if (values[0] == SCRSDK::CrRecordingState::CrRecordingState_Recording) {
                        std::cout << "RECORDING\n";
                    } else if (values[0] == SCRSDK::CrRecordingState::CrRecordingState_NotRecording) {
                        std::cout << "NOT RECORDING\n";
                    } else {
                        std::cout << "UNKNOWN (" << (int)values[0] << ")\n";
                    }
                }
            } else {
                std::cout << "Could not get Recording State (property may not be supported)\n";
            }
            
            // Test Movie Record Command
            std::cout << "\n=== Testing Movie Record Command ===\n";
            std::cout << "Sending movie record command...\n";
            auto record_result = camera->SendCommand(SCRSDK::CrCommandId::CrCommandId_MovieRecord);
            if (CR_SUCCEEDED(record_result)) {
                std::cout << "✓ Movie record command sent successfully\n";
                std::cout << "Check your camera - it should now be recording!\n";
                
                // Wait a bit for the command to take effect
                std::this_thread::sleep_for(std::chrono::seconds(2));
                
                // Check recording state again
                prop.SetCode(SCRSDK::CrDevicePropertyCode::CrDeviceProperty_RecordingState);
                prop_result = camera->GetDeviceProperty(&prop);
                if (CR_SUCCEEDED(prop_result)) {
                    auto* values = static_cast<CrInt8u*>(prop.GetValue());
                    if (values && prop.GetValueSize() > 0) {
                        std::cout << "Recording State after command: ";
                        if (values[0] == SCRSDK::CrRecordingState::CrRecordingState_Recording) {
                            std::cout << "RECORDING ✓ SUCCESS!\n";
                        } else {
                            std::cout << "NOT RECORDING (command may have failed)\n";
                        }
                    }
                }
                
                // Stop recording after 5 seconds
                std::cout << "Recording for 5 seconds...\n";
                std::this_thread::sleep_for(std::chrono::seconds(5));
                
                std::cout << "Sending stop recording command...\n";
                auto stop_result = camera->SendCommand(SCRSDK::CrCommandId::CrCommandId_MovieRecord);
                if (CR_SUCCEEDED(stop_result)) {
                    std::cout << "✓ Movie stop command sent successfully\n";
                } else {
                    std::cout << "Failed to send stop command: " << std::hex << stop_result << std::endl;
                }
                
            } else {
                std::cout << "Failed to send movie record command: " << std::hex << record_result << std::endl;
                std::cout << "Error code meaning: ";
                
                // Provide some common error explanations
                switch (record_result) {
                    case 0x80004005:
                        std::cout << "E_FAIL - Command failed, possibly due to camera state\n";
                        break;
                    case 0x80070057:
                        std::cout << "E_INVALIDARG - Invalid argument\n";
                        break;
                    default:
                        std::cout << "Unknown error code\n";
                        break;
                }
            }
            
            // Disconnect
            SCRSDK::Disconnect(&camera);
            std::cout << "\n✓ Disconnected from camera\n";
        }
    }
    
    // Cleanup
    camera_list->Release();
    SCRSDK::Release();
    
    std::cout << "\n=== Test Complete ===\n";
    std::cout << "\nIf recording didn't work, try:\n";
    std::cout << "1. Set camera to Movie mode manually\n";
    std::cout << "2. Check camera menu settings for remote recording permission\n";
    std::cout << "3. Ensure sufficient memory card space\n";
    
    return 0;
}