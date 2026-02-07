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

    ICrCameraObjectInfo* camInfo = nullptr;
    auto err = SCRSDK::CreateCameraObjectInfoEthernetConnection(
        &camInfo,
        SCRSDK::CrCameraDeviceModelList::CrCameraDeviceModel_ILCE_7M4,
        ipAddr,
        macBuf,
        1);

    if (CR_FAILED(err) || !camInfo) {
        std::cout << "❌ CreateCameraObjectInfoEthernetConnection failed 0x" << std::hex << err << std::dec << std::endl;
        SCRSDK::Release();
        return -1;
    }

    char fingerprint[4096] = {0};
    CrInt32u fpSize = (CrInt32u)sizeof(fingerprint);
    auto fpSt = SCRSDK::GetFingerprint(camInfo, fingerprint, &fpSize);
    if (CR_FAILED(fpSt) || fpSize == 0) {
        std::cout << "⚠️  GetFingerprint failed 0x" << std::hex << fpSt << std::dec << std::endl;
        fpSize = 0;
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

    const char* env_fp = std::getenv("SONY_FINGERPRINT");
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

    // ISO -> WB -> Shutter -> FPS -> Project Rate (TimeCodeFormat)
    const CrInt64u iso_target = ((CrInt64u)CrISO_Normal << 24) | 800;
    choose_and_set_property(device_handle, CrDeviceProperty_IsoSensitivity, "ISO", true, iso_target);
    choose_and_set_property(device_handle, CrDeviceProperty_WhiteBalance, "WhiteBalance", true, CrWhiteBalance_Daylight);
    choose_and_set_property(device_handle, CrDeviceProperty_ShutterSpeed, "ShutterSpeed", false, 0);
    choose_and_set_property(device_handle, CrDeviceProperty_Movie_Recording_FrameRateSetting, "FrameRate", false, 0);
    choose_and_set_property(device_handle, CrDeviceProperty_TimeCodeFormat, "ProjectRate(TimeCodeFormat)", false, 0);

    std::cout << "\n🔌 Disconnecting..." << std::endl;
    SCRSDK::Disconnect(device_handle);
    camInfo->Release();
    SCRSDK::Release();
    std::cout << "✅ Test completed!" << std::endl;
    return 0;
}
