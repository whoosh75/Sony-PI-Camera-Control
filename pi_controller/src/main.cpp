#include <cstdio>
#include <cstdint>
#include <unistd.h>
#include <algorithm>
#include <thread>
#include <chrono>
#include <array>
#include <mutex>
#include <vector>
#include <string>
#include <cstring>
#include "protocol.hpp"
#include "udp_server.hpp"
#include <cstdlib>
#include <ctime>
#include "sony_backend.hpp"

// CRSDK header included so we know headers + linkage still ok
#include "CRSDK/CameraRemote_SDK.h"

using namespace ccu;

static std::array<bool, 8> g_run_state = {}; // reported state per target
static std::array<ccu::SonyBackend, 8> g_sony;

struct SlotConfig {
  bool enabled = false;
  std::string user;
  std::string pass;
  std::string fingerprint;
  std::string accept_fingerprint;
  std::string camera_ip;
  std::string camera_mac;
};

static std::array<SlotConfig, 8> g_slots;
static std::mutex g_env_mutex;

static bool env_is_true(const char* v) {
  return v && v[0] && v[0] == '1';
}

static std::string env_key(const char* base, int idx) {
  return std::string(base) + "_" + std::to_string(idx);
}

static const char* env_slot(const char* base, int idx) {
  const std::string key = env_key(base, idx);
  const char* v = std::getenv(key.c_str());
  if (v && v[0]) return v;
  return std::getenv(base);
}

static SlotConfig load_slot_config(int idx) {
  SlotConfig cfg;
  const char* enable = std::getenv(env_key("SONY_ENABLE", idx).c_str());
  const char* ip = env_slot("SONY_CAMERA_IP", idx);
  const char* mac = env_slot("SONY_CAMERA_MAC", idx);
  const char* user = env_slot("SONY_USER", idx);
  const char* pass = env_slot("SONY_PASS", idx);
  const char* fp = env_slot("SONY_FINGERPRINT", idx);
  const char* accept = env_slot("SONY_ACCEPT_FINGERPRINT", idx);

  cfg.enabled = env_is_true(enable) || (ip && ip[0]);
  if (user) cfg.user = user;
  if (pass) cfg.pass = pass;
  if (fp) cfg.fingerprint = fp;
  if (accept) cfg.accept_fingerprint = accept;
  if (ip) cfg.camera_ip = ip;
  if (mac) cfg.camera_mac = mac;
  return cfg;
}

struct EnvBackupEntry {
  std::string key;
  bool had = false;
  std::string value;
};

class EnvOverride {
public:
  explicit EnvOverride(const SlotConfig& cfg) { apply(cfg); }
  ~EnvOverride() { restore(); }

private:
  std::vector<EnvBackupEntry> m_entries;

  void save_and_set(const char* key, const std::string& value) {
    if (value.empty()) return;
    EnvBackupEntry entry;
    entry.key = key;
    const char* prior = std::getenv(key);
    if (prior) {
      entry.had = true;
      entry.value = prior;
    }
    m_entries.push_back(entry);
    setenv(key, value.c_str(), 1);
  }

  void apply(const SlotConfig& cfg) {
    save_and_set("SONY_USER", cfg.user);
    save_and_set("SONY_PASS", cfg.pass);
    save_and_set("SONY_FINGERPRINT", cfg.fingerprint);
    save_and_set("SONY_ACCEPT_FINGERPRINT", cfg.accept_fingerprint);
    save_and_set("SONY_CAMERA_IP", cfg.camera_ip);
    save_and_set("SONY_CAMERA_MAC", cfg.camera_mac);
  }

  void restore() {
    for (auto it = m_entries.rbegin(); it != m_entries.rend(); ++it) {
      if (it->had) {
        setenv(it->key.c_str(), it->value.c_str(), 1);
      } else {
        unsetenv(it->key.c_str());
      }
    }
  }
};

static bool slot_selected(uint8_t mask, int idx) {
  if (mask == 0xFF) return g_slots[idx].enabled;
  return (mask & (1u << idx)) != 0;
}

static int pick_slot(uint8_t mask) {
  for (int i = 0; i < 8; ++i) {
    if (slot_selected(mask, i)) return i;
  }
  return -1;
}

static bool connect_slot(int idx) {
  if (!g_slots[idx].enabled) return false;
  std::lock_guard<std::mutex> lock(g_env_mutex);
  EnvOverride env(g_slots[idx]);
  return g_sony[idx].connect_first_camera();
}

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

  for (int i = 0; i < 8; ++i) {
    g_slots[i] = load_slot_config(i);
  }
  bool any_enabled = false;
  for (int i = 0; i < 8; ++i) {
    if (g_slots[i].enabled) { any_enabled = true; break; }
  }
  if (!any_enabled) {
    g_slots[0].enabled = true;
  }

  UdpServer udp;
  if (!udp.open(port)) {
    std::fprintf(stderr, "Failed to open UDP port %u\n", port);
    return 1;
  }
  std::printf("ccu_daemon listening UDP :%u\n", port);

  // Background connect loop (non-blocking for UDP)
  std::thread connect_thread([]() {
    while (true) {
      for (int i = 0; i < 8; ++i) {
        if (!g_slots[i].enabled) continue;
        if (!g_sony[i].is_connected()) {
          connect_slot(i);
        }
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

      uint8_t ok_mask = 0;
      uint8_t fail_mask = 0;
      uint8_t busy_mask = 0;
      uint8_t state_run_mask = 0;
      uint8_t state_known_mask = 0;

      for (int i = 0; i < 8; ++i) {
        if (!slot_selected(h.target_mask, i)) continue;
        const bool ok = g_sony[i].set_runstop(run);
        if (ok) {
          ok_mask |= (1u << i);
          g_run_state[i] = run;
          state_known_mask |= (1u << i);
        } else {
          fail_mask |= (1u << i);
        }
        if (g_run_state[i]) state_run_mask |= (1u << i);
      }

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

      const int slot = pick_slot(h.target_mask);
      if (slot < 0) {
        uint8_t ap[8] = {0};
        const size_t outn = build_resp_ack(txbuf, sizeof(txbuf), h.seq, h.target_mask, RESP_UNKNOWN, ap, sizeof(ap));
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
      const bool ok = g_sony[slot].get_property_options(prop_code, opts);
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
      const int slot = pick_slot(h.target_mask);
      if (slot < 0) {
        uint8_t ap[8] = {0};
        const size_t outn = build_resp_ack(txbuf, sizeof(txbuf), h.seq, h.target_mask, RESP_UNKNOWN, ap, sizeof(ap));
        udp.sendto(txbuf, outn, from);
        continue;
      }

      const bool ok = g_sony[slot].get_status(st);
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

      uint8_t payload[128] = {0};
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

      // Append connection type + model string
      uint8_t conn_type = 0;
      const std::string& conn = g_sony[slot].connection_type();
      if (conn == "USB") conn_type = 1;
      else if (conn == "IP" || conn == "Ethernet") conn_type = 2;

      const std::string& model = g_sony[slot].camera_model();
      const uint8_t model_len = (uint8_t)std::min<size_t>(model.size(), 32);

      payload[off++] = conn_type;
      payload[off++] = model_len;
      if (model_len > 0) {
        std::memcpy(payload + off, model.data(), model_len);
        off += model_len;
      }

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

      uint8_t ok_mask = 0;
      uint8_t fail_mask = 0;
      uint8_t busy_mask = 0;
      for (int i = 0; i < 8; ++i) {
        if (!slot_selected(h.target_mask, i)) continue;
        const bool ok = g_sony[i].capture_still(with_af);
        if (ok) ok_mask |= (1u << i);
        else fail_mask |= (1u << i);
      }
      uint8_t ap[8] = { ok_mask, fail_mask, busy_mask, 0, 0, 0, 0, 0 };
      const size_t outn = build_resp_ack(txbuf, sizeof(txbuf), h.seq, h.target_mask, RESP_OK, ap, sizeof(ap));
      udp.sendto(txbuf, outn, from);
      continue;
    }

    if (h.cmd_or_code == CMD_DISCOVER) {
      uint8_t ok_mask = 0;
      uint8_t fail_mask = 0;
      uint8_t busy_mask = 0;
      for (int i = 0; i < 8; ++i) {
        if (!slot_selected(h.target_mask, i)) continue;
        const bool ok = connect_slot(i);
        if (ok) ok_mask |= (1u << i);
        else fail_mask |= (1u << i);
      }
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

