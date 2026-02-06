#include <iostream>
#include <thread>
#include <chrono>
#include <sstream>
#include <vector>
#include <string>
#include <algorithm>
#include <cstring>
#include <map>
#include "camera_controller.hpp"
#include "CameraRemote_SDK.h"
#include "sony_sample/ConnectionInfo.h"

using namespace std;
using namespace SCRSDK;

class CameraControlApp {
private:
    CameraController controller;
    vector<CrCameraDeviceInfo> cameras;
    bool isInitialized = false;
    bool isConnected = false;

    // Camera model mapping for direct IP connection
    map<string, CrDeviceModel> modelMap = {
        {"FX6", CrDeviceModel_FX6},
        {"FX3", CrDeviceModel_FX3},
        {"A7R4", CrDeviceModel_A7R4},
        {"A74", CrDeviceModel_A74},
        {"UNKNOWN", CrDeviceModel_UnknownModel}
    };

    string determineModelString(int deviceModelCode) {
        for (const auto& pair : modelMap) {
            if (static_cast<int>(pair.second) == deviceModelCode) {
                return pair.first;
            }
        }
        return "UNKNOWN";
    }

public:
    bool initialize() {
        CrError ret = CrSDK::Init();
        if (CrError_None == ret) {
            isInitialized = true;
            cout << "✅ Camera controller initialized\n";
            return true;
        }
        cout << "❌ Failed to initialize camera controller\n";
        return false;
    }

    bool connectFlexible() {
        if (!isInitialized) {
            cout << "❌ Controller not initialized\n";
            return false;
        }

        cout << "🔍 Attempting flexible camera connections...\n";
        
        // Try to connect using our flexible connection system
        vector<string> cameraIPs = {
            "192.168.1.110", // Camera 1
            "192.168.1.111", // Camera 2  
            "192.168.1.112", // Camera 3
            "192.168.1.113"  // Camera 4
        };

        bool anyConnected = false;
        for (const string& ip : cameraIPs) {
            cout << "🔌 Trying to connect to camera at " << ip << "\n";
            
            // Try direct Ethernet connection first
            CrCameraDeviceInfo deviceInfo = {};
            CrConnectionInfo connectionInfo = {};
            
            // Set up Ethernet connection  
            connectionInfo.connectionType = CrConnectionType_Ethernet;
            strncpy(connectionInfo.deviceAddress, ip.c_str(), sizeof(connectionInfo.deviceAddress) - 1);
            connectionInfo.deviceAddress[sizeof(connectionInfo.deviceAddress) - 1] = '\0';
            
            // Set device info
            deviceInfo.connectionInfo = connectionInfo;
            deviceInfo.deviceModelCode = static_cast<int>(CrDeviceModel_FX6);  // Default to FX6
            snprintf(deviceInfo.deviceName, sizeof(deviceInfo.deviceName), "Sony Camera (%s)", ip.c_str());
            
            // Try to connect with this device info
            CrDeviceHandle deviceHandle = nullptr;
            CrError result = CrSDK::CreateCameraObjectInfo(&deviceInfo, &deviceHandle);
            
            if (CrError_None == result && deviceHandle) {
                cout << "✅ Connected to camera at " << ip << "\n";
                cameras.push_back(deviceInfo);
                anyConnected = true;
            } else {
                cout << "⚠️  Failed to connect to " << ip << "\n";
            }
        }
        
        if (anyConnected) {
            isConnected = true;
            cout << "🎯 Connected to " << cameras.size() << " camera(s)\n";
            return true;
        }
        
        cout << "❌ No cameras connected\n";
        return false;
    }

    void showMenu() {
        cout << "\n🎬 Camera Control Menu:\n";
        cout << "1. record_start   - Start recording on all cameras\n";
        cout << "2. record_stop    - Stop recording on all cameras\n";
        cout << "3. photo          - Capture photo on main camera\n";
        cout << "4. iso <value>    - Set ISO (50-25600)\n";
        cout << "5. wb <type>      - Set white balance (Auto/Daylight/Tungsten/etc)\n";
        cout << "6. fps <rate>     - Set frame rate (24p/30p/60p/120p)\n";
        cout << "7. status         - Show camera status\n";
        cout << "8. sync_record    - Synchronized recording start\n";
        cout << "9. help           - Show this menu\n";
        cout << "0. quit           - Exit application\n";
        cout << ">> ";
    }

    void processCommand(const string& input) {
        istringstream iss(input);
        string command;
        iss >> command;
        
        // Convert to lowercase for easier matching
        transform(command.begin(), command.end(), command.begin(), ::tolower);
        
        if (command == "record_start" || command == "1") {
            cout << "🔴 Starting recording on all cameras...\n";
            if (controller.startRecording()) {
                cout << "✅ Recording started successfully\n";
            } else {
                cout << "❌ Failed to start recording\n";
            }
        }
        else if (command == "record_stop" || command == "2") {
            cout << "⏹️  Stopping recording on all cameras...\n";
            if (controller.stopRecording()) {
                cout << "✅ Recording stopped successfully\n";
            } else {
                cout << "❌ Failed to stop recording\n";
            }
        }
        else if (command == "photo" || command == "3") {
            cout << "📸 Capturing photo...\n";
            if (controller.capturePhoto()) {
                cout << "✅ Photo captured successfully\n";
            } else {
                cout << "❌ Failed to capture photo\n";
            }
        }
        else if (command == "iso" || command == "4") {
            int isoValue;
            if (iss >> isoValue) {
                cout << "🎚️  Setting ISO to " << isoValue << "...\n";
                if (controller.setISO(isoValue)) {
                    cout << "✅ ISO set successfully\n";
                } else {
                    cout << "❌ Failed to set ISO\n";
                }
            } else {
                cout << "❌ Invalid ISO value. Usage: iso 800\n";
            }
        }
        else if (command == "wb" || command == "5") {
            string wbType;
            if (iss >> wbType) {
                cout << "⚪ Setting white balance to " << wbType << "...\n";
                if (controller.setWhiteBalance(wbType)) {
                    cout << "✅ White balance set successfully\n";
                } else {
                    cout << "❌ Failed to set white balance\n";
                }
            } else {
                cout << "❌ Invalid white balance. Usage: wb Daylight\n";
            }
        }
        else if (command == "fps" || command == "6") {
            string fpsRate;
            if (iss >> fpsRate) {
                cout << "🎬 Setting frame rate to " << fpsRate << "...\n";
                if (controller.setFrameRate(fpsRate)) {
                    cout << "✅ Frame rate set successfully\n";
                } else {
                    cout << "❌ Failed to set frame rate\n";
                }
            } else {
                cout << "❌ Invalid frame rate. Usage: fps 24p\n";
            }
        }
        else if (command == "status" || command == "7") {
            cout << "📊 Camera Status:\n";
            cout << "  Connected cameras: " << cameras.size() << "\n";
            cout << "  Controller status: " << (isConnected ? "Connected" : "Disconnected") << "\n";
            
            // Get status from controller
            auto status = controller.getStatus();
            cout << "  Recording state: " << (status.isRecording ? "Recording" : "Stopped") << "\n";
            cout << "  Last command: " << status.lastCommand << "\n";
        }
        else if (command == "sync_record" || command == "8") {
            cout << "🎯 Starting synchronized recording...\n";
            if (controller.syncRecordStart()) {
                cout << "✅ Synchronized recording started\n";
            } else {
                cout << "❌ Failed to start synchronized recording\n";
            }
        }
        else if (command == "help" || command == "9") {
            showMenu();
        }
        else if (command == "quit" || command == "0" || command == "exit") {
            cout << "👋 Goodbye!\n";
            exit(0);
        }
        else {
            cout << "❓ Unknown command: " << command << "\n";
            cout << "Type 'help' for available commands\n";
        }
    }

    void run() {
        cout << "🎬 Sony Camera Control Test Application\n";
        cout << "=====================================\n\n";
        
        if (!initialize()) {
            return;
        }
        
        if (!connectFlexible()) {
            cout << "❌ No cameras connected. Exiting.\n";
            return;
        }
        
        showMenu();
        
        string input;
        while (getline(cin, input)) {
            if (!input.empty()) {
                processCommand(input);
            }
            cout << ">> ";
        }
    }
};

int main() {
    try {
        CameraControlApp app;
        app.run();
    }
    catch (const exception& e) {
        cerr << "❌ Application error: " << e.what() << endl;
        return 1;
    }
    
    return 0;
}