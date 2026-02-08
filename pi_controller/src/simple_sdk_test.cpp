#include "CRSDK/CameraRemote_SDK.h"
#include <iostream>

int main() {
    std::cout << "Testing Sony SDK initialization..." << std::endl;
    
    // Initialize SDK
    const bool init_ok = SCRSDK::Init();
    std::cout << "SDK Init result: " << (init_ok ? "true" : "false") << std::endl;
    
    if (!init_ok) {
        std::cout << "Failed to initialize SDK (Init returned false)." << std::endl;
        return -1;
    }
    
    std::cout << "SDK initialized successfully!" << std::endl;
    
    // Get device list
    SCRSDK::ICrEnumCameraObjectInfo* camera_list = nullptr;
    auto enum_result = SCRSDK::EnumCameraObjects(&camera_list);
    
    if (enum_result != SCRSDK::CrError_None) {
        std::cout << "Failed to enumerate cameras with error: " << static_cast<int>(enum_result) << std::endl;
        SCRSDK::Release();
        return -1;
    }
    
    if (camera_list) {
        auto count = camera_list->GetCount();
        std::cout << "Found " << count << " camera(s):" << std::endl;
        
        for (int i = 0; i < count; ++i) {
            auto info = camera_list->GetCameraObjectInfo(i);
            if (info) {
                std::cout << "  [" << (i+1) << "] " << info->GetModel() << std::endl;
            }
        }
        camera_list->Release();
    }
    
    // Cleanup
    SCRSDK::Release();
    std::cout << "SDK cleanup completed." << std::endl;
    
    return 0;
}