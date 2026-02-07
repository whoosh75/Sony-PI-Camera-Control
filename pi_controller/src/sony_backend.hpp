#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

// CRSDK
#include "CRSDK/CameraRemote_SDK.h"

namespace ccu {

class SonyBackend {
public:
  SonyBackend() = default;
  ~SonyBackend();

  // Try to connect to first enumerated camera (Ethernet/USB)
  bool connect_first_camera();

  // run=true -> record start, run=false -> record stop
  bool set_runstop(bool run);

  struct PropertyOptions {
    SCRSDK::CrDataType value_type = SCRSDK::CrDataType_Undefined;
    uint32_t current_value = 0;
    std::vector<uint32_t> values;
  };

  bool get_property_options(CrInt32u property_code, PropertyOptions& out);

  struct Status {
    uint32_t battery_level = 0xFFFFFFFFu;
    uint32_t battery_remain = 0xFFFFFFFFu;
    uint32_t battery_remain_unit = 0xFFFFFFFFu;
    uint32_t recording_media = 0xFFFFFFFFu;
    uint32_t movie_recording_media = 0xFFFFFFFFu;
    uint32_t media_slot1_status = 0xFFFFFFFFu;
    uint32_t media_slot1_remaining_number = 0xFFFFFFFFu;
    uint32_t media_slot1_remaining_time = 0xFFFFFFFFu;
    uint32_t media_slot2_status = 0xFFFFFFFFu;
    uint32_t media_slot2_remaining_number = 0xFFFFFFFFu;
    uint32_t media_slot2_remaining_time = 0xFFFFFFFFu;
  };

  bool get_status(Status& out);

  // Stills capture
  bool capture_still(bool with_af);

  const std::string& camera_model() const { return m_camera_model; }
  const std::string& connection_type() const { return m_connection_type; }

  bool is_connected() const { return m_connected && (m_device_handle != 0); }

private:
  bool     m_inited = false;
  bool     m_connected = false;
  SCRSDK::CrDeviceHandle m_device_handle = 0;

  std::string m_camera_model;
  std::string m_connection_type;

  // Opaque pointer to concrete callback implementation (managed in .cpp)
  void* m_callback_impl = nullptr;

};

} // namespace ccu
