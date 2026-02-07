#include "CameraRemote_SDK.h"
#include "CRSDK/IDeviceCallback.h"
#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <atomic>
#include <string>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <arpa/inet.h>

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

static std::string normalize_fingerprint(const char* data, CrInt32u len) {
    if (!data || len == 0) return std::string();
    std::string out(data, data + len);
    while (out.size() % 4 != 0) {
        out.push_back('=');
    }
    return out;
}

static const char* select_fingerprint(const char* env_fp,
                                      CrInt32u env_len,
                                      const char* cam_fp,
                                      CrInt32u cam_len,
                                      CrInt32u* out_len) {
    static thread_local std::string normalized;
    normalized.clear();
    if (out_len) *out_len = 0;
    if (env_fp && env_len > 0) {
        normalized = normalize_fingerprint(env_fp, env_len);
    } else if (cam_fp && cam_len > 0) {
        normalized = normalize_fingerprint(cam_fp, cam_len);
    }
    if (normalized.empty()) return nullptr;
    if (out_len) *out_len = (CrInt32u)normalized.size();
    return normalized.c_str();
}

static bool parse_mac_from_env_or_arp(const char* ip_str, CrInt8u mac_out[6]) {
    if (!mac_out) return false;
    std::memset(mac_out, 0, 6);

    const char* env_mac = std::getenv("SONY_CAMERA_MAC");
    if (env_mac && env_mac[0]) {
        unsigned int ma[6] = {0};
        if (std::sscanf(env_mac, "%x:%x:%x:%x:%x:%x", &ma[0], &ma[1], &ma[2], &ma[3], &ma[4], &ma[5]) == 6) {
            for (int i = 0; i < 6; ++i) mac_out[i] = (CrInt8u)(ma[i] & 0xFF);
            std::cout << "🔎 Using SONY_CAMERA_MAC=" << env_mac << std::endl;
            return true;
        }
    }

    if (!ip_str || !ip_str[0]) return false;

    // Refresh ARP entry
    std::string pingcmd = std::string("ping -c1 -W1 ") + ip_str + " > /dev/null 2>&1";
    std::system(pingcmd.c_str());

    std::string cmd = std::string("ip neigh show ") + ip_str + " 2>/dev/null";
    FILE* f = popen(cmd.c_str(), "r");
    if (!f) return false;

    char buf[256];
    if (fgets(buf, sizeof(buf), f)) {
        unsigned int ma[6] = {0};
        if (std::sscanf(buf, "%*s dev %*s lladdr %x:%x:%x:%x:%x:%x", &ma[0], &ma[1], &ma[2], &ma[3], &ma[4], &ma[5]) == 6) {
            for (int i = 0; i < 6; ++i) mac_out[i] = (CrInt8u)(ma[i] & 0xFF);
            std::cout << "🔎 MAC via ip neigh: "
                      << std::hex
                      << (int)mac_out[0] << ":" << (int)mac_out[1] << ":" << (int)mac_out[2] << ":"
                      << (int)mac_out[3] << ":" << (int)mac_out[4] << ":" << (int)mac_out[5]
                      << std::dec << std::endl;
            pclose(f);
            return true;
        }
    }
    pclose(f);
    return false;
}

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

static bool get_property_info(CrDeviceHandle handle,
                              CrInt32u code,
                              CrDataType& out_type,
                              CrInt64u& out_current) {
    CrDeviceProperty* props = nullptr;
    CrInt32 num_props = 0;
    auto err = SCRSDK::GetDeviceProperties(handle, &props, &num_props);
    if (CR_FAILED(err) || !props || num_props <= 0) {
        std::cout << "❌ GetDeviceProperties failed 0x" << std::hex << err << std::dec
                  << " num_props=" << num_props << std::endl;
        if (props) SCRSDK::ReleaseDeviceProperties(handle, props);
        return false;
    }

    bool found = false;
    for (CrInt32 i = 0; i < num_props; ++i) {
        if (props[i].GetCode() == code) {
            found = true;
            out_type = props[i].GetValueType();
            auto values = parse_values(props[i]);
            if (!values.empty()) {
                out_current = values[0];
            } else {
                out_current = props[i].GetCurrentValue();
            }
            break;
        }
    }

    SCRSDK::ReleaseDeviceProperties(handle, props);
    return found;
}

static bool set_property_value(CrDeviceHandle handle,
                               CrInt32u code,
                               CrDataType type,
                               CrInt64u value) {
    SCRSDK::CrDeviceProperty setprop;
    setprop.SetCode(code);
    setprop.SetValueType(type);
    setprop.SetCurrentValue(value);
    auto err = SCRSDK::SetDeviceProperty(handle, &setprop);
    if (CR_FAILED(err)) {
        std::cout << "❌ SetDeviceProperty failed 0x" << std::hex << err << std::dec << std::endl;
        return false;
    }
    return true;
}

static bool set_property_direct(CrDeviceHandle handle,
                                CrInt32u code,
                                CrDataType type,
                                CrInt64u value,
                                const char* label) {
    std::cout << "🎯 " << label << " set request value=0x" << std::hex << value << std::dec
              << " type=0x" << std::hex << type << std::dec << std::endl;
    if (!set_property_value(handle, code, type, value)) {
        std::cout << "❌ " << label << " set failed" << std::endl;
        return false;
    }
    std::cout << "✅ " << label << " set OK" << std::endl;
    return true;
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

static bool set_property_with_current_type(CrDeviceHandle handle,
                                           CrInt32u code,
                                           const char* label,
                                           bool has_desired,
                                           CrInt64u desired_value) {
    CrDataType type = CrDataType_UInt8;
    CrInt64u current = 0;
    if (!get_property_info(handle, code, type, current)) {
        std::cout << "❌ " << label << ": property not found via GetDeviceProperties" << std::endl;
        return false;
    }

    const CrInt64u chosen = has_desired ? desired_value : current;
    std::cout << "🔎 " << label << " current=0x" << std::hex << current << std::dec
              << " type=0x" << std::hex << type << std::dec << std::endl;

    if (!set_property_value(handle, code, type, chosen)) {
        std::cout << "❌ " << label << " set failed" << std::endl;
        return false;
    }

    std::cout << "✅ " << label << " set to 0x" << std::hex << chosen << std::dec << std::endl;
    return true;
}

int main() {
    std::cout << "🎛️  Sony A74 Ethernet Settings Test" << std::endl;
    std::cout << "==================================" << std::endl;

    const char* ip_env = std::getenv("SONY_CAMERA_IP");
    if (!ip_env || !ip_env[0]) {
        std::cout << "❌ SONY_CAMERA_IP is not set" << std::endl;
        return -1;
    }

    const bool init_ok = SCRSDK::Init();
    if (!init_ok) {
        std::cout << "❌ Failed to initialize SDK" << std::endl;
        return -1;
    }

    in_addr ina;
    if (inet_pton(AF_INET, ip_env, &ina) != 1) {
        std::cout << "❌ Invalid SONY_CAMERA_IP=" << ip_env << std::endl;
        SCRSDK::Release();
        return -1;
    }

    CrInt32u ipAddr = (CrInt32u)ntohl(ina.s_addr);
    CrInt8u macBuf[6] = {0};
    parse_mac_from_env_or_arp(ip_env, macBuf);

    const char* ssh_env = std::getenv("SONY_SSH_SUPPORT");
    CrInt32u ssh_support = 1;
    if (ssh_env && ssh_env[0]) {
        ssh_support = (std::atoi(ssh_env) != 0) ? 1u : 0u;
    }

    SCRSDK::CrCameraDeviceModelList model = SCRSDK::CrCameraDeviceModelList::CrCameraDeviceModel_ILCE_7M4;
    const char* model_env = std::getenv("SONY_CAMERA_MODEL");
    if (model_env && model_env[0]) {
        unsigned long model_val = std::strtoul(model_env, nullptr, 0);
        model = static_cast<SCRSDK::CrCameraDeviceModelList>(model_val);
    }

    std::cout << "🔧 CreateCameraObjectInfoEthernetConnection model=" << (unsigned)model
              << " ip=" << ip_env << " ipNum=" << ipAddr << " ssh=" << (unsigned)ssh_support << std::endl;

    ICrCameraObjectInfo* camInfo = nullptr;
    auto err = SCRSDK::CreateCameraObjectInfoEthernetConnection(
        &camInfo,
        model,
        ipAddr,
        macBuf,
        ssh_support);

    if (CR_FAILED(err) || !camInfo) {
        std::cout << "❌ CreateCameraObjectInfoEthernetConnection failed 0x" << std::hex << err << std::dec << std::endl;
        SCRSDK::Release();
        return -1;
    }

    std::cout << "✅ Camera object created" << std::endl;

    char fingerprint[4096] = {0};
    CrInt32u fpSize = 0;
    const char* env_fp = std::getenv("SONY_FINGERPRINT");
    if (!env_fp || !env_fp[0]) {
        fpSize = (CrInt32u)sizeof(fingerprint);
        std::cout << "🔐 Fetching fingerprint..." << std::endl;
        auto fpSt = SCRSDK::GetFingerprint(camInfo, fingerprint, &fpSize);
        if (CR_FAILED(fpSt) || fpSize == 0) {
            std::cout << "⚠️  GetFingerprint failed 0x" << std::hex << fpSt << std::dec << std::endl;
            fpSize = 0;
        } else {
            std::cout << "✅ Fingerprint fetched (size=" << fpSize << ")" << std::endl;
        }
    } else {
        std::cout << "🔐 Using SONY_FINGERPRINT from env" << std::endl;
    }

    const char* accept_fp = std::getenv("SONY_ACCEPT_FINGERPRINT");
    if (!(accept_fp && accept_fp[0] == '1')) {
        std::cout << "❌ Fingerprint requires acceptance. Set SONY_ACCEPT_FINGERPRINT=1" << std::endl;
        camInfo->Release();
        SCRSDK::Release();
        return -1;
    }

    const char* pass = std::getenv("SONY_PASS");
    if (!pass || !pass[0]) {
        std::cout << "❌ SONY_PASS is not set" << std::endl;
        camInfo->Release();
        SCRSDK::Release();
        return -1;
    }

    const char* user = std::getenv("SONY_USER");
    if (user && !user[0]) user = nullptr;

    const CrInt32u env_fp_len = (env_fp && env_fp[0]) ? (CrInt32u)std::strlen(env_fp) : 0;
    CrInt32u fp_len = 0;
    const char* fp_ptr = select_fingerprint(env_fp, env_fp_len, fingerprint, fpSize, &fp_len);

    MinimalCallback callback;
    CrDeviceHandle device_handle = 0;
    err = SCRSDK::Connect(camInfo, &callback, &device_handle,
                          SCRSDK::CrSdkControlMode_Remote,
                          SCRSDK::CrReconnecting_ON,
                          user, pass, fp_ptr, fp_len);

    if (CR_FAILED(err) || device_handle == 0) {
        std::cout << "❌ Connect failed 0x" << std::hex << err << std::dec << std::endl;
        camInfo->Release();
        SCRSDK::Release();
        return -1;
    }

    for (int i = 0; i < 30 && !callback.connected.load(); ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::cout << "✅ Connected!" << std::endl;

    std::this_thread::sleep_for(std::chrono::seconds(2));

    // Dump property list summary for debugging
    {
        SCRSDK::CrDeviceProperty* props = nullptr;
        CrInt32 num_props = 0;
        auto err = SCRSDK::GetDeviceProperties(device_handle, &props, &num_props);
        if (!CR_FAILED(err) && props && num_props > 0) {
            std::cout << "📋 GetDeviceProperties count=" << num_props << std::endl;
            const char* dump_env = std::getenv("SONY_DUMP_PROPERTIES");
            if (dump_env && dump_env[0] == '1') {
                for (CrInt32 i = 0; i < num_props; ++i) {
                    std::cout << "  - code=0x" << std::hex << props[i].GetCode() << std::dec
                              << " type=0x" << std::hex << props[i].GetValueType() << std::dec
                              << " size=" << props[i].GetValueSize() << std::endl;
                }
            }
        } else {
            std::cout << "❌ GetDeviceProperties failed 0x" << std::hex << err << std::dec
                      << " num_props=" << num_props << std::endl;
        }
        if (props) SCRSDK::ReleaseDeviceProperties(device_handle, props);
    }

    print_status_properties(device_handle, "Status");

    // ISO -> WB -> Shutter -> Timeline FPS (Movie_Recording_FrameRateSetting)
    const CrInt64u iso_target = ((CrInt64u)CrISO_Normal << 24) | 800;
    set_property_direct(device_handle, CrDeviceProperty_IsoSensitivity, CrDataType_UInt32Array, iso_target, "ISO");
    set_property_direct(device_handle, CrDeviceProperty_WhiteBalance, CrDataType_UInt16Array, CrWhiteBalance_Daylight, "WhiteBalance");
    // ShutterSpeed: attempt to keep current if available, else skip
    {
        CrDataType type = CrDataType_UInt32Array;
        CrInt64u current = 0;
        if (get_property_info(device_handle, CrDeviceProperty_ShutterSpeed, type, current)) {
            set_property_value(device_handle, CrDeviceProperty_ShutterSpeed, type, current);
            std::cout << "✅ ShutterSpeed set to current value" << std::endl;
        } else {
            std::cout << "⚠️  ShutterSpeed not found; skipping" << std::endl;
        }
    }
    set_property_direct(device_handle, CrDeviceProperty_Movie_Recording_FrameRateSetting, CrDataType_UInt8Array,
                        CrRecordingFrameRateSettingMovie_24p, "TimelineFPS");

    std::cout << "\n🔌 Disconnecting..." << std::endl;
    const char* skip_disc = std::getenv("SONY_SKIP_DISCONNECT");
    if (!(skip_disc && skip_disc[0] == '1')) {
        SCRSDK::Disconnect(device_handle);
        SCRSDK::ReleaseDevice(device_handle);
    } else {
        std::cout << "⚠️  SONY_SKIP_DISCONNECT=1: skipping disconnect" << std::endl;
    }
    const char* fast_exit = std::getenv("SONY_FAST_EXIT");
    if (fast_exit && fast_exit[0] == '1') {
        std::cout << "⚠️  SONY_FAST_EXIT=1: exiting without SDK release" << std::endl;
        std::_Exit(0);
    }
    camInfo->Release();
    SCRSDK::Release();
    std::cout << "✅ Test completed!" << std::endl;
    return 0;
}
