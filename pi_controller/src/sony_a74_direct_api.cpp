#include "CRSDK/CameraRemote_SDK.h"
#include "CRSDK/IDeviceCallback.h"
#include "OpenCVWrapper.h"
#include <iostream>
#include <vector>
#include <thread>
#include <chrono>

class DeviceCallback : public SCRSDK::IDeviceCallback {
public:
    void OnConnected(SCRSDK::DeviceConnectionVersioin) override {}
    void OnDisconnected(CrInt32u) override {}
    void OnPropertyChanged() override {}
    void OnLvPropertyChanged() override {}
    void OnCompleteDownload(CrChar*, CrInt32u) override {}
    void OnNotifyContentsTransfer(CrInt32u, SCRSDK::CrContentHandle, CrChar*) override {}
    void OnNotifyFTPTransferResult(CrInt32u, CrInt32u, CrInt32u) override {}
    void OnNotifyRemoteTransferResult(CrInt32u, CrInt32u, CrChar*) override {}
    void OnNotifyRemoteTransferResult(CrInt32u, CrInt32u, CrInt8u*, CrInt64u) override {}
    void OnNotifyRemoteTransferContentsListChanged(CrInt32u, CrInt32u, CrInt32u) override {}
    void OnReceivePlaybackTimeCode(CrInt32u) override {}
    void OnReceivePlaybackData(CrInt8u, CrInt32, CrInt8u*, CrInt64, CrInt64, CrInt32, CrInt32) override {}
    void OnNotifyRemoteFirmwareUpdateResult(CrInt32u, const void*) override {}
    void OnWarning(CrInt32u warningId) override {
        if (warningId == SCRSDK::CrWarning_MovieRecordingOperation_Result_OK) {
            std::cout << "✅ Movie recording operation: OK" << std::endl;
        }
    }
    void OnWarningExt(CrInt32u, CrInt32, CrInt32, CrInt32) override {}
    void OnError(CrInt32u) override {}
    void OnPropertyChangedCodes(CrInt32u, CrInt32u*) override {}
    void OnLvPropertyChangedCodes(CrInt32u, CrInt32u*) override {}
};

class SonyA74Controller {
private:
    std::string camera_id;
    SCRSDK::CrDeviceHandle device_handle = 0;
    bool connected = false;
    DeviceCallback callback;

public:
    bool initialize() {
        std::cout << "Initializing Sony SDK..." << std::endl;
        const bool ok = SCRSDK::Init();
        if (!ok) {
            std::cout << "Failed to initialize SDK" << std::endl;
            return false;
        }
        std::cout << "SDK initialized successfully." << std::endl;
        return true;
    }

    bool findSonyA74() {
        std::cout << "Searching for Sony A74 (ILCE-7M4)..." << std::endl;
        
        SCRSDK::ICrEnumCameraObjectInfo* camera_list = nullptr;
        auto result = SCRSDK::EnumCameraObjects(&camera_list);
        
        if (CR_FAILED(result) || !camera_list) {
            std::cout << "Failed to enumerate cameras." << std::endl;
            return false;
        }

        auto count = camera_list->GetCount();
        std::cout << "Found " << count << " cameras:" << std::endl;
        
        for (int i = 0; i < count; ++i) {
            auto info = camera_list->GetCameraObjectInfo(i);
            if (info) {
                std::cout << "[" << i + 1 << "] " << info->GetModel() << std::endl;
                
                if (std::string(info->GetModel()) == "ILCE-7M4") {
                    std::cout << "✅ Found Sony A74!" << std::endl;
                    const CrInt8u* id_raw = info->GetId();
                    if (id_raw) camera_id = std::string(reinterpret_cast<const char*>(id_raw));
                    camera_list->Release();
                    return true;
                }
            }
        }
        
        camera_list->Release();
        std::cout << "❌ Sony A74 not found." << std::endl;
        return false;
    }

    bool connectToCamera() {
        if (camera_id.empty()) {
            std::cout << "No camera ID available." << std::endl;
            return false;
        }

        std::cout << "Connecting to Sony A74..." << std::endl;
        
        // Get camera info for connection
        SCRSDK::ICrEnumCameraObjectInfo* camera_list = nullptr;
        auto enum_result = SCRSDK::EnumCameraObjects(&camera_list);
        
        if (CR_FAILED(enum_result) || !camera_list) {
            return false;
        }
        
        const SCRSDK::ICrCameraObjectInfo* target_camera = nullptr;
        auto count = camera_list->GetCount();
        
        for (int i = 0; i < count; ++i) {
            auto info = camera_list->GetCameraObjectInfo(i);
            if (info) {
                const CrInt8u* id_raw = info->GetId();
                const std::string id = id_raw ? std::string(reinterpret_cast<const char*>(id_raw)) : std::string();
                if (id == camera_id) {
                    target_camera = info;
                    break;
                }
            }
        }
        
        if (!target_camera) {
            camera_list->Release();
            std::cout << "Camera not found for connection." << std::endl;
            return false;
        }
        
        auto result = SCRSDK::Connect(const_cast<SCRSDK::ICrCameraObjectInfo*>(target_camera),
                                      &callback,
                                      &device_handle,
                                      SCRSDK::CrSdkControlMode_Remote,
                                      SCRSDK::CrReconnecting_ON,
                                      nullptr, nullptr, nullptr, 0);
        camera_list->Release();
        
        if (CR_FAILED(result) || device_handle == 0) {
            std::cout << "Failed to connect: 0x" << std::hex << result << std::dec << std::endl;
            return false;
        }

        connected = true;
        std::cout << "✅ Connected to Sony A74 successfully!" << std::endl;
        return true;
    }

    bool startRecording() {
        if (!connected) {
            std::cout << "Not connected to camera." << std::endl;
            return false;
        }

        std::cout << "🎬 Starting recording..." << std::endl;
        
        // Send Movie Record Down command (start recording)
        auto result = SCRSDK::SendCommand(device_handle,
                                          SCRSDK::CrCommandId::CrCommandId_MovieRecord,
                                          SCRSDK::CrCommandParam::CrCommandParam_Down);
        
        if (CR_FAILED(result)) {
            std::cout << "❌ Failed to start recording: 0x" << std::hex << result << std::dec << std::endl;
            return false;
        }

        std::cout << "✅ Recording started successfully!" << std::endl;
        return true;
    }

    bool stopRecording() {
        if (!connected) {
            std::cout << "Not connected to camera." << std::endl;
            return false;
        }

        std::cout << "⏹️ Stopping recording..." << std::endl;
        
        // Send Movie Record Up command (stop recording)
        auto result = SCRSDK::SendCommand(device_handle,
                                          SCRSDK::CrCommandId::CrCommandId_MovieRecord,
                                          SCRSDK::CrCommandParam::CrCommandParam_Up);
        
        if (CR_FAILED(result)) {
            std::cout << "❌ Failed to stop recording: 0x" << std::hex << result << std::dec << std::endl;
            return false;
        }

        std::cout << "✅ Recording stopped successfully!" << std::endl;
        return true;
    }

    void disconnect() {
        if (connected && device_handle) {
            std::cout << "Disconnecting from camera..." << std::endl;
            SCRSDK::Disconnect(device_handle);
            SCRSDK::ReleaseDevice(device_handle);
            connected = false;
            device_handle = 0;
        }
    }

    ~SonyA74Controller() {
        disconnect();
        SCRSDK::Release();
    }
};

int main() {
    std::cout << "🎥 Sony A74 Direct API Recording Controller" << std::endl;
    std::cout << "==========================================" << std::endl;

    SonyA74Controller controller;

    // Initialize SDK
    if (!controller.initialize()) {
        return 1;
    }

    // Find Sony A74
    if (!controller.findSonyA74()) {
        return 1;
    }

    // Connect to camera
    if (!controller.connectToCamera()) {
        return 1;
    }

    // Start recording
    if (!controller.startRecording()) {
        return 1;
    }

    // Record for 5 seconds
    std::cout << "📹 Recording for 5 seconds..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(5));

    // Stop recording
    if (!controller.stopRecording()) {
        return 1;
    }

    std::cout << "🎉 Recording test completed successfully!" << std::endl;
    return 0;
}