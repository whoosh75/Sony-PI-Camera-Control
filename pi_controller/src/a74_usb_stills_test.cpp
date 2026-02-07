#include "CameraRemote_SDK.h"
#include "CRSDK/IDeviceCallback.h"
#include <iostream>
#include <vector>
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
        std::cout << "ℹ️  OnWarning: 0x" << std::hex << warning << std::dec << std::endl;
    }
};

static size_t element_size(CrDataType type) {
    const CrInt32u base = (type & 0x0FFFu);
    switch (base) {
        case CrDataType_UInt8:
        case CrDataType_Int8:
            return 1;
        case CrDataType_UInt16:
        case CrDataType_Int16:
            return 2;
        case CrDataType_UInt32:
        case CrDataType_Int32:
            return 4;
        case CrDataType_UInt64:
        case CrDataType_Int64:
            return 8;
        default:
            return 0;
    }
}

static std::vector<CrInt64u> parse_values(const CrDeviceProperty& prop) {
    std::vector<CrInt64u> out;
    const CrInt8u* raw = prop.GetSetValues();
    const CrInt32u raw_size = prop.GetSetValueSize();
    const CrDataType type = prop.GetValueType();
    const size_t es = element_size(type);
    if (!raw || raw_size == 0 || es == 0) return out;

    const size_t count = raw_size / es;
    out.reserve(count);
    for (size_t i = 0; i < count; ++i) {
        CrInt64u v = 0;
        const CrInt8u* p = raw + (i * es);
        switch (es) {
            case 1: v = *reinterpret_cast<const CrInt8u*>(p); break;
            case 2: v = *reinterpret_cast<const CrInt16u*>(p); break;
            case 4: v = *reinterpret_cast<const CrInt32u*>(p); break;
            case 8: v = *reinterpret_cast<const CrInt64u*>(p); break;
        }
        out.push_back(v);
    }
    return out;
}

static bool choose_and_set_property(CrDeviceHandle handle,
                                    CrInt32u code,
                                    const char* label) {
    CrDeviceProperty* props = nullptr;
    CrInt32 num_props = 0;
    CrInt32u code_copy = code;
    auto err = SCRSDK::GetSelectDeviceProperties(handle, 1, &code_copy, &props, &num_props);
    if (CR_FAILED(err) || !props || num_props <= 0) {
        std::cout << "❌ " << label << ": GetSelectDeviceProperties failed 0x" << std::hex << err << std::dec << std::endl;
        if (props) SCRSDK::ReleaseDeviceProperties(handle, props);
        return false;
    }

    const CrDeviceProperty& prop = props[0];
    const CrDataType type = prop.GetValueType();
    const CrInt64u current = prop.GetCurrentValue();

    std::cout << "🔎 " << label << " current=0x" << std::hex << current << std::dec
              << " type=0x" << std::hex << type << std::dec << std::endl;

    if (!prop.IsSetEnableCurrentValue()) {
        std::cout << "❌ " << label << " is read-only on this camera" << std::endl;
        SCRSDK::ReleaseDeviceProperties(handle, props);
        return false;
    }

    SCRSDK::CrDeviceProperty setprop;
    setprop.SetCode(code);
    setprop.SetValueType(type);
    setprop.SetCurrentValue(current);

    auto set_err = SCRSDK::SetDeviceProperty(handle, &setprop);
    SCRSDK::ReleaseDeviceProperties(handle, props);

    if (CR_FAILED(set_err)) {
        std::cout << "❌ " << label << " set failed 0x" << std::hex << set_err << std::dec << std::endl;
        return false;
    }

    std::cout << "✅ " << label << " set to current value" << std::endl;
    return true;
}

int main() {
    std::cout << "📸 Sony A74 USB Stills Test" << std::endl;
    std::cout << "==========================" << std::endl;

    const bool init_ok = SCRSDK::Init();
    if (!init_ok) {
        std::cout << "❌ Failed to initialize SDK" << std::endl;
        return -1;
    }

    std::this_thread::sleep_for(std::chrono::seconds(2));

    ICrEnumCameraObjectInfo* camera_list = nullptr;
    auto ret = SCRSDK::EnumCameraObjects(&camera_list);
    if (ret != CrError_None || !camera_list || camera_list->GetCount() == 0) {
        std::cout << "❌ No cameras found via USB" << std::endl;
        SCRSDK::Release();
        return -1;
    }

    auto camera_info = camera_list->GetCameraObjectInfo(0);
    CrDeviceHandle device_handle = 0;
    MinimalCallback callback;

    std::cout << "🔗 Connecting to Sony A74..." << std::endl;
    ret = SCRSDK::Connect(const_cast<ICrCameraObjectInfo*>(camera_info), &callback, &device_handle);
    if (ret != CrError_None || device_handle == 0) {
        std::cout << "❌ Failed to connect: 0x" << std::hex << ret << std::dec << std::endl;
        camera_list->Release();
        SCRSDK::Release();
        return -1;
    }

    for (int i = 0; i < 20 && !callback.connected.load(); ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::cout << "✅ Connected!" << std::endl;

    // A74 stills-mode settings
    choose_and_set_property(device_handle, CrDeviceProperty_FocusMode, "FocusMode");
    choose_and_set_property(device_handle, CrDeviceProperty_FocusArea, "FocusArea");
    choose_and_set_property(device_handle, CrDeviceProperty_DriveMode, "DriveMode");
    choose_and_set_property(device_handle, CrDeviceProperty_StillImageQuality, "StillImageQuality");
    choose_and_set_property(device_handle, CrDeviceProperty_CompressionFileFormatStill, "CompressionFileFormatStill");
    choose_and_set_property(device_handle, CrDeviceProperty_StillImageStoreDestination, "StillImageStoreDestination");

    std::cout << "\n📸 Triggering still shutter release..." << std::endl;
    ret = SCRSDK::SendCommand(device_handle, CrCommandId_Release, CrCommandParam_Down);
    if (!CR_FAILED(ret)) {
        SCRSDK::SendCommand(device_handle, CrCommandId_Release, CrCommandParam_Up);
        std::cout << "✅ Release command sent" << std::endl;
    } else {
        std::cout << "❌ Release command failed 0x" << std::hex << ret << std::dec << std::endl;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(800));

    std::cout << "\n🎯 Triggering AF + release (S1andRelease)..." << std::endl;
    ret = SCRSDK::SendCommand(device_handle, CrCommandId_S1andRelease, CrCommandParam_Down);
    if (!CR_FAILED(ret)) {
        SCRSDK::SendCommand(device_handle, CrCommandId_S1andRelease, CrCommandParam_Up);
        std::cout << "✅ S1andRelease command sent" << std::endl;
    } else {
        std::cout << "❌ S1andRelease command failed 0x" << std::hex << ret << std::dec << std::endl;
    }

    std::cout << "\n🔌 Disconnecting..." << std::endl;
    SCRSDK::Disconnect(device_handle);
    camera_list->Release();
    SCRSDK::Release();
    std::cout << "✅ Test completed!" << std::endl;
    return 0;
}
