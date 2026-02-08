#include "CRSDK/CameraRemote_SDK.h"
#include "CRSDK/IDeviceCallback.h"
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <chrono>

// Minimal callback implementation
class DirectAPICallback : public SCRSDK::IDeviceCallback {
public:
    void OnConnected(SCRSDK::DeviceConnectionVersioin) override {
        std::cout << "[Callback] Connected" << std::endl;
    }

    void OnDisconnected(CrInt32u error) override {
        std::cout << "[Callback] Disconnected: 0x" << std::hex << error << std::dec << std::endl;
    }

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
            recording_ok = true;
        }
    }

    void OnWarningExt(CrInt32u, CrInt32, CrInt32, CrInt32) override {}
    void OnError(CrInt32u) override {}
    void OnPropertyChangedCodes(CrInt32u, CrInt32u*) override {}
    void OnLvPropertyChangedCodes(CrInt32u, CrInt32u*) override {}

    bool recording_ok = false;
};

class DirectSonyAPI {
private:
    std::vector<const SCRSDK::ICrCameraObjectInfo*> camera_list;
    SCRSDK::CrDeviceHandle device_handle = 0;
    DirectAPICallback callback;
    bool connected = false;

public:
    bool initialize() {
        std::cout << "🚀 Initializing Sony SDK (like RemoteCli)..." << std::endl;
        
        const bool init_ok = SCRSDK::Init();
        if (!init_ok) {
            std::cout << "❌ SDK initialization failed (Init returned false)." << std::endl;
            return false;
        }
        
        std::cout << "✅ Sony SDK initialized successfully" << std::endl;
        return true;
    }
    
    bool enumerateCameras() {
        std::cout << "📡 Enumerating cameras..." << std::endl;
        
        SCRSDK::ICrEnumCameraObjectInfo* enum_camera_list = nullptr;
        auto result = SCRSDK::EnumCameraObjects(&enum_camera_list);
        
        if (CR_FAILED(result) || !enum_camera_list) {
            std::cout << "❌ Failed to enumerate cameras" << std::endl;
            return false;
        }
        
        auto count = enum_camera_list->GetCount();
        std::cout << "📷 Found " << count << " cameras:" << std::endl;
        
        for (int i = 0; i < count; ++i) {
            auto info = enum_camera_list->GetCameraObjectInfo(i);
            if (info) {
                std::cout << "[" << (i+1) << "] " << info->GetModel() << std::endl;
                camera_list.push_back(info);
            }
        }
        
        return count > 0;
    }
    
    bool connectToSonyA74() {
        // Find Sony A74
        const SCRSDK::ICrCameraObjectInfo* sony_a74 = nullptr;
        for (auto camera : camera_list) {
            if (std::string(camera->GetModel()) == "ILCE-7M4") {
                sony_a74 = camera;
                break;
            }
        }
        
        if (!sony_a74) {
            std::cout << "❌ Sony A74 (ILCE-7M4) not found" << std::endl;
            return false;
        }
        
        std::cout << "🔗 Connecting to Sony A74..." << std::endl;
        
        auto result = SCRSDK::Connect(const_cast<SCRSDK::ICrCameraObjectInfo*>(sony_a74),
                                      &callback,
                                      &device_handle,
                                      SCRSDK::CrSdkControlMode_Remote,
                                      SCRSDK::CrReconnecting_ON,
                                      nullptr, nullptr, nullptr, 0);
        if (CR_FAILED(result) || device_handle == 0) {
            std::cout << "❌ Failed to connect: 0x" << std::hex << result << std::dec << std::endl;
            return false;
        }
        
        connected = true;
        std::cout << "✅ Connected to Sony A74!" << std::endl;
        return true;
    }
    
    bool startRecording() {
        if (!connected) {
            std::cout << "❌ Not connected" << std::endl;
            return false;
        }
        
        std::cout << "🎬 Starting recording (API: MovieRecord Down)..." << std::endl;
        callback.recording_ok = false;
        
        auto result = SCRSDK::SendCommand(device_handle,
                                          SCRSDK::CrCommandId::CrCommandId_MovieRecord,
                                          SCRSDK::CrCommandParam::CrCommandParam_Down);
        if (CR_FAILED(result)) {
            std::cout << "❌ Start recording failed: 0x" << std::hex << result << std::dec << std::endl;
            return false;
        }
        
        // Wait for callback confirmation
        for (int i = 0; i < 20; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            if (callback.recording_ok) {
                std::cout << "✅ Recording started successfully!" << std::endl;
                return true;
            }
        }
        
        std::cout << "⚠️ Recording command sent (no callback confirmation)" << std::endl;
        return true;
    }
    
    bool stopRecording() {
        if (!connected) {
            std::cout << "❌ Not connected" << std::endl;
            return false;
        }
        
        std::cout << "⏹️ Stopping recording (API: MovieRecord Up)..." << std::endl;
        callback.recording_ok = false;
        
        auto result = SCRSDK::SendCommand(device_handle,
                                          SCRSDK::CrCommandId::CrCommandId_MovieRecord,
                                          SCRSDK::CrCommandParam::CrCommandParam_Up);
        if (CR_FAILED(result)) {
            std::cout << "❌ Stop recording failed: 0x" << std::hex << result << std::dec << std::endl;
            return false;
        }
        
        // Wait for callback confirmation  
        for (int i = 0; i < 20; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            if (callback.recording_ok) {
                std::cout << "✅ Recording stopped successfully!" << std::endl;
                return true;
            }
        }
        
        std::cout << "⚠️ Stop recording command sent (no callback confirmation)" << std::endl;
        return true;
    }
    
    void disconnect() {
        if (connected && device_handle) {
            std::cout << "🔌 Disconnecting..." << std::endl;
            SCRSDK::Disconnect(device_handle);
            SCRSDK::ReleaseDevice(device_handle);
            connected = false;
        }
    }
    
    ~DirectSonyAPI() {
        disconnect();
        SCRSDK::Release();
    }
};

int main() {
    std::cout << "🎯 Sony A74 Direct API Recording Test" << std::endl;
    std::cout << "====================================" << std::endl;
    
    DirectSonyAPI api;
    
    if (!api.initialize()) {
        return 1;
    }
    
    if (!api.enumerateCameras()) {
        return 1;
    }
    
    if (!api.connectToSonyA74()) {
        return 1;  
    }
    
    // Test rapid recording start/stop
    std::cout << "\n⚡ Testing DIRECT API recording..." << std::endl;
    
    if (api.startRecording()) {
        std::cout << "📹 Recording for 3 seconds..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(3));
        
        api.stopRecording();
    }
    
    std::cout << "\n🎉 Direct API test completed!" << std::endl;
    return 0;
}