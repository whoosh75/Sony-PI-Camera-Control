#include <iostream>
#include <memory>
#include <thread>
#include <chrono>

#include "src/sony_backend.hpp"
#include "src/protocol.hpp"

int main() {
    std::cout << "Testing Sony camera connection..." << std::endl;

    // Initialize Sony backend 
    sony_backend backend;

    std::cout << "Attempting to connect to camera..." << std::endl;
    bool success = backend.connect_first_camera();
    
    if (success) {
        std::cout << "SUCCESS: Camera connected!" << std::endl;
        
        // Try to get camera status
        camera_state state = backend.get_camera_state();
        std::cout << "Camera Status:" << std::endl;
        std::cout << "  Aperture: " << state.aperture << std::endl;
        std::cout << "  Shutter Speed: " << state.shutter_speed << std::endl;
        std::cout << "  ISO: " << state.iso << std::endl;
        
    } else {
        std::cout << "FAILED: Could not connect to camera" << std::endl;
    }
    
    return success ? 0 : 1;
}