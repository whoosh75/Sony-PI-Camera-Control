#include "CameraRemote_SDK.h" 
#include <iostream>

using namespace SCRSDK;

int main() {
    std::cout << "Testing minimal SDK initialization..." << std::endl;
    
    auto ret = SCRSDK::Init();
    std::cout << "SDK Init result: " << std::hex << ret << std::endl;
    
    if (ret == CrError_None) {
        std::cout << "✅ SDK initialized successfully!" << std::endl;
        
        // Try enumerating cameras
        ICrEnumCameraObjectInfo* camera_list = nullptr;
        ret = SCRSDK::EnumCameraObjects(&camera_list);
        std::cout << "EnumCameraObjects result: " << std::hex << ret << std::endl;
        
        if (ret == CrError_None && camera_list) {
            std::cout << "✅ Found " << camera_list->GetCount() << " camera(s)" << std::endl;
            camera_list->Release();
        } else {
            std::cout << "❌ EnumCameraObjects failed" << std::endl;
        }
        
        SCRSDK::Release();
    } else {
        std::cout << "❌ SDK initialization failed" << std::endl;
        return -1;
    }
    
    return 0;
}