#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <cstring>
#include <cstdlib>
#include <arpa/inet.h>
#include "CRSDK/IDeviceCallback.h"
#include "CRSDK/CameraRemote_SDK.h"
#include "CRSDK/CrDeviceProperty.h"

using namespace std;

namespace {
struct DeviceCallbackImpl : public SCRSDK::IDeviceCallback {
    void OnConnected(SCRSDK::DeviceConnectionVersioin) override {}
    void OnDisconnected(CrInt32u) override {}
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
}

int main() {
    cout << "🧪 Sony Camera SDK Connection Test" << endl;
    cout << "===================================" << endl;

    // Initialize SDK
    if (!SCRSDK::Init()) {
        cout << "❌ Failed to initialize Sony CRSDK" << endl;
        return 1;
    }
    cout << "✅ Sony CRSDK initialized successfully" << endl;

    // Connect to camera
    const char* ip_env = std::getenv("SONY_CAMERA_IP");
    string ip = ip_env && ip_env[0] ? ip_env : "192.168.1.110";
    unsigned int ip_octets[4] = {0};
    if (std::sscanf(ip.c_str(), "%u.%u.%u.%u",
                    &ip_octets[0], &ip_octets[1], &ip_octets[2], &ip_octets[3]) != 4 ||
        ip_octets[0] > 255 || ip_octets[1] > 255 || ip_octets[2] > 255 || ip_octets[3] > 255) {
        cout << "❌ Invalid IP address: " << ip << endl;
        return 1;
    }
    // CRSDK expects a packed value where the first octet is bits 7..0.
    CrInt32u ipAddr = (CrInt32u)(ip_octets[0] | (ip_octets[1] << 8) | (ip_octets[2] << 16) | (ip_octets[3] << 24));
    
    CrInt8u macBuf[6] = {0};
    const char* mac_env = std::getenv("SONY_CAMERA_MAC");
    if (mac_env && mac_env[0]) {
        unsigned int ma[6] = {0};
        if (std::sscanf(mac_env, "%x:%x:%x:%x:%x:%x",
                       &ma[0], &ma[1], &ma[2], &ma[3], &ma[4], &ma[5]) == 6) {
            for (int i = 0; i < 6; ++i) {
                macBuf[i] = (CrInt8u)(ma[i] & 0xFF);
            }
            cout << "✅ Using MAC from SONY_CAMERA_MAC=" << mac_env << endl;
        }
    }
    SCRSDK::ICrCameraObjectInfo* pCamera = nullptr;
    
    auto err = SCRSDK::CreateCameraObjectInfoEthernetConnection(&pCamera, 
        SCRSDK::CrCameraDeviceModelList::CrCameraDeviceModel_MPC_2610, 
        ipAddr, macBuf, 1);
    
    if (CR_FAILED(err) || !pCamera) {
        cout << "❌ Failed to create camera connection. Error: 0x" << hex << err << dec << endl;
        SCRSDK::Release();
        return 1;
    }
    
    cout << "✅ Camera object created successfully!" << endl;
    cout << "   Model: " << (pCamera->GetModel() ? pCamera->GetModel() : "Unknown") << endl;
    cout << "   Name: " << (pCamera->GetName() ? pCamera->GetName() : "Unknown") << endl;
    
    // Test fingerprint
    char fingerprint[4096] = {0};
    CrInt32u fpSize = (CrInt32u)sizeof(fingerprint);
    SCRSDK::CrError fpResult = SCRSDK::GetFingerprint(pCamera, fingerprint, &fpSize);
    
    std::string fp_norm;
    if (CR_SUCCEEDED(fpResult) && fpSize > 0) {
        fp_norm.assign(fingerprint, fingerprint + fpSize);
        while (fp_norm.size() % 4 != 0) {
            fp_norm.push_back('=');
        }
        cout << "🔐 Fingerprint available (length: " << fpSize << ", padded: " << fp_norm.size() << ")" << endl;
        cout << "🔐 Fingerprint (padded): " << fp_norm << endl;
    } else {
        cout << "⚠️  No fingerprint or fingerprint failed: 0x" << hex << fpResult << dec << endl;
    }
    
    const char* user_env = std::getenv("SONY_USER");
    const char* pass_env = std::getenv("SONY_PASS");
    const char* accept_fp_env = std::getenv("SONY_ACCEPT_FINGERPRINT");
    const char* fp_env = std::getenv("SONY_FINGERPRINT");
    const char* user = (user_env && user_env[0]) ? user_env : "admin";
    const char* fp_ptr = nullptr;
    CrInt32u fp_len = 0;
    std::string fp_env_norm;
    if (fp_env && fp_env[0]) {
        fp_env_norm.assign(fp_env, fp_env + std::strlen(fp_env));
        while (fp_env_norm.size() % 4 != 0) {
            fp_env_norm.push_back('=');
        }
        fp_ptr = fp_env_norm.c_str();
        fp_len = (CrInt32u)fp_env_norm.size();
        cout << "🔐 Using fingerprint from SONY_FINGERPRINT (length: " << fp_len << ")" << endl;
    } else if (accept_fp_env && accept_fp_env[0] == '1' && fpSize > 0) {
        fp_ptr = fp_norm.c_str();
        fp_len = (CrInt32u)fp_norm.size();
        cout << "🔐 Using fingerprint from GetFingerprint (length: " << fp_len << ")" << endl;
    }

    // Try different connection approaches
    cout << "\n🔧 Testing Connection Methods:" << endl;
    
    SCRSDK::CrDeviceHandle deviceHandle = 0;
    DeviceCallbackImpl callback;
    
    // Method 1: Simple connect without auth
    cout << "1. Trying simple connection without authentication..." << endl;
    SCRSDK::CrError result1 = SCRSDK::Connect(pCamera, &callback, &deviceHandle, 
        SCRSDK::CrSdkControlMode_Remote, SCRSDK::CrReconnecting_OFF, 
        "", "", "", 0);
    cout << "   Result: 0x" << hex << result1 << dec << " (deviceHandle: " << deviceHandle << ")" << endl;
    
    if (deviceHandle == 0) {
        // Method 3: With password
        cout << "3. Trying with user/password..." << endl;
        result1 = SCRSDK::Connect(pCamera, &callback, &deviceHandle, 
            SCRSDK::CrSdkControlMode_Remote, SCRSDK::CrReconnecting_OFF, 
            user, pass_env ? pass_env : "", fp_ptr, fp_len);
        cout << "   Result: 0x" << hex << result1 << dec << " (deviceHandle: " << deviceHandle << ")" << endl;
    }
    
    if (deviceHandle == 0) {
        // Method 4: ContentsTransfer mode
        cout << "4. Trying ContentsTransfer mode..." << endl;
        result1 = SCRSDK::Connect(pCamera, &callback, &deviceHandle, 
            SCRSDK::CrSdkControlMode_ContentsTransfer, SCRSDK::CrReconnecting_OFF, 
            user, pass_env ? pass_env : "", fp_ptr, fp_len);
        cout << "   Result: 0x" << hex << result1 << dec << " (deviceHandle: " << deviceHandle << ")" << endl;
    }
    
    if (deviceHandle != 0) {
        cout << "\n✅ CONNECTION SUCCESS! Device handle: " << deviceHandle << endl;
        
        // ISO test (set to 800, then read back if available)
        cout << "\n🎚️  Testing ISO set (800)..." << endl;
        SCRSDK::CrDeviceProperty iso_prop;
        iso_prop.SetCode(SCRSDK::CrDevicePropertyCode::CrDeviceProperty_IsoSensitivity);
        iso_prop.SetValueType(SCRSDK::CrDataType_UInt32Array);
        const CrInt32u iso_value = (SCRSDK::CrISO_Normal << 24) | 800u;
        iso_prop.SetCurrentValue(iso_value);
        SCRSDK::CrError iso_set = SCRSDK::SetDeviceProperty(deviceHandle, &iso_prop);
        cout << "ISO set result: 0x" << hex << iso_set << dec << endl;

        // Read back ISO if present in device property list
        {
            SCRSDK::CrDeviceProperty* props = nullptr;
            CrInt32 num_props = 0;
            SCRSDK::CrError iso_get = SCRSDK::GetDeviceProperties(deviceHandle, &props, &num_props);
            if (CR_SUCCEEDED(iso_get) && props && num_props > 0) {
                bool found = false;
                for (CrInt32 i = 0; i < num_props; ++i) {
                    if (props[i].GetCode() == SCRSDK::CrDevicePropertyCode::CrDeviceProperty_IsoSensitivity) {
                        found = true;
                        auto* values = reinterpret_cast<CrInt32u*>(props[i].GetValues());
                        if (values && props[i].GetValueSize() >= sizeof(CrInt32u)) {
                            CrInt32u raw = values[0];
                            CrInt32u iso = raw & 0xFFFFFFu;
                            cout << "ISO readback raw=0x" << hex << raw << dec << " iso=" << iso << endl;
                        } else {
                            cout << "ISO readback empty" << endl;
                        }
                        break;
                    }
                }
                if (!found) {
                    cout << "ISO property not found in list" << endl;
                }
                SCRSDK::ReleaseDeviceProperties(deviceHandle, props);
            } else {
                cout << "ISO readback failed: 0x" << hex << iso_get << dec << endl;
                if (props) {
                    SCRSDK::ReleaseDeviceProperties(deviceHandle, props);
                }
            }
        }

        // Frame rate test (set to 24p, then read back if available)
        cout << "\n🎞️  Testing frame rate set (24p)..." << endl;
        SCRSDK::CrDeviceProperty fps_prop;
        fps_prop.SetCode(SCRSDK::CrDevicePropertyCode::CrDeviceProperty_Movie_Recording_FrameRateSetting);
        fps_prop.SetValueType(SCRSDK::CrDataType_UInt8Array);
        const CrInt32u fps_value = SCRSDK::CrRecordingFrameRateSettingMovie_24p;
        fps_prop.SetCurrentValue(fps_value);
        SCRSDK::CrError fps_set = SCRSDK::SetDeviceProperty(deviceHandle, &fps_prop);
        cout << "Frame rate set result: 0x" << hex << fps_set << dec << endl;

        // Read back frame rate if present in device property list
        {
            SCRSDK::CrDeviceProperty* props = nullptr;
            CrInt32 num_props = 0;
            SCRSDK::CrError fps_get = SCRSDK::GetDeviceProperties(deviceHandle, &props, &num_props);
            if (CR_SUCCEEDED(fps_get) && props && num_props > 0) {
                bool found = false;
                for (CrInt32 i = 0; i < num_props; ++i) {
                    if (props[i].GetCode() == SCRSDK::CrDevicePropertyCode::CrDeviceProperty_Movie_Recording_FrameRateSetting) {
                        found = true;
                        auto* values = props[i].GetValues();
                        if (values && props[i].GetValueSize() >= sizeof(CrInt8u)) {
                            CrInt8u raw = values[0];
                            cout << "Frame rate readback raw=0x" << hex << (int)raw << dec << endl;
                        } else {
                            cout << "Frame rate readback empty" << endl;
                        }
                        break;
                    }
                }
                if (!found) {
                    cout << "Frame rate property not found in list" << endl;
                }
                SCRSDK::ReleaseDeviceProperties(deviceHandle, props);
            } else {
                cout << "Frame rate readback failed: 0x" << hex << fps_get << dec << endl;
                if (props) {
                    SCRSDK::ReleaseDeviceProperties(deviceHandle, props);
                }
            }
        }

        // Try sending a toggle record command (Down -> Up)
        cout << "\n📷 Testing camera commands..." << endl;

        auto has_property = [&](SCRSDK::CrDevicePropertyCode code) {
            SCRSDK::CrDeviceProperty* props = nullptr;
            CrInt32 num_props = 0;
            SCRSDK::CrError result = SCRSDK::GetDeviceProperties(deviceHandle, &props, &num_props);
            if (CR_FAILED(result) || !props || num_props <= 0) {
                if (props) {
                    SCRSDK::ReleaseDeviceProperties(deviceHandle, props);
                }
                return false;
            }
            bool found = false;
            for (CrInt32 i = 0; i < num_props; ++i) {
                if (props[i].GetCode() == static_cast<CrInt32u>(code)) {
                    found = true;
                    break;
                }
            }
            SCRSDK::ReleaseDeviceProperties(deviceHandle, props);
            return found;
        };

        auto read_u8_property = [&](SCRSDK::CrDevicePropertyCode code, const char* label) {
            SCRSDK::CrDeviceProperty* props = nullptr;
            CrInt32 num_props = 0;
            SCRSDK::CrError result = SCRSDK::GetDeviceProperties(deviceHandle, &props, &num_props);
            if (CR_FAILED(result) || !props || num_props <= 0) {
                cout << label << " read failed: 0x" << hex << result << dec << endl;
                if (props) {
                    SCRSDK::ReleaseDeviceProperties(deviceHandle, props);
                }
                return (CrInt32u)0xFFFFFFFFu;
            }

            CrInt32u value = 0xFFFFFFFFu;
            bool found = false;
            for (CrInt32 i = 0; i < num_props; ++i) {
                if (props[i].GetCode() == static_cast<CrInt32u>(code)) {
                    found = true;
                    auto* values = props[i].GetValues();
                    if (values && props[i].GetValueSize() >= 1) {
                        value = values[0];
                        cout << label << " = " << (int)value << endl;
                    } else {
                        cout << label << " read empty" << endl;
                    }
                    break;
                }
            }

            if (!found) {
                cout << label << " not found in property list" << endl;
            }
            SCRSDK::ReleaseDeviceProperties(deviceHandle, props);
            return value;
        };

        const bool has_toggle_enable = has_property(SCRSDK::CrDevicePropertyCode::CrDeviceProperty_MovieRecButtonToggleEnableStatus);
        const bool has_stream_button = has_property(SCRSDK::CrDevicePropertyCode::CrDeviceProperty_StreamButtonEnableStatus);
        const bool has_stream_status = has_property(SCRSDK::CrDevicePropertyCode::CrDeviceProperty_StreamStatus);
        const bool has_stream_rec_perm = has_property(SCRSDK::CrDevicePropertyCode::CrDeviceProperty_VideoStreamMovieRecPermission);
        read_u8_property(SCRSDK::CrDevicePropertyCode::CrDeviceProperty_MovieRecButtonToggleEnableStatus,
            "Movie Rec Button Toggle status");
        read_u8_property(SCRSDK::CrDevicePropertyCode::CrDeviceProperty_RecordingState,
            "Recording state");
        if (has_stream_button) {
            read_u8_property(SCRSDK::CrDevicePropertyCode::CrDeviceProperty_StreamButtonEnableStatus,
                "Stream Button Enable status");
        }
        if (has_stream_status) {
            read_u8_property(SCRSDK::CrDevicePropertyCode::CrDeviceProperty_StreamStatus,
                "Stream status");
        }
        if (has_stream_rec_perm) {
            read_u8_property(SCRSDK::CrDevicePropertyCode::CrDeviceProperty_VideoStreamMovieRecPermission,
                "Stream Movie Rec Permission");
        }

        if (has_toggle_enable) {
            SCRSDK::CrDeviceProperty enable_prop;
            enable_prop.SetCode(SCRSDK::CrDevicePropertyCode::CrDeviceProperty_MovieRecButtonToggleEnableStatus);
            enable_prop.SetValueType(SCRSDK::CrDataType_UInt8);
            enable_prop.SetCurrentValue(SCRSDK::CrMovieRecButtonToggle_Enable);
            SCRSDK::CrError enable_result = SCRSDK::SetDeviceProperty(deviceHandle, &enable_prop);
            cout << "Movie Rec Button Toggle enable result: 0x" << hex << enable_result << dec << endl;

            read_u8_property(SCRSDK::CrDevicePropertyCode::CrDeviceProperty_MovieRecButtonToggleEnableStatus,
                "Movie Rec Button Toggle status (after set)");
        } else {
            cout << "Movie Rec Button Toggle enable property not supported; skipping enable" << endl;
        }

        auto send_toggle = [&](SCRSDK::CrCommandId cmd_id, const char* label) {
            cout << label << " (Down -> Up)..." << endl;
            SCRSDK::CrError cmdDown = SCRSDK::SendCommand(deviceHandle,
                cmd_id,
                SCRSDK::CrCommandParam::CrCommandParam_Down);
            std::this_thread::sleep_for(std::chrono::milliseconds(150));
            SCRSDK::CrError cmdUp = SCRSDK::SendCommand(deviceHandle,
                cmd_id,
                SCRSDK::CrCommandParam::CrCommandParam_Up);
            cout << "Toggle Down result: 0x" << hex << cmdDown << dec << endl;
            cout << "Toggle Up result: 0x" << hex << cmdUp << dec << endl;
            return std::pair<SCRSDK::CrError, SCRSDK::CrError>(cmdDown, cmdUp);
        };

        auto send_toggle_with_retry = [&](SCRSDK::CrCommandId cmd_id,
                                          const char* label,
                                          int attempts,
                                          int delay_ms) {
            std::pair<SCRSDK::CrError, SCRSDK::CrError> result{0xFFFFFFFF, 0xFFFFFFFF};
            for (int attempt = 1; attempt <= attempts; ++attempt) {
                if (attempt > 1) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
                }
                cout << label << " attempt " << attempt << "/" << attempts << endl;
                result = send_toggle(cmd_id, label);
                if (result.first != 0x8402 && result.second != 0x8402) {
                    break;
                }
            }
            return result;
        };

        std::this_thread::sleep_for(std::chrono::seconds(2));

        cout << "Record start" << endl;
        auto start_result = send_toggle_with_retry(
            SCRSDK::CrCommandId::CrCommandId_MovieRecButtonToggle,
            "Record start toggle (MovieRecButtonToggle)",
            3,
            750);
        if (start_result.first == 0x8402 || start_result.second == 0x8402) {
            send_toggle_with_retry(
                SCRSDK::CrCommandId::CrCommandId_MovieRecButtonToggle2,
                "Record start toggle (MovieRecButtonToggle2)",
                2,
                750);
            cout << "Record start fallback (MovieRecord Down -> Up)..." << endl;
            send_toggle_with_retry(
                SCRSDK::CrCommandId::CrCommandId_MovieRecord,
                "Record start (MovieRecord)",
                2,
                750);
            if (has_stream_button) {
                cout << "Record start fallback (StreamButton Down -> Up)..." << endl;
                send_toggle_with_retry(
                    SCRSDK::CrCommandId::CrCommandId_StreamButton,
                    "Record start (StreamButton)",
                    2,
                    750);
            }
        }

        std::this_thread::sleep_for(std::chrono::seconds(2));

        read_u8_property(SCRSDK::CrDevicePropertyCode::CrDeviceProperty_RecordingState,
            "Recording state (after start)");

        cout << "Record stop" << endl;
        auto stop_result = send_toggle_with_retry(
            SCRSDK::CrCommandId::CrCommandId_MovieRecButtonToggle,
            "Record stop toggle (MovieRecButtonToggle)",
            2,
            500);
        if (stop_result.first == 0x8402 || stop_result.second == 0x8402) {
            send_toggle_with_retry(
                SCRSDK::CrCommandId::CrCommandId_MovieRecButtonToggle2,
                "Record stop toggle (MovieRecButtonToggle2)",
                2,
                500);
            cout << "Record stop fallback (MovieRecord Down -> Up)..." << endl;
            send_toggle_with_retry(
                SCRSDK::CrCommandId::CrCommandId_MovieRecord,
                "Record stop (MovieRecord)",
                2,
                500);
            if (has_stream_button) {
                cout << "Record stop fallback (StreamButton Down -> Up)..." << endl;
                send_toggle_with_retry(
                    SCRSDK::CrCommandId::CrCommandId_StreamButton,
                    "Record stop (StreamButton)",
                    2,
                    500);
            }
        }

        read_u8_property(SCRSDK::CrDevicePropertyCode::CrDeviceProperty_RecordingState,
            "Recording state (after stop)");
        
        // Cleanup
        SCRSDK::Disconnect(deviceHandle);
        SCRSDK::ReleaseDevice(deviceHandle);
    } else {
        cout << "\n❌ All connection methods failed" << endl;
        cout << "📝 Camera object works but authentication is required for commands" << endl;
    }
    
    // Cleanup
    pCamera->Release();
    SCRSDK::Release();
    
    return 0;
}