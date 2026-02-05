#include "sony_backend.hpp"
#include "CRSDK/IDeviceCallback.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <thread>
#include <chrono>

// Minimal device callback implementation (no-op) copied signatures from sample
// to ensure we pass a valid callback object to SCRSDK::Connect.
namespace {
struct DeviceCallbackImpl : public SCRSDK::IDeviceCallback {
  // Inherited via IDeviceCallback (no-op implementations)
  virtual void OnConnected(SCRSDK::DeviceConnectionVersioin version) override {}
  virtual void OnDisconnected(CrInt32u error) override {}
  virtual void OnPropertyChanged() override {}
  virtual void OnLvPropertyChanged() override {}
  virtual void OnCompleteDownload(CrChar* filename, CrInt32u type) override {}
  virtual void OnWarning(CrInt32u warning) override {}
  virtual void OnWarningExt(CrInt32u warning, CrInt32 param1, CrInt32 param2, CrInt32 param3) override {}
  virtual void OnError(CrInt32u error) override {}
  virtual void OnPropertyChangedCodes(CrInt32u num, CrInt32u* codes) override {}
  virtual void OnLvPropertyChangedCodes(CrInt32u num, CrInt32u* codes) override {}
  virtual void OnNotifyContentsTransfer(CrInt32u notify, SCRSDK::CrContentHandle contentHandle, CrChar* filename) override {}
  virtual void OnNotifyFTPTransferResult(CrInt32u notify, CrInt32u numOfSuccess, CrInt32u numOfFail) override {}
  virtual void OnNotifyRemoteTransferResult(CrInt32u notify, CrInt32u per, CrChar* filename) override {}
  virtual void OnNotifyRemoteTransferResult(CrInt32u notify, CrInt32u per, CrInt8u* data, CrInt64u size) override {}
  virtual void OnNotifyRemoteTransferContentsListChanged(CrInt32u notify, CrInt32u slotNumber, CrInt32u addSize) override {}
  virtual void OnReceivePlaybackTimeCode(CrInt32u timeCode) override {}
  virtual void OnReceivePlaybackData(CrInt8u mediaType, CrInt32 dataSize, CrInt8u* data, CrInt64 pts, CrInt64 dts, CrInt32 param1, CrInt32 param2) override {}
  virtual void OnNotifyRemoteFirmwareUpdateResult(CrInt32u notify, const void* param) override {}
};
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
  if (is_connected()) return true;

  // 1) Init once
  if (!m_inited) {
    const bool ok = SCRSDK::Init(0);
    std::printf("[SonyBackend] Init() returned: %d\n", ok ? 1 : 0);
    if (!ok) return false;
    m_inited = true;
  }

  // 2) Enumerate with retry/backoff
  SCRSDK::ICrEnumCameraObjectInfo* enumInfo = nullptr;
  std::printf("[SonyBackend] EnumCameraObjects...\n");
  const int max_attempts = 5;
  SCRSDK::CrError st = 0; /* initialize to OK */
  for (int attempt = 1; attempt <= max_attempts; ++attempt) {
    st = SCRSDK::EnumCameraObjects(&enumInfo, 3);
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

  const auto n = enumInfo->GetCount();
  if (n == 0) {
    std::printf("[SonyBackend] No cameras found\n");
    enumInfo->Release();
    return false;
  }

  const SCRSDK::ICrCameraObjectInfo* camInfoConst = enumInfo->GetCameraObjectInfo(0);
  if (!camInfoConst) {
    std::printf("[SonyBackend] Failed to get camera info index 0\n");
    enumInfo->Release();
    return false;
  }

  // Connect requires non-const
  SCRSDK::ICrCameraObjectInfo* camInfo =
      const_cast<SCRSDK::ICrCameraObjectInfo*>(camInfoConst);

  // 3) Fingerprint (Ethernet requires this)
  char fingerprint[4096] = {0};
  CrInt32u fpSize = (CrInt32u)sizeof(fingerprint); // NOTE: NOT SCRSDK::CrInt32u
  SCRSDK::CrError fpSt = SCRSDK::GetFingerprint(camInfo, fingerprint, &fpSize);

  if (CR_FAILED(fpSt) || fpSize == 0) {
    std::printf("[SonyBackend] GetFingerprint failed (0x%08X)\n", (unsigned)fpSt);
    fpSize = 0; // still attempt connect
  } else {
    // Ensure null termination for printing
    if (fpSize < sizeof(fingerprint)) fingerprint[fpSize] = '\0';
    fingerprint[sizeof(fingerprint) - 1] = '\0';

    std::printf("[SonyBackend] Fingerprint OK (size=%u)\n", (unsigned)fpSize);
    std::printf("[SonyBackend] fingerprint:\n%s\n", fingerprint);
  }

  // 4) Credentials from env (match your RemoteCli behaviour)
  const char* user = std::getenv("SONY_USER"); // can be null
  const char* pass = std::getenv("SONY_PASS"); // should be your SSH password prompt in RemoteCli

  if (!pass || !pass[0]) {
    std::printf("[SonyBackend] Missing SONY_PASS env var\n");
    enumInfo->Release();
    return false;
  }
  // RemoteCli normally uses "admin" as the inputId in samples; prefer that if unset
  if (!user || !user[0]) user = "admin";

  // 5) Connect (Remote Control Mode)
  std::printf("[SonyBackend] Connect (Remote Control Mode)...\n");
  SCRSDK::CrDeviceHandle handle = 0;

  // Ensure we have a valid callback instance (managed as opaque pointer)
  if (!m_callback_impl) {
    m_callback_impl = static_cast<void*>(new DeviceCallbackImpl());
  }

  SCRSDK::CrError st2 = SCRSDK::Connect(
      camInfo,
      static_cast<SCRSDK::IDeviceCallback*>(m_callback_impl),
      &handle,
      SCRSDK::CrSdkControlMode_Remote,
      SCRSDK::CrReconnecting_ON,
      user,
      pass,
      (fpSize > 0) ? fingerprint : nullptr,
      fpSize);

  enumInfo->Release();

  if (CR_FAILED(st2) || handle == 0) {
    std::printf("[SonyBackend] Connect failed (0x%08X)\n", (unsigned)st2);
    return false;
  }

  m_device_handle = handle;
  m_connected = true;
  std::printf("[SonyBackend] Connected!\n");
  return true;
}

bool SonyBackend::set_runstop(bool run) {
  if (!is_connected()) {
    std::printf("[SonyBackend] set_runstop(%d): not connected\n", run ? 1 : 0);
    return false;
  }

  // Same mapping the sample uses: Up/Down
  const SCRSDK::CrCommandParam param = run
      ? SCRSDK::CrCommandParam::CrCommandParam_Up
      : SCRSDK::CrCommandParam::CrCommandParam_Down;

  // Try toggle, then fall back to MovieRecord
  auto st = SCRSDK::SendCommand(
      m_device_handle,
      SCRSDK::CrCommandId::CrCommandId_MovieRecButtonToggle,
      (SCRSDK::CrCommandParam)param);

  if (CR_FAILED(st)) {
    st = SCRSDK::SendCommand(
        m_device_handle,
        SCRSDK::CrCommandId::CrCommandId_MovieRecord,
        (SCRSDK::CrCommandParam)param);
  }

  if (CR_FAILED(st)) {
    std::printf("[SonyBackend] set_runstop(%d): SendCommand FAILED (0x%08X)\n",
                run ? 1 : 0, (unsigned)st);
    return false;
  }

  std::printf("[SonyBackend] set_runstop(%d): OK\n", run ? 1 : 0);
  return true;
}

} // namespace ccu
