#include "sony_backend.hpp"
#include "CRSDK/IDeviceCallback.h"
#include "CrDebugString.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <thread>
#include <chrono>
#include <string>
#include <cctype>
#include <arpa/inet.h>

#define MSEARCH_ENB  // Enable camera enumeration like RemoteCli

// Minimal device callback implementation (no-op) copied signatures from sample
// to ensure we pass a valid callback object to SCRSDK::Connect.
namespace {
static const char* warning_name(CrInt32u warning) {
  switch (warning) {
    case SCRSDK::CrNotify_Captured_Event: return "CrNotify_Captured_Event";
    case SCRSDK::CrWarning_MovieRecordingOperation_Result_OK: return "CrWarning_MovieRecordingOperation_Result_OK";
    case SCRSDK::CrWarning_MovieRecordingOperation_Result_NG: return "CrWarning_MovieRecordingOperation_Result_NG";
    case SCRSDK::CrWarning_MovieRecordingOperation_Result_Invalid: return "CrWarning_MovieRecordingOperation_Result_Invalid";
    default: return "(unknown)";
  }
}

struct DeviceCallbackImpl : public SCRSDK::IDeviceCallback {
  // Inherited via IDeviceCallback - log events for debugging
  virtual void OnConnected(SCRSDK::DeviceConnectionVersioin version) override {
    std::printf("[DeviceCallback] OnConnected(version=%d)\n", (int)version);
  }
  virtual void OnDisconnected(CrInt32u error) override {
    std::printf("[DeviceCallback] OnDisconnected(error=0x%08X)\n", (unsigned)error);
  }
  virtual void OnPropertyChanged() override { std::printf("[DeviceCallback] OnPropertyChanged\n"); }
  virtual void OnLvPropertyChanged() override { std::printf("[DeviceCallback] OnLvPropertyChanged\n"); }
  virtual void OnCompleteDownload(CrChar* filename, CrInt32u type) override { std::printf("[DeviceCallback] OnCompleteDownload(filename=%s,type=%u)\n", filename ? filename : "(null)", (unsigned)type); }
  virtual void OnWarning(CrInt32u warning) override {
    std::printf("[DeviceCallback] OnWarning(0x%08X %s)\n", (unsigned)warning, warning_name(warning));
  }
  virtual void OnWarningExt(CrInt32u warning, CrInt32 param1, CrInt32 param2, CrInt32 param3) override {
    std::printf("[DeviceCallback] OnWarningExt(0x%08X %s) params=(%d,%d,%d)\n",
                (unsigned)warning, warning_name(warning), (int)param1, (int)param2, (int)param3);
  }
  virtual void OnError(CrInt32u error) override { std::printf("[DeviceCallback] OnError(0x%08X)\n", (unsigned)error); }
  virtual void OnPropertyChangedCodes(CrInt32u num, CrInt32u* codes) override { std::printf("[DeviceCallback] OnPropertyChangedCodes(num=%u)\n", (unsigned)num); }
  virtual void OnLvPropertyChangedCodes(CrInt32u num, CrInt32u* codes) override { std::printf("[DeviceCallback] OnLvPropertyChangedCodes(num=%u)\n", (unsigned)num); }
  virtual void OnNotifyContentsTransfer(CrInt32u notify, SCRSDK::CrContentHandle contentHandle, CrChar* filename) override { std::printf("[DeviceCallback] OnNotifyContentsTransfer(notify=%u,filename=%s)\n", (unsigned)notify, filename ? filename : "(null)"); }
  virtual void OnNotifyFTPTransferResult(CrInt32u notify, CrInt32u numOfSuccess, CrInt32u numOfFail) override { std::printf("[DeviceCallback] OnNotifyFTPTransferResult(%u,%u)\n", (unsigned)numOfSuccess, (unsigned)numOfFail); }
  virtual void OnNotifyRemoteTransferResult(CrInt32u notify, CrInt32u per, CrChar* filename) override { std::printf("[DeviceCallback] OnNotifyRemoteTransferResult(notify=%u,per=%u,filename=%s)\n", (unsigned)notify, (unsigned)per, filename ? filename : "(null)"); }
  virtual void OnNotifyRemoteTransferResult(CrInt32u notify, CrInt32u per, CrInt8u* data, CrInt64u size) override { std::printf("[DeviceCallback] OnNotifyRemoteTransferResultData(notify=%u,per=%u,size=%llu)\n", (unsigned)notify, (unsigned)per, (unsigned long long)size); }
  virtual void OnNotifyRemoteTransferContentsListChanged(CrInt32u notify, CrInt32u slotNumber, CrInt32u addSize) override { std::printf("[DeviceCallback] OnNotifyRemoteTransferContentsListChanged(notify=%u,slot=%u,add=%u)\n", (unsigned)notify, (unsigned)slotNumber, (unsigned)addSize); }
  virtual void OnReceivePlaybackTimeCode(CrInt32u timeCode) override { std::printf("[DeviceCallback] OnReceivePlaybackTimeCode(%u)\n", (unsigned)timeCode); }
  virtual void OnReceivePlaybackData(CrInt8u mediaType, CrInt32 dataSize, CrInt8u* data, CrInt64 pts, CrInt64 dts, CrInt32 param1, CrInt32 param2) override { std::printf("[DeviceCallback] OnReceivePlaybackData(type=%u,size=%d)\n", (unsigned)mediaType, (int)dataSize); }
  virtual void OnNotifyRemoteFirmwareUpdateResult(CrInt32u notify, const void* param) override { std::printf("[DeviceCallback] OnNotifyRemoteFirmwareUpdateResult(notify=%u)\n", (unsigned)notify); }
};
} // anonymous namespace

// Map a CrError category to human-friendly string for logging
static const char* crerror_category(SCRSDK::CrError err) {
  unsigned cat = ((unsigned)err) & 0xFF00u;
  switch (cat) {
    case SCRSDK::CrError_Connect: return "Connect";
    case SCRSDK::CrError_Adaptor: return "Adaptor";
    case SCRSDK::CrError_Api: return "Api";
    case SCRSDK::CrError_Init: return "Init";
    case SCRSDK::CrError_File: return "File";
    case SCRSDK::CrError_Memory: return "Memory";
    default: return "Unknown";
  }
}

// A74 recording behavior freeze (validated on hardware):
// - Date: 2026-02-08
// - Camera: ILCE-7M4 over USB via ccu_daemon
// - Known-good sequence:
//   1) Prepare PriorityKeySettings=PCRemote and ExposureProgramMode=Movie_P
//   2) Send MovieRecord Down(start) / Up(stop)
//   3) If API returns InvalidCalled (0x8402), fallback through toggle commands
// Do not alter command ordering/semantics without re-validating on hardware.
static constexpr const char* kA74FrozenModel = "ILCE-7M4";
static constexpr SCRSDK::CrError kA74InvalidCalled = SCRSDK::CrError_Api_InvalidCalled;

static std::string normalize_fingerprint(const char* data, CrInt32u len) {
  if (!data || len == 0) return std::string();
  std::string out(data, data + len);
  while (out.size() % 4 != 0) {
    out.push_back('=');
  }
  return out;
}

static size_t element_size(SCRSDK::CrDataType type) {
  const CrInt32u base = (type & 0x0FFFu);
  switch (base) {
    case SCRSDK::CrDataType_UInt8:
    case SCRSDK::CrDataType_Int8:
      return 1;
    case SCRSDK::CrDataType_UInt16:
    case SCRSDK::CrDataType_Int16:
      return 2;
    case SCRSDK::CrDataType_UInt32:
    case SCRSDK::CrDataType_Int32:
      return 4;
    case SCRSDK::CrDataType_UInt64:
    case SCRSDK::CrDataType_Int64:
      return 8;
    default:
      return 0;
  }
}

static bool get_prop_first_value(const SCRSDK::CrDeviceProperty& prop, uint32_t& out) {
  // For status polling we need the current camera value, not the first entry
  // from the value list (which is often just the first enum option).
  out = (uint32_t)prop.GetCurrentValue();
  return true;
}

static bool read_recording_flags(SCRSDK::CrDeviceHandle device_handle,
                                 uint32_t& recording_state,
                                 uint32_t& recorder_main_status,
                                 bool& is_recording) {
  recording_state = 0xFFFFFFFFu;
  recorder_main_status = 0xFFFFFFFFu;
  is_recording = false;

  SCRSDK::CrDeviceProperty* props = nullptr;
  CrInt32 num_props = 0;
  CrInt32u codes[2] = {
    SCRSDK::CrDeviceProperty_RecordingState,
    SCRSDK::CrDeviceProperty_RecorderMainStatus
  };

  const auto err = SCRSDK::GetSelectDeviceProperties(device_handle, 2, codes, &props, &num_props);
  if (CR_FAILED(err) || !props || num_props <= 0) {
    if (props) SCRSDK::ReleaseDeviceProperties(device_handle, props);
    return false;
  }

  for (CrInt32 i = 0; i < num_props; ++i) {
    uint32_t v = 0xFFFFFFFFu;
    if (!get_prop_first_value(props[i], v)) continue;

    if (props[i].GetCode() == SCRSDK::CrDeviceProperty_RecordingState) {
      recording_state = v;
    } else if (props[i].GetCode() == SCRSDK::CrDeviceProperty_RecorderMainStatus) {
      recorder_main_status = v;
    }
  }

  SCRSDK::ReleaseDeviceProperties(device_handle, props);

  const bool rec_state_known = (recording_state != 0xFFFFFFFFu);
  const bool rec_main_known = (recorder_main_status != 0xFFFFFFFFu);
  if (!rec_state_known && !rec_main_known) return false;

  is_recording = ((rec_state_known && recording_state != 0u) ||
                  (rec_main_known && recorder_main_status != 0u));
  return true;
}

static void try_prepare_recording_mode(SCRSDK::CrDeviceHandle device_handle) {
  // Best-effort: some bodies reject MovieRecord until PC Remote priority/mode
  // is asserted.
  SCRSDK::CrDeviceProperty priority;
  priority.SetCode(SCRSDK::CrDevicePropertyCode::CrDeviceProperty_PriorityKeySettings);
  priority.SetCurrentValue(SCRSDK::CrPriorityKeySettings::CrPriorityKey_PCRemote);
  priority.SetValueType(SCRSDK::CrDataType::CrDataType_UInt32Array);
  auto st_priority = SCRSDK::SetDeviceProperty(device_handle, &priority);
  std::printf("[SonyBackend] prepare_recording: PriorityKeySettings=PCRemote st=0x%08X\n",
              (unsigned)st_priority);

  SCRSDK::CrDeviceProperty exp_mode;
  exp_mode.SetCode(SCRSDK::CrDevicePropertyCode::CrDeviceProperty_ExposureProgramMode);
  exp_mode.SetCurrentValue(SCRSDK::CrExposureProgram::CrExposure_Movie_P);
  exp_mode.SetValueType(SCRSDK::CrDataType::CrDataType_UInt16Array);
  auto st_exp = SCRSDK::SetDeviceProperty(device_handle, &exp_mode);
  std::printf("[SonyBackend] prepare_recording: ExposureProgramMode=Movie_P st=0x%08X\n",
              (unsigned)st_exp);
}

static bool connect_camera(SCRSDK::ICrCameraObjectInfo* cam,
                           SCRSDK::IDeviceCallback* cb,
                           const char* user,
                           const char* pass,
                           const char* fingerprint,
                           CrInt32u fp_size,
                           SCRSDK::CrDeviceHandle* out_handle) {
  if (!out_handle) return false;
  *out_handle = 0;
  auto err = SCRSDK::Connect(cam, cb, out_handle,
                             SCRSDK::CrSdkControlMode_Remote,
                             SCRSDK::CrReconnecting_ON,
                             user, pass, fingerprint, fp_size);
  if (!CR_FAILED(err) && *out_handle != 0) {
    std::printf("[SonyBackend] Connect succeeded (handle=%lld)\n", (long long)*out_handle);
    return true;
  }
  std::printf("[SonyBackend] Connect failed (0x%08X) category=%s\n",
              (unsigned)err, crerror_category(err));
  return false;
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

namespace ccu {

SonyBackend::~SonyBackend() {
  if (m_device_handle != 0) {
    SCRSDK::Disconnect(m_device_handle);
    SCRSDK::ReleaseDevice(m_device_handle);
    m_device_handle = 0;
  }
  if (m_callback_impl) {
    delete static_cast<DeviceCallbackImpl*>(m_callback_impl);
    m_callback_impl = nullptr;
  }
}

bool SonyBackend::connect_first_camera() {
  std::printf("[SonyBackend] connect_first_camera() called (is_connected=%d)\n", is_connected() ? 1 : 0);
  if (is_connected()) return true;

  // 1) Init once - match RemoteCli exactly (no parameters)
  if (!m_inited) {
    const bool ok = SCRSDK::Init();
    std::printf("[SonyBackend] Init() returned: %d\n", ok ? 1 : 0);
    if (!ok) return false;
    m_inited = true;
  }

  const char* dbg_ip = std::getenv("SONY_CAMERA_IP");
  const char* dbg_accept = std::getenv("SONY_ACCEPT_FINGERPRINT");
  const char* dbg_pass = std::getenv("SONY_PASS");
  const char* dbg_user = std::getenv("SONY_USER");
  std::printf("[SonyBackend] Env SONY_CAMERA_IP=%s SONY_ACCEPT_FINGERPRINT=%s SONY_PASS=%s SONY_USER=%s\n",
             dbg_ip ? dbg_ip : "(unset)",
             dbg_accept ? dbg_accept : "(unset)",
             dbg_pass ? "(set)" : "(unset)",
             dbg_user ? dbg_user : "(unset)");

  // If the user provided SONY_CAMERA_IP, try direct IP connection first (deterministic path)
  const char* cam_ip_env = std::getenv("SONY_CAMERA_IP");
  if (cam_ip_env && cam_ip_env[0]) {
    std::printf("[SonyBackend] Attempting direct IP camera info via SONY_CAMERA_IP=%s\n", cam_ip_env);
    // Parse IP using inet_pton and convert to CRSDK-packed value
    in_addr ina;
    if (inet_pton(AF_INET, cam_ip_env, &ina) == 1) {
      SCRSDK::ICrCameraObjectInfo* pCam = nullptr;
      CrInt32u ipAddr = (CrInt32u)ntohl(ina.s_addr); // CRSDK expects first octet in bits 7..0
      std::printf("[SonyBackend] CreateCameraObjectInfoEthernetConnection(model=CrCameraDeviceModel_ILME_FX6 ip=%s numeric=%u)...\n", cam_ip_env, (unsigned)ipAddr);
      CrInt8u macBuf[6] = {0};
      // Try to obtain MAC from env (SONY_CAMERA_MAC) or ARP table for the IP
      const char* env_mac = std::getenv("SONY_CAMERA_MAC");
      if (env_mac && env_mac[0]) {
        unsigned int ma[6] = {0};
        if (std::sscanf(env_mac, "%x:%x:%x:%x:%x:%x", &ma[0], &ma[1], &ma[2], &ma[3], &ma[4], &ma[5]) == 6) {
          for (int i = 0; i < 6; ++i) macBuf[i] = (CrInt8u)(ma[i] & 0xFF);
          std::printf("[SonyBackend] Using SONY_CAMERA_MAC=%s\n", env_mac);
        }
      } else {
        // try to populate ARP entry then 'ip neigh show <ip>' to get MAC
        {
          std::string pingcmd = std::string("ping -c1 -W1 ") + cam_ip_env + " > /dev/null 2>&1";
          system(pingcmd.c_str());
        }
        std::string cmd = std::string("ip neigh show ") + cam_ip_env + " 2>/dev/null";
        FILE* f = popen(cmd.c_str(), "r");
        if (f) {
          char buf[256];
          if (fgets(buf, sizeof(buf), f)) {
            unsigned int ma[6] = {0};
            // Typical output: "192.168.33.94 dev eth0 lladdr ac:80:0a:39:29:84 STALE"
            if (std::sscanf(buf, "%*s dev %*s lladdr %x:%x:%x:%x:%x:%x", &ma[0], &ma[1], &ma[2], &ma[3], &ma[4], &ma[5]) == 6) {
              for (int i = 0; i < 6; ++i) macBuf[i] = (CrInt8u)(ma[i] & 0xFF);
              std::printf("[SonyBackend] Found MAC via 'ip neigh': %02X:%02X:%02X:%02X:%02X:%02X\n", macBuf[0], macBuf[1], macBuf[2], macBuf[3], macBuf[4], macBuf[5]);
            }
          }
          pclose(f);
        }
      }

      auto err = SCRSDK::CreateCameraObjectInfoEthernetConnection(&pCam, SCRSDK::CrCameraDeviceModelList::CrCameraDeviceModel_MPC_2610, ipAddr, macBuf, 1);
      if (!CR_FAILED(err) && pCam) {
        std::printf("[SonyBackend] Created camera object for IP %s\n", cam_ip_env);
        SCRSDK::ICrCameraObjectInfo* camInfo = pCam;

        // Fingerprint
        char fingerprint[4096] = {0};
        CrInt32u fpSize = (CrInt32u)sizeof(fingerprint);
        SCRSDK::CrError fpSt = SCRSDK::GetFingerprint(camInfo, fingerprint, &fpSize);
        std::string fp_norm;
        if (CR_FAILED(fpSt) || fpSize == 0) {
          std::printf("[SonyBackend] GetFingerprint failed (0x%08X)\n", (unsigned)fpSt);
          fpSize = 0; // still attempt connect
        } else {
          fp_norm = normalize_fingerprint(fingerprint, fpSize);
          std::printf("[SonyBackend] Fingerprint OK (size=%u padded=%u)\n", (unsigned)fpSize, (unsigned)fp_norm.size());
          std::printf("[SonyBackend] fingerprint (padded):\n%s\n", fp_norm.c_str());
          const char* accept_fp = std::getenv("SONY_ACCEPT_FINGERPRINT");
          if (!(accept_fp && accept_fp[0] && accept_fp[0] == '1')) {
            std::printf("[SonyBackend] Fingerprint requires acceptance. Set SONY_ACCEPT_FINGERPRINT=1 to auto-accept and connect.\n");
            pCam->Release();
            return false;
          }
        }

        // Credentials: only password is required (match RemoteCli)
        const char* pass = std::getenv("SONY_PASS");
        if (!pass || !pass[0]) {
          std::printf("[SonyBackend] Missing SONY_PASS env var\n");
          pCam->Release();
          return false;
        }

        if (!m_callback_impl) m_callback_impl = static_cast<void*>(new DeviceCallbackImpl());
        auto* cb = static_cast<SCRSDK::IDeviceCallback*>(m_callback_impl);
        const char* user = std::getenv("SONY_USER");
        if (!user || !user[0]) user = nullptr;
        const char* accept_fp = std::getenv("SONY_ACCEPT_FINGERPRINT");
        const char* env_fp = std::getenv("SONY_FINGERPRINT");
        const CrInt32u env_fp_len = (env_fp && env_fp[0]) ? (CrInt32u)std::strlen(env_fp) : 0;
        CrInt32u fp_len = 0;
        const char* fp_ptr = nullptr;
        if (accept_fp && accept_fp[0] == '1') {
          fp_ptr = select_fingerprint(env_fp, env_fp_len, fingerprint, fpSize, &fp_len);
          if (env_fp && env_fp[0]) {
            std::printf("[SonyBackend] Using fingerprint from SONY_FINGERPRINT (len=%u)\n", (unsigned)fp_len);
          } else if (fp_ptr && fp_len > 0) {
            std::printf("[SonyBackend] Using fingerprint from GetFingerprint (len=%u)\n", (unsigned)fp_len);
          }
        }

        const int max_attempts = 5;
        bool cd_connected = false;
        for (int attempt = 1; attempt <= max_attempts; ++attempt) {
          SCRSDK::CrDeviceHandle h = 0;
          if (connect_camera(camInfo, cb, user, pass, fp_ptr, fp_len, &h)) {
            m_device_handle = h;
            m_connected = true;
            std::printf("[SonyBackend] Connect succeeded on attempt %d\n", attempt);
            cd_connected = true;
            break;
          }
          std::printf("[SonyBackend] Connect attempt %d/%d failed\n", attempt, max_attempts);
          if (attempt < max_attempts) std::this_thread::sleep_for(std::chrono::seconds(1 << (attempt-1)));
        }

        pCam->Release();
        if (cd_connected) {
          std::printf("[SonyBackend] Connected via direct CRSDK Connect!\n");
          return true;
        } else {
          std::printf("[SonyBackend] Connect failed after %d attempts\n", max_attempts);
          // fallthrough to enumeration
        }
      } else {
        std::printf("[SonyBackend] CreateCameraObjectInfoEthernetConnection failed (0x%08X) category=%s\n", (unsigned)err, crerror_category(err));

        // Try byte-order fallback: some SDKs expect host order for the IP integer
        CrInt32u ipHostOrder = (CrInt32u)ntohl(ina.s_addr);
        if (ipHostOrder != ipAddr) {
          std::printf("[SonyBackend] Trying CreateCameraObjectInfoEthernetConnection with IP host-order numeric=%u\n", (unsigned)ipHostOrder);
          SCRSDK::ICrCameraObjectInfo* pCam1b = nullptr;
          // For host-order attempt, reuse macBuf if populated, else zero
          CrInt8u macBuf1b[6] = {0};
          auto err1b = SCRSDK::CreateCameraObjectInfoEthernetConnection(&pCam1b, SCRSDK::CrCameraDeviceModelList::CrCameraDeviceModel_ILME_FX6, ipHostOrder, macBuf1b, 1);
          if (!CR_FAILED(err1b) && pCam1b) {
            std::printf("[SonyBackend] Created camera object for IP %s using host-order numeric=%u\n", cam_ip_env, (unsigned)ipHostOrder);
            // proceed as normal (reuse the pCam path) by swapping pCam to pCam1b
            SCRSDK::ICrCameraObjectInfo* camInfo = pCam1b;
            // fingerprint
            char fingerprint[4096] = {0};
            CrInt32u fpSize = (CrInt32u)sizeof(fingerprint);
            SCRSDK::CrError fpSt = SCRSDK::GetFingerprint(camInfo, fingerprint, &fpSize);
            if (CR_FAILED(fpSt) || fpSize == 0) {
              std::printf("[SonyBackend] GetFingerprint failed (0x%08X)\n", (unsigned)fpSt);
              fpSize = 0; // still attempt connect
            } else {
              if (fpSize < sizeof(fingerprint)) fingerprint[fpSize] = '\0';
              fingerprint[sizeof(fingerprint)-1] = '\0';
              std::printf("[SonyBackend] Fingerprint OK (size=%u)\n", (unsigned)fpSize);
              std::printf("[SonyBackend] fingerprint:\n%s\n", fingerprint);
              const char* accept_fp = std::getenv("SONY_ACCEPT_FINGERPRINT");
              if (!(accept_fp && accept_fp[0] && accept_fp[0] == '1')) {
                std::printf("[SonyBackend] Fingerprint requires acceptance. Set SONY_ACCEPT_FINGERPRINT=1 to auto-accept and connect.\n");
                pCam1b->Release();
                return false;
              }
            }

            const char* user = std::getenv("SONY_USER");
            const char* pass = std::getenv("SONY_PASS");
            if (!pass || !pass[0]) {
              std::printf("[SonyBackend] Missing SONY_PASS env var\n");
              pCam1b->Release();
              return false;
            }
            if (!user || !user[0]) user = nullptr; // match RemoteCli: no username, only password

            if (!m_callback_impl) m_callback_impl = static_cast<void*>(new DeviceCallbackImpl());
            auto* cb = static_cast<SCRSDK::IDeviceCallback*>(m_callback_impl);
            const char* accept_fp_env = std::getenv("SONY_ACCEPT_FINGERPRINT");
            const char* env_fp = std::getenv("SONY_FINGERPRINT");
            const CrInt32u env_fp_len = (env_fp && env_fp[0]) ? (CrInt32u)std::strlen(env_fp) : 0;
            CrInt32u fp_len = 0;
            const char* fp_ptr = nullptr;
            if (accept_fp_env && accept_fp_env[0] == '1') {
              fp_ptr = select_fingerprint(env_fp, env_fp_len, fingerprint, fpSize, &fp_len);
              if (env_fp && env_fp[0]) {
                std::printf("[SonyBackend] Using fingerprint from SONY_FINGERPRINT (len=%u)\n", (unsigned)fp_len);
              } else if (fp_ptr && fp_len > 0) {
                std::printf("[SonyBackend] Using fingerprint from GetFingerprint (len=%u)\n", (unsigned)fp_len);
              }
            }

            const int max_attempts_host = 5;
            bool cd_connected_host = false;
            for (int attempt = 1; attempt <= max_attempts_host; ++attempt) {
              SCRSDK::CrDeviceHandle h = 0;
              if (connect_camera(camInfo, cb, user, pass, fp_ptr, fp_len, &h)) {
                m_device_handle = h;
                m_connected = true;
                std::printf("[SonyBackend] Connect succeeded on attempt %d\n", attempt);
                cd_connected_host = true;
                break;
              }
              std::printf("[SonyBackend] Connect attempt %d/%d failed\n", attempt, max_attempts_host);
              if (attempt < max_attempts_host) std::this_thread::sleep_for(std::chrono::seconds(1 << (attempt-1)));
            }

            pCam1b->Release();
            if (cd_connected_host) {
              std::printf("[SonyBackend] Connected via direct CRSDK Connect (host-order IP)!\n");
              return true;
            } else {
              std::printf("[SonyBackend] Connect (host-order IP) failed after %d attempts\n", max_attempts_host);
              // fallthrough to earlier fallbacks
            }
          } else {
            std::printf("[SonyBackend] CreateCameraObjectInfoEthernetConnection with host-order IP failed (0x%08X) category=%s\n", (unsigned)err1b, crerror_category(err1b));
          }
        }

        std::printf("[SonyBackend] Trying fallback CreateCameraObjectInfoEthernetConnection with model=0 (unknown)...\n");
        SCRSDK::ICrCameraObjectInfo* pCam2 = nullptr;
        CrInt8u macBuf2[6] = {0};
        auto err2 = SCRSDK::CreateCameraObjectInfoEthernetConnection(&pCam2, (SCRSDK::CrCameraDeviceModelList)0, ipAddr, macBuf2, 1);
        if (!CR_FAILED(err2) && pCam2) {
          std::printf("[SonyBackend] Fallback created camera object for IP %s (model=0)\n", cam_ip_env);

          // fingerprint (fallback)
          char fingerprint2[4096] = {0};
          CrInt32u fpSize2 = (CrInt32u)sizeof(fingerprint2);
          SCRSDK::CrError fpSt2 = SCRSDK::GetFingerprint(pCam2, fingerprint2, &fpSize2);
          if (CR_FAILED(fpSt2) || fpSize2 == 0) {
            std::printf("[SonyBackend] GetFingerprint(fallback) failed (0x%08X)\n", (unsigned)fpSt2);
            fpSize2 = 0;
          } else {
            if (fpSize2 < sizeof(fingerprint2)) fingerprint2[fpSize2] = '\0';
            fingerprint2[sizeof(fingerprint2)-1] = '\0';
            std::printf("[SonyBackend] Fingerprint OK (size=%u)\n", (unsigned)fpSize2);
            std::printf("[SonyBackend] fingerprint:\n%s\n", fingerprint2);
            const char* accept_fp2 = std::getenv("SONY_ACCEPT_FINGERPRINT");
            if (!(accept_fp2 && accept_fp2[0] && accept_fp2[0] == '1')) {
              std::printf("[SonyBackend] Fingerprint requires acceptance. Set SONY_ACCEPT_FINGERPRINT=1 to auto-accept and connect.\n");
              pCam2->Release();
              return false;
            }
          }

          const char* user2 = std::getenv("SONY_USER");
          const char* pass2 = std::getenv("SONY_PASS");
          if (!pass2 || !pass2[0]) {
            std::printf("[SonyBackend] Missing SONY_PASS env var\n");
            pCam2->Release();
            return false;
          }
          if (!user2 || !user2[0]) user2 = nullptr;

          if (!m_callback_impl) m_callback_impl = static_cast<void*>(new DeviceCallbackImpl());
          auto* cb = static_cast<SCRSDK::IDeviceCallback*>(m_callback_impl);
          const char* user2_env = std::getenv("SONY_USER");
          if (!user2_env || !user2_env[0]) user2_env = nullptr;
          const char* accept_fp2 = std::getenv("SONY_ACCEPT_FINGERPRINT");
          const char* env_fp = std::getenv("SONY_FINGERPRINT");
          const CrInt32u env_fp_len = (env_fp && env_fp[0]) ? (CrInt32u)std::strlen(env_fp) : 0;
          CrInt32u fp_len2 = 0;
          const char* fp_ptr2 = nullptr;
          if (accept_fp2 && accept_fp2[0] == '1') {
            fp_ptr2 = select_fingerprint(env_fp, env_fp_len, fingerprint2, fpSize2, &fp_len2);
            if (env_fp && env_fp[0]) {
              std::printf("[SonyBackend] Using fingerprint from SONY_FINGERPRINT (len=%u)\n", (unsigned)fp_len2);
            } else if (fp_ptr2 && fp_len2 > 0) {
              std::printf("[SonyBackend] Using fingerprint from GetFingerprint (len=%u)\n", (unsigned)fp_len2);
            }
          }

          const int max_connect_attempts_ip2 = 3;
          bool cd_connected_fb = false;
          for (int attempt2 = 1; attempt2 <= max_connect_attempts_ip2; ++attempt2) {
            SCRSDK::CrDeviceHandle h = 0;
            if (connect_camera(pCam2, cb, user2_env, pass2, fp_ptr2, fp_len2, &h)) {
              m_device_handle = h;
              m_connected = true;
              std::printf("[SonyBackend] Connect (fallback) succeeded on attempt %d\n", attempt2);
              cd_connected_fb = true;
              break;
            }
            std::printf("[SonyBackend] Connect (fallback) attempt %d/%d failed\n", attempt2, max_connect_attempts_ip2);
            if (attempt2 < max_connect_attempts_ip2) std::this_thread::sleep_for(std::chrono::seconds(1 << (attempt2-1)));
          }

          pCam2->Release();
          if (cd_connected_fb) {
            std::printf("[SonyBackend] Connected via direct CRSDK Connect (fallback)!\n");
            return true;
          } else {
            std::printf("[SonyBackend] Connect (fallback) failed after %d attempts\n", max_connect_attempts_ip2);
            // fallthrough to enumeration
          }
        } else {
          std::printf("[SonyBackend] Fallback CreateCameraObjectInfoEthernetConnection failed (0x%08X) category=%s\n", (unsigned)err2, crerror_category(err2));

          std::printf("[SonyBackend] Scanning model enum values for CreateCameraObjectInfoEthernetConnection successes...\n");
          for (int cand = 1; cand <= 32; ++cand) {
            SCRSDK::ICrCameraObjectInfo* pCamC = nullptr;
            CrInt8u macBufC[6] = {0};
            auto errC = SCRSDK::CreateCameraObjectInfoEthernetConnection(&pCamC, (SCRSDK::CrCameraDeviceModelList)cand, ipAddr, macBufC, 1);
            if (!CR_FAILED(errC) && pCamC) {
              std::printf("[SonyBackend] Candidate model %d succeeded to create camera object (err=0x%08X).\n", cand, (unsigned)errC);

              // Try fingerprint + connect immediately for this candidate
              char fingerprintC[4096] = {0};
              CrInt32u fpSizeC = (CrInt32u)sizeof(fingerprintC);
              SCRSDK::CrError fpStC = SCRSDK::GetFingerprint(pCamC, fingerprintC, &fpSizeC);
              if (CR_FAILED(fpStC) || fpSizeC == 0) {
                std::printf("[SonyBackend] Candidate model %d GetFingerprint failed (0x%08X)\n", cand, (unsigned)fpStC);
                fpSizeC = 0;
              } else {
                if (fpSizeC < sizeof(fingerprintC)) fingerprintC[fpSizeC] = '\0';
                fingerprintC[sizeof(fingerprintC)-1] = '\0';
                std::printf("[SonyBackend] Candidate model %d fingerprint size=%u\n", cand, (unsigned)fpSizeC);
              }

              const char* userC = std::getenv("SONY_USER");
              const char* passC = std::getenv("SONY_PASS");
              if (!passC || !passC[0]) {
                std::printf("[SonyBackend] Missing SONY_PASS env var\n");
                pCamC->Release();
                return false;
              }
              if (!userC || !userC[0]) userC = nullptr;

              if (!m_callback_impl) m_callback_impl = static_cast<void*>(new DeviceCallbackImpl());
              auto* cb = static_cast<SCRSDK::IDeviceCallback*>(m_callback_impl);
              const char* userC_env = std::getenv("SONY_USER");
              if (!userC_env || !userC_env[0]) userC_env = nullptr;
              const char* accept_fpC = std::getenv("SONY_ACCEPT_FINGERPRINT");
              const char* env_fp = std::getenv("SONY_FINGERPRINT");
              const CrInt32u env_fp_len = (env_fp && env_fp[0]) ? (CrInt32u)std::strlen(env_fp) : 0;
              CrInt32u fp_lenC = 0;
              const char* fp_ptrC = nullptr;
              if (accept_fpC && accept_fpC[0] == '1') {
                fp_ptrC = select_fingerprint(env_fp, env_fp_len, fingerprintC, fpSizeC, &fp_lenC);
                if (env_fp && env_fp[0]) {
                  std::printf("[SonyBackend] Using fingerprint from SONY_FINGERPRINT (len=%u)\n", (unsigned)fp_lenC);
                } else if (fp_ptrC && fp_lenC > 0) {
                  std::printf("[SonyBackend] Using fingerprint from GetFingerprint (len=%u)\n", (unsigned)fp_lenC);
                }
              }

              SCRSDK::CrDeviceHandle h = 0;
              if (connect_camera(pCamC, cb, userC_env, passC, fp_ptrC, fp_lenC, &h)) {
                m_device_handle = h;
                std::printf("[SonyBackend] Candidate model %d Connect succeeded via direct CRSDK Connect!\n", cand);
                m_connected = true;
                pCamC->Release();
                return true;
              }
              std::printf("[SonyBackend] Candidate model %d Connect failed via direct CRSDK Connect\n", cand);

              pCamC->Release();
            } else {
              /* creation failed */
              //std::printf("[SonyBackend] Candidate model %d failed to create (0x%08X)\n", cand, (unsigned)errC);
            }
          }
          std::printf("[SonyBackend] Model scan complete, no candidate succeeded.\n");

          // Diagnostic: try a non-SSH direct connect (sshSupport=0) to see if non-SSH connects
          {
            std::printf("[SonyBackend] Attempting non-SSH direct connect (sshSupport=0) as diagnostic...\n");
            SCRSDK::ICrCameraObjectInfo* pCam_no_ssh = nullptr;
            CrInt8u macBufNoSsh[6] = {0};
            auto err_no_ssh = SCRSDK::CreateCameraObjectInfoEthernetConnection(&pCam_no_ssh, (SCRSDK::CrCameraDeviceModelList)0, ipAddr, macBufNoSsh, 0);
            if (!CR_FAILED(err_no_ssh) && pCam_no_ssh) {
              std::printf("[SonyBackend] Created non-SSH camera object (model=0) for IP %s\n", cam_ip_env);

              // Use a lightweight callback for diagnostic if not present
              if (!m_callback_impl) m_callback_impl = static_cast<void*>(new DeviceCallbackImpl());

              // Attempt Connect without fingerprint/password
              SCRSDK::CrDeviceHandle h = 0;
              SCRSDK::CrError st_no_ssh = SCRSDK::Connect(pCam_no_ssh, static_cast<SCRSDK::IDeviceCallback*>(m_callback_impl), &h, SCRSDK::CrSdkControlMode_Remote, SCRSDK::CrReconnecting_ON, nullptr, nullptr, nullptr, 0);
              if (!CR_FAILED(st_no_ssh) && h != 0) {
                std::printf("[SonyBackend] Non-SSH Connect succeeded!\n");
                m_device_handle = h;
                m_connected = true;
                pCam_no_ssh->Release();
                return true;
              } else {
                std::printf("[SonyBackend] Non-SSH Connect failed (0x%08X) category=%s\n", (unsigned)st_no_ssh, crerror_category(st_no_ssh));
              }
              pCam_no_ssh->Release();
            } else {
              std::printf("[SonyBackend] CreateCameraObjectInfoEthernetConnection (non-SSH) failed (0x%08X) category=%s\n", (unsigned)err_no_ssh, crerror_category(err_no_ssh));
            }
          }
        }
      }
    } else {
      std::printf("[SonyBackend] SONY_CAMERA_IP invalid format: %s\n", cam_ip_env);
    }
  }

  // 2) Enumerate with retry/backoff - match RemoteCli (no timeout)
  SCRSDK::ICrEnumCameraObjectInfo* enumInfo = nullptr;
  std::printf("[SonyBackend] EnumCameraObjects...\n");
  const int max_attempts = 5;
  SCRSDK::CrError st = 0; /* initialize to OK */
  for (int attempt = 1; attempt <= max_attempts; ++attempt) {
    // Match RemoteCli exactly: no timeout parameter
    st = SCRSDK::EnumCameraObjects(&enumInfo);
    if (!CR_FAILED(st) && enumInfo) break;
    std::printf("[SonyBackend] EnumCameraObjects failed (0x%08X) attempt %d/%d\n", (unsigned)st, attempt, max_attempts);
    if (attempt < max_attempts) {
      // show local interfaces to aid debugging
      system("ip -brief addr 2>/dev/null || ip addr 2>/dev/null || echo 'ip command not available'");
      std::this_thread::sleep_for(std::chrono::seconds(1 << (attempt-1)));
    }
  }
  if (CR_FAILED(st) || !enumInfo) {
    std::printf("[SonyBackend] EnumCameraObjects failed after %d attempts (0x%08X)\n", max_attempts, (unsigned)st);

    return false;
  }

  const CrInt32u n = enumInfo->GetCount();
  if (n == 0) {
    std::printf("[SonyBackend] No cameras found\n");
    enumInfo->Release();
    return false;
  }

  std::printf("[SonyBackend] Enumerated %u camera(s):\n", (unsigned)n);

  // Selection: prefer SONY_CAMERA_INDEX (1-based) or SONY_CAMERA_MAC (MAC string), else default to first
  int selectedIndex = -1;
  const char* env_index = std::getenv("SONY_CAMERA_INDEX");
  const char* env_mac = std::getenv("SONY_CAMERA_MAC");

  auto to_lower = [](const char* s) {
    std::string r;
    if (!s) return r;
    r = s;
    for (auto &c : r) c = (char)std::tolower((unsigned char)c);
    return r;
  };

  for (CrInt32u i = 0; i < n; ++i) {
    const SCRSDK::ICrCameraObjectInfo* info = enumInfo->GetCameraObjectInfo(i);
    if (!info) continue;
    const char* conn = info->GetConnectionTypeName();
    const char* model = info->GetModel();
    const CrInt8u* id_raw = info->GetId();
    std::string idstr = id_raw ? std::string(reinterpret_cast<const char*>(id_raw)) : std::string();
    const char* mac = nullptr;
    // GetMACAddressChar exists for IP-connected devices in sample
    mac = reinterpret_cast<const char*>(info->GetMACAddressChar());

    std::printf("[%u] model=%s conn=%s id=%s mac=%s adaptor=%s\n",
                (unsigned)(i + 1), model ? model : "", conn ? conn : "", idstr.c_str(), mac ? mac : "", info->GetAdaptorName());

    if (selectedIndex == -1 && env_index && env_index[0]) {
      char* endptr = nullptr;
      long v = std::strtol(env_index, &endptr, 10);
      if (endptr && *endptr == '\0' && v >= 1 && v <= (long)n) {
        selectedIndex = (int)(v - 1);
      }
    }

    if (selectedIndex == -1 && env_mac && env_mac[0] && mac) {
      if (to_lower(mac) == to_lower(env_mac)) {
        selectedIndex = (int)i;
      }
    }
  }

  if (selectedIndex == -1) {
    if (env_index || env_mac) {
      std::printf("[SonyBackend] SONY_CAMERA_INDEX/MAC specified but no match found. Defaulting to first camera.\n");
    }
    selectedIndex = 0; // default to first
  }

  const SCRSDK::ICrCameraObjectInfo* camInfoConst = enumInfo->GetCameraObjectInfo((CrInt32u)selectedIndex);
  if (!camInfoConst) {
    std::printf("[SonyBackend] Failed to get camera info index %d\n", selectedIndex);
    enumInfo->Release();
    return false;
  }

  m_camera_model = camInfoConst->GetModel() ? camInfoConst->GetModel() : "";
  m_connection_type = camInfoConst->GetConnectionTypeName() ? camInfoConst->GetConnectionTypeName() : "";
  std::printf("[SonyBackend] Selected camera model=%s conn=%s\n",
              m_camera_model.c_str(), m_connection_type.c_str());

  const bool is_usb = (m_connection_type == "USB");

  // Connect requires non-const
  SCRSDK::ICrCameraObjectInfo* camInfo =
      const_cast<SCRSDK::ICrCameraObjectInfo*>(camInfoConst);

  // 3) Fingerprint + credentials (Ethernet requires this; USB does not)
  char fingerprint[4096] = {0};
  CrInt32u fpSize = 0;
  const char* user = nullptr;
  const char* pass = nullptr;

  if (!is_usb) {
    fpSize = (CrInt32u)sizeof(fingerprint);
    SCRSDK::CrError fpSt = SCRSDK::GetFingerprint(camInfo, fingerprint, &fpSize);

    std::string fp_norm;
    if (CR_FAILED(fpSt) || fpSize == 0) {
      std::printf("[SonyBackend] GetFingerprint failed (0x%08X)\n", (unsigned)fpSt);
      fpSize = 0; // still attempt connect
    } else {
      fp_norm = normalize_fingerprint(fingerprint, fpSize);

      std::printf("[SonyBackend] Fingerprint OK (size=%u padded=%u)\n", (unsigned)fpSize, (unsigned)fp_norm.size());
      std::printf("[SonyBackend] fingerprint (padded):\n%s\n", fp_norm.c_str());

      const char* accept_fp = std::getenv("SONY_ACCEPT_FINGERPRINT");
      if (!(accept_fp && accept_fp[0] && accept_fp[0] == '1')) {
        std::printf("[SonyBackend] Fingerprint requires acceptance. Set SONY_ACCEPT_FINGERPRINT=1 to auto-accept and connect.\n");
        enumInfo->Release();
        return false;
      }
    }

    // Credentials from env (match RemoteCli behaviour)
    user = std::getenv("SONY_USER");
    pass = std::getenv("SONY_PASS");

    if (!pass || !pass[0]) {
      std::printf("[SonyBackend] Missing SONY_PASS env var\n");
      enumInfo->Release();
      return false;
    }
    if (!user || !user[0]) user = nullptr;
  } else {
    std::printf("[SonyBackend] USB connection: skipping fingerprint and password\n");
  }

  // 5) Connect (Remote Control Mode) with retries + backoff using direct CRSDK Connect
  std::printf("[SonyBackend] Connect (Remote Control Mode) via direct CRSDK Connect...\n");

  if (!m_callback_impl) m_callback_impl = static_cast<void*>(new DeviceCallbackImpl());
  auto* cb = static_cast<SCRSDK::IDeviceCallback*>(m_callback_impl);
  const char* accept_fp = std::getenv("SONY_ACCEPT_FINGERPRINT");
  const char* env_fp = std::getenv("SONY_FINGERPRINT");
  const CrInt32u env_fp_len = (env_fp && env_fp[0]) ? (CrInt32u)std::strlen(env_fp) : 0;
  CrInt32u fp_len = 0;
  const char* fp_ptr = nullptr;
  if (!is_usb && accept_fp && accept_fp[0] == '1') {
    fp_ptr = select_fingerprint(env_fp, env_fp_len, fingerprint, fpSize, &fp_len);
    if (env_fp && env_fp[0]) {
      std::printf("[SonyBackend] Using fingerprint from SONY_FINGERPRINT (len=%u)\n", (unsigned)fp_len);
    } else if (fp_ptr && fp_len > 0) {
      std::printf("[SonyBackend] Using fingerprint from GetFingerprint (len=%u)\n", (unsigned)fp_len);
    }
  }

  const int max_connect_attempts = 5;
  bool cd_connected = false;
  for (int attempt = 1; attempt <= max_connect_attempts; ++attempt) {
    SCRSDK::CrDeviceHandle h = 0;
    if (connect_camera(camInfo, cb, user, pass, fp_ptr, fp_len, &h)) {
      m_device_handle = h;
      m_connected = true;
      std::printf("[SonyBackend] Connect succeeded on attempt %d\n", attempt);
      cd_connected = true;
      break;
    }
    std::printf("[SonyBackend] Connect attempt %d/%d failed\n", attempt, max_connect_attempts);
    if (attempt < max_connect_attempts) std::this_thread::sleep_for(std::chrono::seconds(1 << (attempt - 1)));
  }

  enumInfo->Release();

  if (!cd_connected) {
    std::printf("[SonyBackend] Connect failed after %d attempts\n", max_connect_attempts);
    return false;
  }

  std::printf("[SonyBackend] Connected!\n");
  return true;
}

bool SonyBackend::set_runstop(bool run) {
  if (!is_connected()) {
    std::printf("[SonyBackend] set_runstop(%d): not connected\n", run ? 1 : 0);
    return false;
  }

  if (m_camera_model == kA74FrozenModel) {
    const SCRSDK::CrCommandParam movie_param = run
        ? SCRSDK::CrCommandParam::CrCommandParam_Down
        : SCRSDK::CrCommandParam::CrCommandParam_Up;

    uint32_t rec_state = 0xFFFFFFFFu;
    uint32_t rec_main = 0xFFFFFFFFu;
    bool is_recording = false;
    if (read_recording_flags(m_device_handle, rec_state, rec_main, is_recording) && is_recording == run) {
      std::printf("[SonyBackend] set_runstop(%d): already target state (rec_state=0x%08X rec_main=0x%08X)\n",
                  run ? 1 : 0, (unsigned)rec_state, (unsigned)rec_main);
      return true;
    }

    try_prepare_recording_mode(m_device_handle);
    // A74 expects direct button semantics: Down=start, Up=stop.
    std::this_thread::sleep_for(std::chrono::milliseconds(120));
    auto st = SCRSDK::SendCommand(
        m_device_handle,
        SCRSDK::CrCommandId::CrCommandId_MovieRecord,
        movie_param);
    std::printf("[SonyBackend] set_runstop(%d): A74 MovieRecord param=%s st=0x%08X\n",
                run ? 1 : 0,
                run ? "Down(start)" : "Up(stop)",
                (unsigned)st);

    if (CR_FAILED(st)) {
      // Try to enable toggle command support, then fallback command paths.
      SCRSDK::CrDeviceProperty enable_prop;
      enable_prop.SetCode(SCRSDK::CrDevicePropertyCode::CrDeviceProperty_MovieRecButtonToggleEnableStatus);
      enable_prop.SetValueType(SCRSDK::CrDataType_UInt8);
      enable_prop.SetCurrentValue(SCRSDK::CrMovieRecButtonToggle_Enable);
      auto st_enable = SCRSDK::SetDeviceProperty(m_device_handle, &enable_prop);
      std::printf("[SonyBackend] set_runstop(%d): enable toggle st=0x%08X\n",
                  run ? 1 : 0, (unsigned)st_enable);

      auto send_toggle = [&](SCRSDK::CrCommandId cmd_id, const char* label) {
        auto st_down = SCRSDK::SendCommand(
            m_device_handle, cmd_id, SCRSDK::CrCommandParam::CrCommandParam_Down);
        std::this_thread::sleep_for(std::chrono::milliseconds(120));
        auto st_up = SCRSDK::SendCommand(
            m_device_handle, cmd_id, SCRSDK::CrCommandParam::CrCommandParam_Up);
        std::printf("[SonyBackend] set_runstop(%d): %s down=0x%08X up=0x%08X\n",
                    run ? 1 : 0, label, (unsigned)st_down, (unsigned)st_up);
        return (!CR_FAILED(st_down) || !CR_FAILED(st_up));
      };

      bool any_ok = false;
      any_ok = send_toggle(SCRSDK::CrCommandId::CrCommandId_MovieRecButtonToggle, "MovieRecButtonToggle") || any_ok;
      any_ok = send_toggle(SCRSDK::CrCommandId::CrCommandId_MovieRecButtonToggle2, "MovieRecButtonToggle2") || any_ok;
      any_ok = send_toggle(SCRSDK::CrCommandId::CrCommandId_StreamButton, "StreamButton") || any_ok;

      if (!any_ok) {
        const char* allow_invalid = std::getenv("SONY_ALLOW_INVALID_CALLED");
        if (allow_invalid && allow_invalid[0] == '1' && st == kA74InvalidCalled) {
          std::printf("[SonyBackend] set_runstop(%d): treating MovieRecord 0x8402 as success (SONY_ALLOW_INVALID_CALLED=1)\n",
                      run ? 1 : 0);
          any_ok = true;
        }
      }
      if (!any_ok) return false;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(250));
    if (read_recording_flags(m_device_handle, rec_state, rec_main, is_recording)) {
      std::printf("[SonyBackend] set_runstop(%d): post-cmd rec_state=0x%08X rec_main=0x%08X is_recording=%d\n",
                  run ? 1 : 0, (unsigned)rec_state, (unsigned)rec_main, is_recording ? 1 : 0);
    } else {
      std::printf("[SonyBackend] set_runstop(%d): post-cmd recording flags unavailable\n", run ? 1 : 0);
    }
    return true;
  }

  const SCRSDK::CrCommandParam start_param = SCRSDK::CrCommandParam::CrCommandParam_Down;
  const SCRSDK::CrCommandParam stop_param = SCRSDK::CrCommandParam::CrCommandParam_Up;
  const SCRSDK::CrCommandParam movie_param = run ? start_param : stop_param;

  auto send_toggle = [&](SCRSDK::CrCommandId cmd_id) {
    auto st_down = SCRSDK::SendCommand(m_device_handle, cmd_id, SCRSDK::CrCommandParam::CrCommandParam_Down);
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    auto st_up = SCRSDK::SendCommand(m_device_handle, cmd_id, SCRSDK::CrCommandParam::CrCommandParam_Up);
    std::printf("[SonyBackend] set_runstop(%d): %s down=0x%08X up=0x%08X\n",
                run ? 1 : 0,
                (cmd_id == SCRSDK::CrCommandId::CrCommandId_MovieRecButtonToggle) ? "Toggle" : "Toggle2",
                (unsigned)st_down,
                (unsigned)st_up);
    return std::pair<SCRSDK::CrError, SCRSDK::CrError>(st_down, st_up);
  };

  auto toggle_result = send_toggle(SCRSDK::CrCommandId::CrCommandId_MovieRecButtonToggle);
  if (CR_SUCCEEDED(toggle_result.first) || CR_SUCCEEDED(toggle_result.second)) {
    std::printf("[SonyBackend] set_runstop(%d): OK (toggle)\n", run ? 1 : 0);
    return true;
  }

  auto st = SCRSDK::SendCommand(
      m_device_handle,
      SCRSDK::CrCommandId::CrCommandId_MovieRecord,
      movie_param);
  if (CR_SUCCEEDED(st)) {
    std::printf("[SonyBackend] set_runstop(%d): OK (movie)\n", run ? 1 : 0);
    return true;
  }

  const char* allow_invalid = std::getenv("SONY_ALLOW_INVALID_CALLED");
  if (allow_invalid && allow_invalid[0] == '1' &&
      toggle_result.first == 0x8402 && toggle_result.second == 0x8402) {
    std::printf("[SonyBackend] set_runstop(%d): treating 0x8402 as success (SONY_ALLOW_INVALID_CALLED=1)\n",
                run ? 1 : 0);
    return true;
  }

  std::printf("[SonyBackend] set_runstop(%d): SendCommand FAILED (toggle=0x%08X/0x%08X movie=0x%08X)\n",
              run ? 1 : 0,
              (unsigned)toggle_result.first,
              (unsigned)toggle_result.second,
              (unsigned)st);
  return false;
}

bool SonyBackend::get_property_options(CrInt32u property_code, PropertyOptions& out) {
  if (!is_connected()) {
    std::printf("[SonyBackend] get_property_options: not connected\n");
    return false;
  }

  SCRSDK::CrDeviceProperty* props = nullptr;
  CrInt32 num_props = 0;
  CrInt32u code = property_code;
  auto err = SCRSDK::GetSelectDeviceProperties(m_device_handle, 1, &code, &props, &num_props);
  if (CR_FAILED(err) || !props || num_props <= 0) {
    std::printf("[SonyBackend] get_property_options: GetSelectDeviceProperties failed 0x%08X\n", (unsigned)err);
    if (props) SCRSDK::ReleaseDeviceProperties(m_device_handle, props);
    return false;
  }

  const auto& prop = props[0];
  out.value_type = prop.GetValueType();
  out.current_value = (uint32_t)prop.GetCurrentValue();
  out.values.clear();

  const CrInt8u* raw = prop.GetSetValues();
  const CrInt32u raw_size = prop.GetSetValueSize();
  const size_t es = element_size(out.value_type);
  if (raw && raw_size > 0 && es > 0) {
    const size_t count = raw_size / es;
    out.values.reserve(count);
    for (size_t i = 0; i < count; ++i) {
      uint32_t v = 0;
      const CrInt8u* p = raw + (i * es);
      switch (es) {
        case 1: v = *reinterpret_cast<const CrInt8u*>(p); break;
        case 2: v = *reinterpret_cast<const CrInt16u*>(p); break;
        case 4: v = *reinterpret_cast<const CrInt32u*>(p); break;
        case 8: v = (uint32_t)(*reinterpret_cast<const CrInt64u*>(p)); break;
      }
      out.values.push_back(v);
    }
  }

  SCRSDK::ReleaseDeviceProperties(m_device_handle, props);
  return true;
}

bool SonyBackend::set_property_value(CrInt32u property_code, uint32_t value) {
  if (!is_connected()) {
    std::printf("[SonyBackend] set_property_value: not connected\n");
    return false;
  }

  SCRSDK::CrDeviceProperty* props = nullptr;
  CrInt32 num_props = 0;
  CrInt32u code = property_code;
  auto err = SCRSDK::GetSelectDeviceProperties(m_device_handle, 1, &code, &props, &num_props);
  if (CR_FAILED(err) || !props || num_props <= 0) {
    std::printf("[SonyBackend] set_property_value: GetSelectDeviceProperties failed 0x%08X\n", (unsigned)err);
    if (props) SCRSDK::ReleaseDeviceProperties(m_device_handle, props);
    return false;
  }

  bool ok = false;
  SCRSDK::CrDeviceProperty& prop = props[0];
  if (!prop.IsSetEnableCurrentValue()) {
    std::printf("[SonyBackend] set_property_value: property 0x%08X not settable\n", (unsigned)property_code);
  } else {
    prop.SetCurrentValue((CrInt64u)value);
    auto st = SCRSDK::SetDeviceProperty(m_device_handle, &prop);
    if (!CR_FAILED(st)) {
      ok = true;
    } else {
      std::printf("[SonyBackend] set_property_value: SetDeviceProperty failed 0x%08X\n", (unsigned)st);
    }
  }

  SCRSDK::ReleaseDeviceProperties(m_device_handle, props);
  return ok;
}

bool SonyBackend::step_property_value(CrInt32u property_code, int8_t step) {
  if (step == 0) return true;
  PropertyOptions opts;
  if (!get_property_options(property_code, opts)) return false;
  if (opts.values.empty()) return false;

  size_t idx = 0;
  bool found = false;
  for (size_t i = 0; i < opts.values.size(); ++i) {
    if (opts.values[i] == opts.current_value) {
      idx = i;
      found = true;
      break;
    }
  }
  if (!found) {
    idx = (step > 0) ? 0u : (opts.values.size() - 1u);
  }

  int next = (int)idx + (int)step;
  if (next < 0) next = 0;
  if (next >= (int)opts.values.size()) next = (int)opts.values.size() - 1;

  const uint32_t next_value = opts.values[(size_t)next];
  return set_property_value(property_code, next_value);
}

bool SonyBackend::get_status(Status& out) {
  if (!is_connected()) {
    std::printf("[SonyBackend] get_status: not connected\n");
    return false;
  }

  SCRSDK::CrDeviceProperty* props = nullptr;
  CrInt32 num_props = 0;
  auto err = SCRSDK::GetDeviceProperties(m_device_handle, &props, &num_props);
  if (CR_FAILED(err) || !props || num_props <= 0) {
    std::printf("[SonyBackend] get_status: GetDeviceProperties failed 0x%08X num_props=%d\n",
                (unsigned)err, (int)num_props);
    if (props) SCRSDK::ReleaseDeviceProperties(m_device_handle, props);
    return false;
  }

  auto get_code = [&](CrInt32u code, uint32_t& dest) {
    for (CrInt32 i = 0; i < num_props; ++i) {
      if (props[i].GetCode() == code) {
        uint32_t v = 0;
        if (get_prop_first_value(props[i], v)) {
          dest = v;
        }
        return;
      }
    }
  };

  get_code(SCRSDK::CrDeviceProperty_BatteryLevel, out.battery_level);
  get_code(SCRSDK::CrDeviceProperty_BatteryRemain, out.battery_remain);
  get_code(SCRSDK::CrDeviceProperty_BatteryRemainDisplayUnit, out.battery_remain_unit);
  get_code(SCRSDK::CrDeviceProperty_RecordingMedia, out.recording_media);
  get_code(SCRSDK::CrDeviceProperty_Movie_RecordingMedia, out.movie_recording_media);
  get_code(SCRSDK::CrDeviceProperty_MediaSLOT1_Status, out.media_slot1_status);
  get_code(SCRSDK::CrDeviceProperty_MediaSLOT1_RemainingNumber, out.media_slot1_remaining_number);
  get_code(SCRSDK::CrDeviceProperty_MediaSLOT1_RemainingTime, out.media_slot1_remaining_time);
  get_code(SCRSDK::CrDeviceProperty_MediaSLOT2_Status, out.media_slot2_status);
  get_code(SCRSDK::CrDeviceProperty_MediaSLOT2_RemainingNumber, out.media_slot2_remaining_number);
  get_code(SCRSDK::CrDeviceProperty_MediaSLOT2_RemainingTime, out.media_slot2_remaining_time);
  get_code(SCRSDK::CrDeviceProperty_RecordingState, out.recording_state);
  uint32_t recorder_main_status = 0xFFFFFFFFu;
  get_code(SCRSDK::CrDeviceProperty_RecorderMainStatus, recorder_main_status);
  if ((out.recording_state == 0xFFFFFFFFu || out.recording_state == 0u) &&
      recorder_main_status != 0xFFFFFFFFu) {
    out.recording_state = recorder_main_status;
  }

  SCRSDK::ReleaseDeviceProperties(m_device_handle, props);
  return true;
}

bool SonyBackend::capture_still(bool with_af) {
  if (!is_connected()) {
    std::printf("[SonyBackend] capture_still: not connected\n");
    return false;
  }

  const SCRSDK::CrCommandId cmd_id = with_af
    ? SCRSDK::CrCommandId::CrCommandId_S1andRelease
    : SCRSDK::CrCommandId::CrCommandId_Release;

  auto st_down = SCRSDK::SendCommand(m_device_handle, cmd_id, SCRSDK::CrCommandParam::CrCommandParam_Down);
  std::this_thread::sleep_for(std::chrono::milliseconds(120));
  auto st_up = SCRSDK::SendCommand(m_device_handle, cmd_id, SCRSDK::CrCommandParam::CrCommandParam_Up);

  std::printf("[SonyBackend] capture_still(%d): cmd=%d down=0x%08X up=0x%08X\n",
              with_af ? 1 : 0, (int)cmd_id, (unsigned)st_down, (unsigned)st_up);

  return CR_SUCCEEDED(st_down) || CR_SUCCEEDED(st_up);
}

} // namespace ccu
