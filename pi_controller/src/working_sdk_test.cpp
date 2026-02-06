#include "CRSDK/CameraRemote_SDK.h"
#include "OpenCVWrapper.h"
#include <iostream>
#include <memory>

int main() {
    std::cout << "Testing Sony SDK with OpenCV wrapper..." << std::endl;
    
    // Initialize SDK exactly like RemoteCli
    std::cout << "Initialize Remote SDK..." << std::endl;
    const bool init_ok = SCRSDK::Init();
    
    if (!init_ok) {
        std::cout << "Failed to initialize SDK (Init returned false)." << std::endl;
        return -1;
    }
    
    std::cout << "Remote SDK successfully initialized." << std::endl;
    
    // Enumerate cameras exactly like RemoteCli
    std::cout << "Enumerate connected camera devices..." << std::endl;
    SCRSDK::ICrEnumCameraObjectInfo* camera_list = nullptr;
    auto enum_result = SCRSDK::EnumCameraObjects(&camera_list);
    
    if (enum_result != SCRSDK::CrError_None) {
        std::cout << "Failed to enumerate cameras with error: " << static_cast<int>(enum_result) << std::endl;
        SCRSDK::Release();
        return -1;
    }
    
    if (camera_list) {
        auto count = camera_list->GetCount();
        if (count == 0) {
            std::cout << "No cameras detected. Connect a camera and retry." << std::endl;
        } else {
            std::cout << "Camera enumeration successful. " << count << " detected." << std::endl;
            
            // Look for the Sony A74 (ILCE-7M4)
            for (int i = 0; i < count; ++i) {
                auto info = camera_list->GetCameraObjectInfo(i);
                if (info) {
                    std::cout << "[" << (i+1) << "] " << info->GetModel() << std::endl;
                    
                    // Check if this is the Sony A74
                    if (std::string(info->GetModel()) == "ILCE-7M4") {
                        std::cout << "Found Sony A74! Ready for recording tests." << std::endl;
                    }
                }
            }
        }
        camera_list->Release();
    } else {
        std::cout << "Camera list is null" << std::endl;
    }
    
    // Cleanup
    SCRSDK::Release();
    std::cout << "SDK cleanup completed." << std::endl;
    
    return 0;
}