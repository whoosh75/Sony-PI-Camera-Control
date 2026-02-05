#include <cstdio>
#include <cstdint>
#include <unistd.h>
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

int main(int argc, char** argv) {
  const uint16_t port = (argc >= 2) ? (uint16_t)std::atoi(argv[1]) : 5555;

  UdpServer udp;
  if (!udp.open(port)) {
    std::fprintf(stderr, "Failed to open UDP port %u\n", port);
    return 1;
  }
  std::printf("ccu_daemon listening UDP :%u\n", port);
// Attempt camera connect at startup (non-fatal if camera not present)
g_sony.connect_first_camera();

  uint8_t rxbuf[512];
  uint8_t txbuf[512];

  while (true) {
// Non-blocking periodic connect retry (every ~2s) until a camera is present
static uint32_t last_try_ms = 0;
const uint32_t now_ms = (uint32_t)(::time(nullptr) * 1000u);
if (!g_sony.is_connected() && (now_ms - last_try_ms) > 2000u) {
  last_try_ms = now_ms;
  g_sony.connect_first_camera();
}

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

