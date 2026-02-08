#include "CRSDK/CameraRemote_SDK.h"
#include "CRSDK/IDeviceCallback.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <string>

namespace {
struct DeviceCallbackImpl : public SCRSDK::IDeviceCallback {
    void OnConnected(SCRSDK::DeviceConnectionVersioin version) override {
        std::cout << "[DeviceCallback] OnConnected(version=" << (int)version << ")\n";
    }
    void OnDisconnected(CrInt32u error) override {
        std::cout << "[DeviceCallback] OnDisconnected(error=0x" << std::hex << error << std::dec << ")\n";
    }
    void OnPropertyChanged() override {}
    void OnLvPropertyChanged() override {}
    void OnCompleteDownload(CrChar*, CrInt32u) override {}
    void OnWarning(CrInt32u) override {}
    void OnWarningExt(CrInt32u, CrInt32, CrInt32, CrInt32) override {}
    void OnError(CrInt32u) override {}
    void OnPropertyChangedCodes(CrInt32u, CrInt32u*) override {}
    void OnLvPropertyChangedCodes(CrInt32u, CrInt32u*) override {}
    void OnNotifyContentsTransfer(CrInt32u, SCRSDK::CrContentHandle, CrChar*) override {}
    void OnNotifyFTPTransferResult(CrInt32u, CrInt32u, CrInt32u) override {}
    void OnNotifyRemoteTransferResult(CrInt32u, CrInt32u, CrChar*) override {}
    void OnNotifyRemoteTransferResult(CrInt32u, CrInt32u, CrInt8u*, CrInt64u) override {}
    void OnNotifyRemoteTransferContentsListChanged(CrInt32u, CrInt32u, CrInt32u) override {}
    void OnReceivePlaybackTimeCode(CrInt32u) override {}
    void OnReceivePlaybackData(CrInt8u, CrInt32, CrInt8u*, CrInt64, CrInt64, CrInt32, CrInt32) override {}
    void OnNotifyRemoteFirmwareUpdateResult(CrInt32u, const void*) override {}
};

static bool get_prop_first_value(const SCRSDK::CrDeviceProperty& prop, uint32_t& out) {
    const CrInt8u* raw = prop.GetValues();
    const CrInt32u raw_size = prop.GetValueSize();
    const SCRSDK::CrDataType type = prop.GetValueType();
    const CrInt32u base = (type & 0x0FFFu);
    if (raw && raw_size >= 1) {
        switch (base) {
            case SCRSDK::CrDataType_UInt8:
            case SCRSDK::CrDataType_Int8:
                out = *reinterpret_cast<const CrInt8u*>(raw);
                return true;
            case SCRSDK::CrDataType_UInt16:
            case SCRSDK::CrDataType_Int16:
                if (raw_size >= 2) { out = *reinterpret_cast<const CrInt16u*>(raw); return true; }
                break;
            case SCRSDK::CrDataType_UInt32:
            case SCRSDK::CrDataType_Int32:
                if (raw_size >= 4) { out = *reinterpret_cast<const CrInt32u*>(raw); return true; }
                break;
            default:
                break;
        }
    }
    out = (uint32_t)prop.GetCurrentValue();
    return true;
}
}

int main() {
    std::cout << "=== Sony A74 USB Connection Test ===\n\n";

    if (!SCRSDK::Init()) {
        std::cout << "Failed to initialize SDK\n";
        return -1;
    }
    std::cout << "✓ SDK initialized successfully\n";

    SCRSDK::ICrEnumCameraObjectInfo* camera_list = nullptr;
    auto enum_result = SCRSDK::EnumCameraObjects(&camera_list);
    if (CR_FAILED(enum_result) || !camera_list) {
        std::cout << "Failed to enumerate cameras: 0x" << std::hex << enum_result << std::dec << "\n";
        SCRSDK::Release();
        return -1;
    }

    const auto camera_count = camera_list->GetCount();
    std::cout << "Found " << camera_count << " camera(s)\n";
    if (camera_count == 0) {
        camera_list->Release();
        SCRSDK::Release();
        return -1;
    }

    for (CrInt32u i = 0; i < camera_count; ++i) {
        const auto* info = camera_list->GetCameraObjectInfo(i);
        if (!info) continue;
        std::cout << "\nCamera " << (i + 1) << ":\n";
        std::cout << "  Model: " << (info->GetModel() ? info->GetModel() : "") << "\n";
        std::cout << "  Connection Type: " << (info->GetConnectionTypeName() ? info->GetConnectionTypeName() : "") << "\n";
    }

    std::cout << "\n=== Testing Connection to Camera 1 ===\n";
    auto* camera_info = const_cast<SCRSDK::ICrCameraObjectInfo*>(camera_list->GetCameraObjectInfo(0));
    if (!camera_info) {
        camera_list->Release();
        SCRSDK::Release();
        return -1;
    }

    DeviceCallbackImpl cb;
    SCRSDK::CrDeviceHandle handle = 0;
    auto connect_result = SCRSDK::Connect(camera_info, &cb, &handle,
                                                                                SCRSDK::CrSdkControlMode_Remote,
                                                                                SCRSDK::CrReconnecting_ON,
                                                                                nullptr, nullptr, nullptr, 0);
    if (CR_FAILED(connect_result) || handle == 0) {
        std::cout << "Failed to connect: 0x" << std::hex << connect_result << std::dec << "\n";
        camera_list->Release();
        SCRSDK::Release();
        return -1;
    }
    std::cout << "✓ Connected\n";

    std::cout << "\n=== Testing Movie Record Command ===\n";
    auto st_down = SCRSDK::SendCommand(handle,
                                                                         SCRSDK::CrCommandId::CrCommandId_MovieRecord,
                                                                         SCRSDK::CrCommandParam::CrCommandParam_Down);
    std::cout << "MovieRecord DOWN: 0x" << std::hex << st_down << std::dec << "\n";
    std::this_thread::sleep_for(std::chrono::seconds(2));

    SCRSDK::CrDeviceProperty* props = nullptr;
    CrInt32 num_props = 0;
    auto prop_err = SCRSDK::GetDeviceProperties(handle, &props, &num_props);
    if (!CR_FAILED(prop_err) && props && num_props > 0) {
        for (CrInt32 i = 0; i < num_props; ++i) {
            if (props[i].GetCode() == SCRSDK::CrDeviceProperty_RecordingState) {
                uint32_t v = 0;
                if (get_prop_first_value(props[i], v)) {
                    std::cout << "Recording State: " << v << "\n";
                }
                break;
            }
        }
    }
    if (props) SCRSDK::ReleaseDeviceProperties(handle, props);

    std::this_thread::sleep_for(std::chrono::seconds(3));
    auto st_up = SCRSDK::SendCommand(handle,
                                                                     SCRSDK::CrCommandId::CrCommandId_MovieRecord,
                                                                     SCRSDK::CrCommandParam::CrCommandParam_Up);
    std::cout << "MovieRecord UP: 0x" << std::hex << st_up << std::dec << "\n";

    SCRSDK::Disconnect(handle);
    SCRSDK::ReleaseDevice(handle);
    camera_list->Release();
    SCRSDK::Release();
    std::cout << "\n=== Test Complete ===\n";
    return 0;
}