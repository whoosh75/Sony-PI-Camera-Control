#include <iostream>
#include <thread>
#include <chrono>
#include <sstream>
#include <string>
#include <algorithm>
#include "camera_controller.hpp"

using namespace std;

static const char* recording_state_name(RecordingState st) {
    switch (st) {
        case RecordingState::STOPPED: return "STOPPED";
        case RecordingState::RECORDING: return "RECORDING";
        case RecordingState::PAUSED: return "PAUSED";
        case RecordingState::ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

class CameraControlApp {
private:
    CameraController controller;
    bool connected = false;

public:
    bool connectUSB() {
        cout << "🔌 Connecting via USB...\n";
        connected = controller.connectViaUSB();
        cout << (connected ? "✅ USB connected\n" : "❌ USB connect failed\n");
        return connected;
    }

    void showMenu() {
        cout << "\n🎬 Camera Control Menu:\n";
        cout << "1. record_start   - Start recording\n";
        cout << "2. record_stop    - Stop recording\n";
        cout << "3. photo          - Capture photo\n";
        cout << "4. iso <value>    - Set ISO (e.g., 800)\n";
        cout << "5. wb <type>      - Set white balance (Auto/Daylight/Tungsten/etc)\n";
        cout << "6. fps <rate>     - Set frame rate (24p/30p/60p/120p)\n";
        cout << "7. status         - Show camera status\n";
        cout << "0. quit           - Exit application\n";
        cout << ">> ";
    }

    void processCommand(const string& input) {
        istringstream iss(input);
        string command;
        iss >> command;
        transform(command.begin(), command.end(), command.begin(), ::tolower);

        if (command == "record_start" || command == "1") {
            cout << (controller.startRecording() ? "✅ Recording started\n" : "❌ Start failed\n");
        } else if (command == "record_stop" || command == "2") {
            cout << (controller.stopRecording() ? "✅ Recording stopped\n" : "❌ Stop failed\n");
        } else if (command == "photo" || command == "3") {
            cout << (controller.capturePhoto() ? "✅ Photo captured\n" : "❌ Photo failed\n");
        } else if (command == "iso" || command == "4") {
            int isoValue;
            if (iss >> isoValue) {
                cout << (controller.setISO(isoValue) ? "✅ ISO set\n" : "❌ ISO failed\n");
            } else {
                cout << "❌ Usage: iso 800\n";
            }
        } else if (command == "wb" || command == "5") {
            string wbType;
            if (iss >> wbType) {
                cout << (controller.setWhiteBalance(wbType) ? "✅ WB set\n" : "❌ WB failed\n");
            } else {
                cout << "❌ Usage: wb Daylight\n";
            }
        } else if (command == "fps" || command == "6") {
            string fpsRate;
            if (iss >> fpsRate) {
                cout << (controller.setFrameRate(fpsRate) ? "✅ FPS set\n" : "❌ FPS failed\n");
            } else {
                cout << "❌ Usage: fps 24p\n";
            }
        } else if (command == "status" || command == "7") {
            auto status = controller.getCameraStatus();
            cout << "Recording: " << recording_state_name(status.recording_state) << "\n";
            cout << "ISO: " << status.iso_value << " WB: " << status.white_balance << " FPS: " << status.frame_rate << "\n";
            cout << "Battery: " << status.battery_level << "%\n";
        } else if (command == "quit" || command == "0" || command == "exit") {
            cout << "👋 Goodbye!\n";
            exit(0);
        } else if (command == "help" || command == "9") {
            showMenu();
        } else {
            cout << "❓ Unknown command. Type 'help'.\n";
        }
    }

    void run() {
        cout << "🎬 Sony Camera Control Test Application\n";
        cout << "=====================================\n\n";
        if (!connectUSB()) return;
        showMenu();
        string input;
        while (getline(cin, input)) {
            if (!input.empty()) processCommand(input);
            cout << ">> ";
        }
    }
};

int main() {
    CameraControlApp app;
    app.run();
    return 0;
}