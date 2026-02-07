#include <cstdio>
#include <cstdint>
#include <unistd.h>
#include <algorithm>
#include <thread>
#include <chrono>
#include "protocol.hpp"
#include "udp_server.hpp"
#include <cstdlib>
#include <ctime>
#include "sony_backend.hpp"

// CRSDK header included so we know headers + linkage still ok
#include "CRSDK/CameraRemote_SDK.h"

using namespace ccu;

static bool g_run_state = false; // reported state (until we subscribe to CRSDK state)
static ccu::SonyBackend g_sony;

static uint32_t read_env_u32(const char* name) {
  const char* v = std::getenv(name);
  if (!v || !v[0]) return 0;
  return (uint32_t)std::strtoul(v, nullptr, 0);
}

static uint32_t clamp_percent(uint32_t v) {
  if (v > 100u) return 100u;
  return v;
}

static uint32_t battery_percent_from_status(const ccu::SonyBackend::Status& st) {
  if (st.battery_remain != 0xFFFFFFFFu && st.battery_remain <= 100u) {
    return st.battery_remain;
  }

  switch (st.battery_level) {
    case SCRSDK::CrBatteryLevel_PreEndBattery: return 5;
    case SCRSDK::CrBatteryLevel_1_4: return 25;
    case SCRSDK::CrBatteryLevel_2_4: return 50;
    case SCRSDK::CrBatteryLevel_3_4: return 75;
    case SCRSDK::CrBatteryLevel_4_4: return 100;
    case SCRSDK::CrBatteryLevel_1_3: return 33;
    case SCRSDK::CrBatteryLevel_2_3: return 66;
    case SCRSDK::CrBatteryLevel_3_3: return 100;
    case SCRSDK::CrBatteryLevel_USBPowerSupply: return 100;
    case SCRSDK::CrBatteryLevel_PreEnd_PowerSupply: return 5;
    case SCRSDK::CrBatteryLevel_1_4_PowerSupply: return 25;
    case SCRSDK::CrBatteryLevel_2_4_PowerSupply: return 50;
    case SCRSDK::CrBatteryLevel_3_4_PowerSupply: return 75;
    case SCRSDK::CrBatteryLevel_4_4_PowerSupply: return 100;
    default:
      return 0xFFFFFFFFu;
  }
}

static uint32_t media_time_value(uint32_t remaining_time) {
  // A74 reports remaining time in seconds; convert to whole minutes.
  if (remaining_time == 0xFFFFFFFFu) return remaining_time;
  if (remaining_time >= 60u) return remaining_time / 60u;
  return remaining_time;
}

int main(int argc, char** argv) {
  const uint16_t port = (argc >= 2) ? (uint16_t)std::atoi(argv[1]) : 5555;

  UdpServer udp;
  if (!udp.open(port)) {
    std::fprintf(stderr, "Failed to open UDP port %u\n", port);
    return 1;
  }
  std::printf("ccu_daemon listening UDP :%u\n", port);

  // Background connect loop (non-blocking for UDP)
  std::thread connect_thread([]() {
    while (true) {
      if (!g_sony.is_connected()) {
        g_sony.connect_first_camera();
      }
      std::this_thread::sleep_for(std::chrono::seconds(2));
    }
  });
  connect_thread.detach();

  uint8_t rxbuf[512];
  uint8_t txbuf[512];

  while (true) {
    sockaddr_in from{};
    int n = udp.recv(rxbuf, sizeof(rxbuf), from);
    if (n <= 0) { usleep(1000); continue; }

    Header h{};
    const uint8_t* pl = nullptr;
    size_t pl_len = 0;
    uint8_t err = RESP_OK;

    if (!parse_packet(rxbuf, (size_t)n, h, pl, pl_len, err)) {
      uint8_t ap[8] = {0};
      const size_t outn = build_resp_ack(txbuf, sizeof(txbuf), 0, 0, err, ap, sizeof(ap));
      udp.sendto(txbuf, outn, from);
      continue;
    }

    if (h.msg_type != MSG_REQ_CMD) {
      uint8_t ap[8] = {0};
      const size_t outn = build_resp_ack(txbuf, sizeof(txbuf), h.seq, h.target_mask, RESP_BAD_FORMAT, ap, sizeof(ap));
      udp.sendto(txbuf, outn, from);
      continue;
    }

    if (h.cmd_or_code == CMD_RUNSTOP) {
      if (pl_len < 1) {
        uint8_t ap[8] = {0};
        const size_t outn = build_resp_ack(txbuf, sizeof(txbuf), h.seq, h.target_mask, RESP_BAD_FORMAT, ap, sizeof(ap));
        udp.sendto(txbuf, outn, from);
        continue;
      }

      const bool run = (pl[0] != 0);

std::printf("RUNSTOP requested: %d (seq=%u target=0x%02X)\n",
            run ? 1 : 0, h.seq, h.target_mask);

// Apply via CRSDK backend (currently stub until we paste CRSDK code)
const bool ok = g_sony.set_runstop(run);
if (ok) g_run_state = run;


      // ACK payload (8 bytes)
      // ok_mask: pretend CamA (bit0) ok if target includes it (or ALL)
      const bool selA = (h.target_mask == 0xFF) || (h.target_mask & 0x01);
const uint8_t ok_mask   = (selA && ok) ? 0x01 : 0x00;
const uint8_t fail_mask = (selA && !ok) ? 0x01 : 0x00;
      const uint8_t busy_mask = 0x00;
      const uint8_t state_run_mask = g_run_state ? 0x01 : 0x00;
      const uint8_t state_known_mask = 0x01;

      uint8_t ap[8] = { ok_mask, fail_mask, busy_mask, state_run_mask, state_known_mask, 0,0,0 };
      const size_t outn = build_resp_ack(txbuf, sizeof(txbuf), h.seq, h.target_mask, RESP_OK, ap, sizeof(ap));
      udp.sendto(txbuf, outn, from);
      continue;
    }

    if (h.cmd_or_code == CMD_GET_OPTIONS) {
      if (pl_len < 1) {
        uint8_t ap[8] = {0};
        const size_t outn = build_resp_ack(txbuf, sizeof(txbuf), h.seq, h.target_mask, RESP_BAD_FORMAT, ap, sizeof(ap));
        udp.sendto(txbuf, outn, from);
        continue;
      }

      const uint8_t opt_id = pl[0];
      CrInt32u prop_code = 0;
      const char* label = "";
      switch (opt_id) {
        case OPT_ISO:
          prop_code = SCRSDK::CrDeviceProperty_IsoSensitivity;
          label = "ISO";
          break;
        case OPT_WHITE_BALANCE:
          prop_code = SCRSDK::CrDeviceProperty_WhiteBalance;
          label = "WhiteBalance";
          break;
        case OPT_SHUTTER:
          prop_code = SCRSDK::CrDeviceProperty_ShutterSpeed;
          label = "ShutterSpeed";
          break;
        case OPT_FPS:
          prop_code = SCRSDK::CrDeviceProperty_Movie_Recording_FrameRateSetting;
          label = "FrameRate";
          break;
        default:
          {
            uint8_t ap[8] = {0};
            const size_t outn = build_resp_ack(txbuf, sizeof(txbuf), h.seq, h.target_mask, RESP_BAD_FORMAT, ap, sizeof(ap));
            udp.sendto(txbuf, outn, from);
            continue;
          }
      }

      ccu::SonyBackend::PropertyOptions opts;
      const bool ok = g_sony.get_property_options(prop_code, opts);
      if (!ok) {
        uint8_t ap[8] = {0};
        const size_t outn = build_resp_ack(txbuf, sizeof(txbuf), h.seq, h.target_mask, RESP_UNKNOWN, ap, sizeof(ap));
        udp.sendto(txbuf, outn, from);
        continue;
      }

      auto wr16 = [&](uint8_t* p, uint16_t v) {
        p[0] = (uint8_t)(v & 0xFF);
        p[1] = (uint8_t)((v >> 8) & 0xFF);
      };
      auto wr32 = [&](uint8_t* p, uint32_t v) {
        p[0] = (uint8_t)(v & 0xFF);
        p[1] = (uint8_t)((v >> 8) & 0xFF);
        p[2] = (uint8_t)((v >> 16) & 0xFF);
        p[3] = (uint8_t)((v >> 24) & 0xFF);
      };

      const uint16_t count = (uint16_t)opts.values.size();
      const size_t payload_len = 1 + 2 + 2 + 4 + (size_t)count * 4;
      if (payload_len > sizeof(txbuf) - sizeof(Header) - 4) {
        uint8_t ap[8] = {0};
        const size_t outn = build_resp_ack(txbuf, sizeof(txbuf), h.seq, h.target_mask, RESP_BAD_FORMAT, ap, sizeof(ap));
        udp.sendto(txbuf, outn, from);
        continue;
      }

      uint8_t payload[512] = {0};
      size_t off = 0;
      payload[off++] = opt_id;
      wr16(payload + off, (uint16_t)opts.value_type); off += 2;
      wr16(payload + off, count); off += 2;
      wr32(payload + off, opts.current_value); off += 4;
      for (uint16_t i = 0; i < count; ++i) {
        wr32(payload + off, opts.values[i]);
        off += 4;
      }

      std::printf("OPTIONS %s count=%u current=0x%08X\n", label, (unsigned)count, (unsigned)opts.current_value);
      const size_t outn = build_resp_ack(txbuf, sizeof(txbuf), h.seq, h.target_mask, RESP_OK, payload, off);
      udp.sendto(txbuf, outn, from);
      continue;
    }

    if (h.cmd_or_code == CMD_GET_STATUS) {
      ccu::SonyBackend::Status st{};
      const bool ok = g_sony.get_status(st);
      if (!ok) {
        uint8_t ap[8] = {0};
        const size_t outn = build_resp_ack(txbuf, sizeof(txbuf), h.seq, h.target_mask, RESP_UNKNOWN, ap, sizeof(ap));
        udp.sendto(txbuf, outn, from);
        continue;
      }

      const uint32_t battery_pct = battery_percent_from_status(st);
        const uint32_t media1_time = media_time_value(st.media_slot1_remaining_time);
        const uint32_t media2_time = media_time_value(st.media_slot2_remaining_time);

      // Normalize to percent in response
      st.battery_level = battery_pct;
      st.battery_remain = battery_pct;
      st.battery_remain_unit = 1; // percent
      st.media_slot1_remaining_time = media1_time;
      st.media_slot2_remaining_time = media2_time;

      auto wr32 = [&](uint8_t* p, uint32_t v) {
        p[0] = (uint8_t)(v & 0xFF);
        p[1] = (uint8_t)((v >> 8) & 0xFF);
        p[2] = (uint8_t)((v >> 16) & 0xFF);
        p[3] = (uint8_t)((v >> 24) & 0xFF);
      };

      uint8_t payload[64] = {0};
      size_t off = 0;
      wr32(payload + off, st.battery_level); off += 4;
      wr32(payload + off, st.battery_remain); off += 4;
      wr32(payload + off, st.battery_remain_unit); off += 4;
      wr32(payload + off, st.recording_media); off += 4;
      wr32(payload + off, st.movie_recording_media); off += 4;
      wr32(payload + off, st.media_slot1_status); off += 4;
      wr32(payload + off, st.media_slot1_remaining_number); off += 4;
      wr32(payload + off, st.media_slot1_remaining_time); off += 4;
      wr32(payload + off, st.media_slot2_status); off += 4;
      wr32(payload + off, st.media_slot2_remaining_number); off += 4;
      wr32(payload + off, st.media_slot2_remaining_time); off += 4;

      const size_t outn = build_resp_ack(txbuf, sizeof(txbuf), h.seq, h.target_mask, RESP_OK, payload, off);
      udp.sendto(txbuf, outn, from);
      continue;
    }

    if (h.cmd_or_code == CMD_CAPTURE_STILL) {
      if (pl_len < 1) {
        uint8_t ap[8] = {0};
        const size_t outn = build_resp_ack(txbuf, sizeof(txbuf), h.seq, h.target_mask, RESP_BAD_FORMAT, ap, sizeof(ap));
        udp.sendto(txbuf, outn, from);
        continue;
      }

      const bool with_af = (pl[0] != 0);
      const bool ok = g_sony.capture_still(with_af);

      const bool selA = (h.target_mask == 0xFF) || (h.target_mask & 0x01);
      const uint8_t ok_mask   = (selA && ok) ? 0x01 : 0x00;
      const uint8_t fail_mask = (selA && !ok) ? 0x01 : 0x00;
      const uint8_t busy_mask = 0x00;
      uint8_t ap[8] = { ok_mask, fail_mask, busy_mask, 0, 0, 0, 0, 0 };
      const size_t outn = build_resp_ack(txbuf, sizeof(txbuf), h.seq, h.target_mask, RESP_OK, ap, sizeof(ap));
      udp.sendto(txbuf, outn, from);
      continue;
    }

    // Unknown command
    {
      uint8_t ap[8] = {0};
      const size_t outn = build_resp_ack(txbuf, sizeof(txbuf), h.seq, h.target_mask, RESP_UNKNOWN, ap, sizeof(ap));
      udp.sendto(txbuf, outn, from);
    }
  }

  udp.close();
  return 0;
}

