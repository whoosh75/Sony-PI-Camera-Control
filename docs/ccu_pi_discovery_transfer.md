CCU ↔ Pi CRSDK Discovery Trigger (Transfer Notes)

Summary
- CCU now sends a CCU1 discover command when a camera type is switched to a Sony model in roster type‑edit.
- Pi daemon handles CMD_DISCOVER (0x32) by calling connect_first_camera() and returns ACK.
- CCU uses PI UDP bridge on 192.168.0.14:5555 and polls Sony status adaptively (fast when active, slow when inactive, off when no Sony connected).

CCU changes (copy to Teensy repo)
1) CCU1 CMD_DISCOVER send
- src/backend_sony_crsdk.cpp
  - Added CMD_DISCOVER = 0x32
  - Added crsdk_request_discover()
  - Added adaptive polling controls and intervals
- src/backend_sony_crsdk.h
  - Declared crsdk_request_discover(), polling control setters

2) Trigger discover when Sony type selected
- src/CCU.ino
  - When roster type‑edit changes to Sony, call crsdk_request_discover()
  - Added CRSDK comms test serial commands:
    - C = print CRSDK status
    - c = request status now
    - Q = request status with target_mask=0xFF
  - CCU IP defaults set to 192.168.0.50/24 (see ccu_types.h)
  - Pi IP set to 192.168.0.14
  - Adaptive polling based on Sony connection + active selection

3) Network defaults
- src/ccu_types.h
  - NetConfig defaults changed to 192.168.0.50 / 255.255.255.0 / 192.168.0.1
- src/CCU.ino
  - NET_VERSION bumped to force reload defaults

Pi changes (copy into Pi repo)
1) Add discover command constant
- pi_controller/src/protocol.hpp
  - CMD_DISCOVER = 0x32

2) Handle discover command
- pi_controller/src/main.cpp
  - On CMD_DISCOVER: g_sony.connect_first_camera(); ACK with ok/fail mask

How to use
1) Ensure CCU and Pi are on 192.168.0.x. CCU should show 192.168.0.50 via Serial command L.
2) Run Pi daemon: SONY_PASS="..." ./run_ccu.sh 5555
3) On CCU, set a camera type to a Sony model in roster type‑edit.
4) Pi should attempt discovery immediately (connect_first_camera()).
5) Use Serial commands:
   - c then C to confirm CCU↔Pi comms (cam_last_rx_ms non‑zero)
   - Q to force ALL‑mask status request

Files to copy
CCU:
- src/CCU.ino
- src/backend_sony_crsdk.cpp
- src/backend_sony_crsdk.h
- src/ccu_types.h

Pi:
- pi_controller/src/protocol.hpp
- pi_controller/src/main.cpp
