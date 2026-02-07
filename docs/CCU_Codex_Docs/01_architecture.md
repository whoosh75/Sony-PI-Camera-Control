# 01 — System Architecture

## Roles
### Teensy 4.1 (Gateway + UI Host)
- Unified UI (OLED/encoder/buttons)
- Multicam routing (targets A–H and ALL)
- Camera backends:
  - RED backend: direct UDP control
  - ARRI backend: CAP over IP
  - Sony backend: proxy to Pi over CCU Bus transport
- Transport layer (matches motor project):
  - CCU Bus over UDP (LAN/Wi‑Fi)
  - CCU Bus over UART (transparent RF modem)
  - AUTO selection + link health

### Raspberry Pi 3B (Sony Adapter Only)
- Runs Sony SDK/toolkit
- Maintains sessions for multiple Sony cameras
- Exposes a simple CCU Bus endpoint (UDP, optionally UART if ever required)
- Returns ACK + cached state snapshots

## Control planes
### CCU Bus (Intent Plane)
This is the internal protocol representing UI intent:
- RECORD start/stop, exposure params, WB, shutter, ND, focus actions, etc.
- target selection: single slot A–H or ALL

CCU Bus must be vendor-neutral.

### Vendor planes (Execution)
- RED RCP/RCP2: UDP/Wi‑Fi/serial (implementation is Teensy local)
- ARRI CAP: IP (implementation is Teensy local)
- Sony: executed on Pi via SDK; Teensy sends intent to Pi

## Static IP assumptions
- Cameras are mostly configured with static IP.
- The registry stores: vendor, slot, ip, port, and capabilities.
- Sony discovery can still exist, but slot assignment should support direct static-IP binding.

## Physical placement
Recommended:
- Teensy + Pi placed camera-side on the camera network.
- Operator long-range uses transparent UART RF to send CCU Bus frames to the camera-side gateway.
