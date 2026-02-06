#include <iostream>
#include <vector>
#include <memory>
#include "src/camera_controller.hpp"

int main() {
    std::cout << "🎬 Sony Camera Control Test" << std::endl;
    std::cout << "===========================" << std::endl;

    // Initialize Sony CRSDK
    if (!SCRSDK::Init()) {
        std::cerr << "❌ Failed to initialize Sony CRSDK" << std::endl;
        return 1;
    }

    // Create camera controllers
    std::vector<std::unique_ptr<CameraController>> cameras;
    
    // Example: Connect to multiple cameras
    // Camera 1 - Main FX6
    auto cam1 = std::make_unique<CameraController>("CAM1-MAIN-FX6");
    if (cam1->connectViaIP("192.168.33.91", "50:26:EF:B8:3F:2C", 
                          SCRSDK::CrCameraDeviceModelList::CrCameraDeviceModel_ILME_FX6)) {
        std::cout << "✅ Camera 1 connected" << std::endl;
        cameras.push_back(std::move(cam1));
    }
    
    // Camera 2 - B-Cam FX6
    auto cam2 = std::make_unique<CameraController>("CAM2-BCAM-FX6");
    if (cam2->connectViaIP("192.168.33.92", "", 
                          SCRSDK::CrCameraDeviceModelList::CrCameraDeviceModel_ILME_FX6)) {
        std::cout << "✅ Camera 2 connected" << std::endl;
        cameras.push_back(std::move(cam2));
    }
    
    // Camera 3 - Wide FX3
    auto cam3 = std::make_unique<CameraController>("CAM3-WIDE-FX3");
    if (cam3->connectViaIP("192.168.33.93", "",
                          SCRSDK::CrCameraDeviceModelList::CrCameraDeviceModel_ILME_FX3)) {
        std::cout << "✅ Camera 3 connected" << std::endl;
        cameras.push_back(std::move(cam3));
    }

    if (cameras.empty()) {
        std::cout << "❌ No cameras connected. Exiting." << std::endl;
        SCRSDK::Release();
        return 1;
    }

    std::cout << "\n🎯 Connected to " << cameras.size() << " camera(s)" << std::endl;

    // Interactive command menu
    std::string command;
    while (true) {
        std::cout << "\n🎬 Camera Control Menu:" << std::endl;
        std::cout << "  record_start  - Start recording on all cameras" << std::endl;
        std::cout << "  record_stop   - Stop recording on all cameras" << std::endl;
        std::cout << "  photo         - Take photo on camera 1" << std::endl;
        std::cout << "  iso <value>   - Set ISO on all cameras (100-25600)" << std::endl;
        std::cout << "  wb <type>     - Set white balance (Auto/Daylight/Tungsten/etc)" << std::endl;
        std::cout << "  fps <rate>    - Set frame rate (24p/30p/60p/120p)" << std::endl;
        std::cout << "  status        - Show camera status" << std::endl;
        std::cout << "  quit          - Exit program" << std::endl;
        std::cout << "\nEnter command: ";
        
        std::getline(std::cin, command);
        
        if (command == "quit" || command == "exit") {
            break;
        }
        else if (command == "record_start" || command == "rec") {
            std::vector<CameraController*> cam_ptrs;
            for (auto& cam : cameras) {
                cam_ptrs.push_back(cam.get());
            }
            CameraController::syncRecordStart(cam_ptrs);
        }
        else if (command == "record_stop" || command == "stop") {
            std::vector<CameraController*> cam_ptrs;
            for (auto& cam : cameras) {
                cam_ptrs.push_back(cam.get());
            }
            CameraController::syncRecordStop(cam_ptrs);
        }
        else if (command == "photo" || command == "capture") {
            if (!cameras.empty()) {
                cameras[0]->capturePhoto();
            }
        }
        else if (command.substr(0, 3) == "iso" && command.length() > 4) {
            int iso = std::stoi(command.substr(4));
            std::vector<CameraController*> cam_ptrs;
            for (auto& cam : cameras) {
                cam_ptrs.push_back(cam.get());
            }
            CameraController::setAllISO(cam_ptrs, iso);
        }
        else if (command.substr(0, 2) == "wb" && command.length() > 3) {
            std::string wb = command.substr(3);
            for (auto& cam : cameras) {
                cam->setWhiteBalance(wb);
            }
        }
        else if (command.substr(0, 3) == "fps" && command.length() > 4) {
            std::string fps = command.substr(4);
            for (auto& cam : cameras) {
                cam->setFrameRate(fps);
            }
        }
        else if (command == "status") {
            for (size_t i = 0; i < cameras.size(); ++i) {
                std::cout << "Camera " << (i+1) << " status: Connected" << std::endl;
            }
        }
        else {
            std::cout << "❓ Unknown command. Try 'record_start', 'record_stop', 'iso 800', etc." << std::endl;
        }
    }

    std::cout << "\n👋 Disconnecting cameras..." << std::endl;
    cameras.clear();
    
    SCRSDK::Release();
    std::cout << "✅ Camera control session ended" << std::endl;
    
    return 0;
}