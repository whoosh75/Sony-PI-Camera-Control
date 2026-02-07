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
        if (warning == CrWarning_MovieRecordingOperation_Result_OK) {
            std::cout << "✅ OnWarning: MovieRecordingOperation_Result_OK" << std::endl;
        } else {
            std::cout << "ℹ️  OnWarning: 0x" << std::hex << warning << std::dec << std::endl;
        }
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

static void print_status_properties(CrDeviceHandle handle, const char* label) {
    SCRSDK::CrDeviceProperty* props = nullptr;
    CrInt32 num_props = 0;
    auto err = SCRSDK::GetDeviceProperties(handle, &props, &num_props);
    if (CR_FAILED(err) || !props || num_props <= 0) {
        std::cout << "❌ " << label << ": GetDeviceProperties failed 0x" << std::hex << err << std::dec
                  << " num_props=" << num_props << std::endl;
        if (props) SCRSDK::ReleaseDeviceProperties(handle, props);
        return;
    }

    auto print_code = [&](CrInt32u code, const char* name) {
        for (CrInt32 i = 0; i < num_props; ++i) {
            if (props[i].GetCode() == code) {
                auto values = parse_values(props[i]);
                if (!values.empty()) {
                    std::cout << "✅ " << name << " = 0x" << std::hex << values[0] << std::dec << std::endl;
                } else {
                    std::cout << "✅ " << name << " (present, no value data)" << std::endl;
                }
                return;
            }
        }
        std::cout << "⚠️  " << name << " not found" << std::endl;
    };

    std::cout << "🔋 Battery/Media status" << std::endl;
    print_code(CrDeviceProperty_BatteryLevel, "BatteryLevel");
    print_code(CrDeviceProperty_BatteryRemain, "BatteryRemain");
    print_code(CrDeviceProperty_BatteryRemainDisplayUnit, "BatteryRemainDisplayUnit");
    print_code(CrDeviceProperty_RecordingMedia, "RecordingMedia");
    print_code(CrDeviceProperty_Movie_RecordingMedia, "Movie_RecordingMedia");
    print_code(CrDeviceProperty_MediaSLOT1_Status, "MediaSLOT1_Status");
    print_code(CrDeviceProperty_MediaSLOT1_RemainingNumber, "MediaSLOT1_RemainingNumber");
    print_code(CrDeviceProperty_MediaSLOT1_RemainingTime, "MediaSLOT1_RemainingTime");
    print_code(CrDeviceProperty_MediaSLOT2_Status, "MediaSLOT2_Status");
    print_code(CrDeviceProperty_MediaSLOT2_RemainingNumber, "MediaSLOT2_RemainingNumber");
    print_code(CrDeviceProperty_MediaSLOT2_RemainingTime, "MediaSLOT2_RemainingTime");

    SCRSDK::ReleaseDeviceProperties(handle, props);
}

static bool choose_and_set_property(CrDeviceHandle handle,
                                    CrInt32u code,
                                    const char* label,
                                    bool has_desired,
                                    CrInt64u desired_value) {
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

    const bool is_range = (type & CrDataType_RangeBit) != 0;
    CrInt64u chosen = current;

    if (is_range) {
        auto vals = parse_values(prop);
        if (vals.size() >= 2) {
            CrInt64u minv = vals[0];
            CrInt64u maxv = vals[1];
            if (has_desired && desired_value >= minv && desired_value <= maxv) {
                chosen = desired_value;
            } else if (current != minv) {
                chosen = minv;
            } else {
                chosen = maxv;
            }
            std::cout << "📐 " << label << " range min=0x" << std::hex << minv
                      << " max=0x" << maxv << std::dec << std::endl;
        }
    } else {
        auto vals = parse_values(prop);
        if (!vals.empty()) {
            bool found_desired = false;
            for (auto v : vals) {
                if (has_desired && v == desired_value) {
                    chosen = v;
                    found_desired = true;
                    break;
                }
            }
            if (!found_desired) {
                for (auto v : vals) {
                    if (v != current) { chosen = v; break; }
                }
            }
            std::cout << "📋 " << label << " options=" << vals.size() << std::endl;
        } else if (has_desired) {
            chosen = desired_value;
        }
    }

    if (!prop.IsSetEnableCurrentValue()) {
        std::cout << "❌ " << label << " is read-only on this camera" << std::endl;
        SCRSDK::ReleaseDeviceProperties(handle, props);
        return false;
    }

    SCRSDK::CrDeviceProperty setprop;
    setprop.SetCode(code);
    setprop.SetValueType(type);
    setprop.SetCurrentValue(chosen);

    auto set_err = SCRSDK::SetDeviceProperty(handle, &setprop);
    SCRSDK::ReleaseDeviceProperties(handle, props);

    if (CR_FAILED(set_err)) {
        std::cout << "❌ " << label << " set failed 0x" << std::hex << set_err << std::dec << std::endl;
        return false;
    }

    std::cout << "✅ " << label << " set to 0x" << std::hex << chosen << std::dec << std::endl;
    return true;
}

int main() {
    std::cout << "🎛️  Sony A74 USB Settings Test" << std::endl;
    std::cout << "==============================" << std::endl;

    const bool init_ok = SCRSDK::Init();
    if (!init_ok) {
        std::cout << "❌ Failed to initialize SDK" << std::endl;
        return -1;
    }

    std::cout << "✅ Sony SDK initialized" << std::endl;
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

    std::this_thread::sleep_for(std::chrono::seconds(1));
    print_status_properties(device_handle, "Status");

    // ISO -> WB -> Shutter -> FPS
    const CrInt64u iso_target = ((CrInt64u)CrISO_Normal << 24) | 800;
    choose_and_set_property(device_handle, CrDeviceProperty_IsoSensitivity, "ISO", true, iso_target);

    choose_and_set_property(device_handle, CrDeviceProperty_WhiteBalance, "WhiteBalance", true, CrWhiteBalance_Daylight);

    // For shutter, attempt to keep current unless alternative available
    choose_and_set_property(device_handle, CrDeviceProperty_ShutterSpeed, "ShutterSpeed", false, 0);

    choose_and_set_property(device_handle, CrDeviceProperty_Movie_Recording_FrameRateSetting, "FrameRate", false, 0);

    std::cout << "\n🔌 Disconnecting..." << std::endl;
    SCRSDK::Disconnect(device_handle);
    camera_list->Release();
    SCRSDK::Release();
    std::cout << "✅ Test completed!" << std::endl;
    return 0;
}
