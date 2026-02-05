#pragma once
#include <cstdint>
#include <memory>

// CRSDK
#include "CRSDK/CameraRemote_SDK.h"
#include "sony_sample/CameraDevice.h"
#include <memory>

namespace ccu {

class SonyBackend {
public:
  SonyBackend() = default;
  ~SonyBackend();

  // Try to connect to first enumerated camera (Ethernet/USB)
  bool connect_first_camera();

  // run=true -> record start, run=false -> record stop
  bool set_runstop(bool run);

  bool is_connected() const { return m_connected && (m_device_handle != 0); }

private:
  bool     m_inited = false;
  bool     m_connected = false;
  SCRSDK::CrDeviceHandle m_device_handle = 0;

  // Opaque pointer to concrete callback implementation (managed in .cpp)
  void* m_callback_impl = nullptr;

  // When reusing Sony sample's CameraDevice, hold an instance here
  std::unique_ptr<cli::CameraDevice> m_camera_device;
};

} // namespace ccu
