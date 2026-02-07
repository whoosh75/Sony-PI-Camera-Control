#include "CameraRemote_SDK.h"
#include "CRSDK/IDeviceCallback.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>

using namespace SCRSDK;

class MinimalCallback : public IDeviceCallback {
public:
    std::atomic<bool> connected{false};

    void OnConnected(DeviceConnectionVersioin) override {
        connected.store(true);
    }

    void OnWarning(CrInt32u warning) override {
        if (warning == CrWarning_MovieRecordingOperation_Result_OK) {
            std::cout << "✅ OnWarning: MovieRecordingOperation_Result_OK" << std::endl;
        } else {
            std::cout << "ℹ️  OnWarning: 0x" << std::hex << warning << std::dec << std::endl;
        }
    }
};

void print_recording_status(CrDeviceHandle device_handle) {
    CrDeviceProperty* prop_list = nullptr;
    CrInt32 num_props = 0;

    auto ret = SCRSDK::GetDeviceProperties(device_handle, &prop_list, &num_props);
    if (ret == CrError_None) {
        bool found_recorder = false;
        bool found_recording_state = false;
        bool found_toggle = false;

        for (int i = 0; i < num_props; i++) {
            if (prop_list[i].GetCode() == CrDeviceProperty_RecorderMainStatus) {
                std::cout << "📹 Recorder Status: " << std::hex << prop_list[i].GetCurrentValue()
                          << (prop_list[i].GetCurrentValue() == 0x00000001 ? " (Recording)" : " (Not Recording)") << std::endl;
                found_recorder = true;
            }
            else if (prop_list[i].GetCode() == CrDeviceProperty_RecordingState) {
                std::cout << "🎥 Recording State: " << std::hex << prop_list[i].GetCurrentValue()
                          << (prop_list[i].GetCurrentValue() == 0x00000001 ? " (Recording)" : " (Not Recording)") << std::endl;
                found_recording_state = true;
            }
            else if (prop_list[i].GetCode() == CrDeviceProperty_MovieRecButtonToggleEnableStatus) {
                std::cout << "🔘 Movie Rec Button Toggle: " << std::hex << prop_list[i].GetCurrentValue()
                          << (prop_list[i].GetCurrentValue() == CrMovieRecButtonToggle_Enable ? " (Enabled)" : " (Disabled)") << std::endl;
                found_toggle = true;
            }
        }

        if (!found_recorder) std::cout << "❌ RecorderMainStatus property not found" << std::endl;
        if (!found_recording_state) std::cout << "❌ RecordingState property not found" << std::endl;
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
    const bool init_ok = SCRSDK::Init();
    if (!init_ok) {
        std::cerr << "❌ Failed to initialize SDK" << std::endl;
        return -1;
    }
    std::cout << "✅ Sony SDK initialized\n";

    // Wait a bit for USB devices to be detected
    std::cout << "⏳ Waiting for USB camera detection...\n";
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // Enumerate cameras
    ICrEnumCameraObjectInfo* camera_list = nullptr;
    auto ret = SCRSDK::EnumCameraObjects(&camera_list);
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
    MinimalCallback callback;

    std::cout << "🔗 Connecting to Sony A74...\n";
    ret = SCRSDK::Connect(const_cast<ICrCameraObjectInfo*>(camera_info), &callback, &device_handle);
    if (ret != CrError_None) {
        std::cerr << "❌ Failed to connect to camera: " << std::hex << ret << std::endl;
        camera_list->Release();
        SCRSDK::Release();
        return -1;
    }

    std::cout << "✅ Connected to Sony A74!\n\n";

    // Wait for OnConnected callback
    for (int i = 0; i < 20 && !callback.connected.load(); ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Check initial status
    std::cout << "📊 Initial Status:\n";
    print_recording_status(device_handle);
    std::cout << "\n";

    // Test direct recording using toggle first, then MovieRecord
    std::cout << "🎬 Testing Direct Recording Commands...\n";

    auto try_record_cmd = [&](CrCommandId cmd, const char* name) -> bool {
        std::cout << "▶️  Starting recording with " << name << "...\n";
        ret = SCRSDK::SendCommand(device_handle, cmd, CrCommandParam_Down);
        if (ret != CrError_None) {
            std::cerr << "❌ Failed to start recording (" << name << "): " << std::hex << ret << std::endl;
            return false;
        }

        SCRSDK::SendCommand(device_handle, cmd, CrCommandParam_Up);
        std::cout << "✅ Start command sent\n";

        std::this_thread::sleep_for(std::chrono::milliseconds(1500));
        std::cout << "📊 Status after start command:\n";
        print_recording_status(device_handle);

        std::cout << "\n⏱️  Recording for 3 seconds...\n";
        std::this_thread::sleep_for(std::chrono::seconds(3));

        std::cout << "⏹️  Stopping recording with " << name << "...\n";
        ret = SCRSDK::SendCommand(device_handle, cmd, CrCommandParam_Down);
        if (ret != CrError_None) {
            std::cerr << "❌ Failed to stop recording (" << name << "): " << std::hex << ret << std::endl;
            return false;
        }

        SCRSDK::SendCommand(device_handle, cmd, CrCommandParam_Up);
        std::cout << "✅ Stop command sent\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        std::cout << "📊 Final Status:\n";
        print_recording_status(device_handle);
        return true;
    };

    if (try_record_cmd(CrCommandId_MovieRecord, "CrCommandId_MovieRecord")) {
        std::cout << "🎉 Success with MovieRecord\n";
    } else {
        std::cerr << "❌ MovieRecord failed\n";
        std::cerr << "   This could be due to:\n";
        std::cerr << "   - Camera not in movie mode\n";
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