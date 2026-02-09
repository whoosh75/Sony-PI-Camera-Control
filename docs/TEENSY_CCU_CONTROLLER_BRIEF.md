# Teensy CCU Controller Brief (for Codex AI)

## Goal
Teensy 4.1 acts as a hardware controller (buttons/encoders/OLED) and communicates with the Raspberry Pi 3B CCU daemon over UDP. The Pi runs Sony CRSDK and handles camera control. The Teensy should only send CCU protocol packets and render responses.

## Transport
- UDP to Pi (default port 5555)
- Packet format defined in `pi_controller/src/protocol.hpp`

## Packet Header (16 bytes)
- magic: 0x43435531 ('CCU1')
- version: 1
- msg_type: 0x01 (request) / 0x81 (response)
- payload_len
- seq
- target_mask
- cmd_or_code
- flags

## Commands Teensy Can Send
### 1) Run/Stop Recording
- Command: `CMD_RUNSTOP` (0x10)
- Payload: 1 byte
  - 0x00 = stop
  - 0x01 = start
- Response: ACK with masks indicating success/failure per target.

### 2) Get Option Lists (for OLED menus)
- Command: `CMD_GET_OPTIONS` (0x20)
- Payload: 1 byte `opt_id`
  - ISO: `OPT_ISO` (0x01)
  - White Balance: `OPT_WHITE_BALANCE` (0x02)
  - Shutter: `OPT_SHUTTER` (0x03)
  - FPS: `OPT_FPS` (0x04)

- Response payload format (little-endian):
  - Byte 0: option id
  - Bytes 1–2: value_type (CRSDK `CrDataType`)
  - Bytes 3–4: count (N)
  - Bytes 5–8: current_value
  - Then N entries of 32-bit values

## Teensy UX Flow (Recommended)
1) On boot: request option lists for ISO/WB/Shutter/FPS.
2) Render list on OLED and allow scrolling selection.
3) When user selects a value, send a “set” command (to be added) or instruct Pi to apply the selected value.

## Current Status
- A74 USB recording works via `CrCommandId_MovieRecord` (Down/Up).
- A74 ISO/WB/Shutter/FPS values can be queried and set on the Pi.
- `CMD_GET_OPTIONS` is implemented on the Pi and returns per-camera option lists.
- A74 REC behavior is frozen as of 2026-02-08. Do not change run/stop semantics without re-validation.
- Freeze reference: `docs/A74_RECORD_FREEZE_2026-02-08.md`

## To Be Added (Next)
- Set-Value command (ISO/WB/Shutter/FPS) so Teensy can apply selections directly.
- Targeted camera addressing for multi-cam (target_mask already supported).
- Status query command for battery/media/recording state.

## References
- Protocol definition: `pi_controller/src/protocol.hpp`
- UDP server: `pi_controller/src/udp_server.cpp`
- CCU daemon logic: `pi_controller/src/main.cpp`
- Sony backend: `pi_controller/src/sony_backend.cpp`
