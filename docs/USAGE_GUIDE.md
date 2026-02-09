# USAGE GUIDE - Sony Camera API

## A74 Recording Freeze
- A74 (`ILCE-7M4`, USB) REC behavior is frozen and validated.
- Start: `CMD_RUNSTOP` payload `1` -> camera records.
- Stop: `CMD_RUNSTOP` payload `0` -> camera stops.
- Status: `CMD_GET_STATUS.recording_state` updates quickly (`0` idle, non-zero recording).
- Baseline reference: [`docs/A74_RECORD_FREEZE_2026-02-08.md`](docs/A74_RECORD_FREEZE_2026-02-08.md)

## Quick Start

### 1. Basic Recording
```bash
cd /home/whoosh/camera-control/pi_controller/build
python3 working_sony_api.py
```

### 1b. A74 USB Direct Tests
```bash
cd /home/whoosh/camera-control/pi_controller/build
./a74_usb_record_test     # A74 record start/stop (MovieRecord)
./a74_usb_settings_test   # ISO/WB/Shutter/FPS validation
./a74_usb_stills_test     # Stills settings + shutter release/AF+release
```

### 2. Custom Scripts
```python
#!/usr/bin/env python3
from working_sony_api import SonyCameraAPI
import time

camera = SonyCameraAPI()

# Start recording
camera.start_recording()
time.sleep(10)  # Record for 10 seconds
camera.stop_recording()

# Take a photo
camera.take_photo()
```

### 3. Shell Script Usage
```bash
# Direct shell access
cd /home/whoosh/camera-control/pi_controller/build
./working_rec.sh start    # Start recording
./working_rec.sh stop     # Stop recording  
./working_rec.sh toggle   # Toggle state
./working_rec.sh photo    # Take photo
```

## Advanced Features

### Timed Recording
```python
camera = SonyCameraAPI()
camera.record_for_duration(30)  # 30-second clip
```

### Error Handling
```python
camera = SonyCameraAPI()
if camera.start_recording():
    print("Recording started successfully")
else:
    print("Failed to start recording")
```

### Multiple Clips
```python
camera = SonyCameraAPI()
for i in range(5):
    print(f"Recording clip {i+1}")
    camera.record_for_duration(5)
    time.sleep(1)  # Brief pause between clips
```

## Supported Cameras
- ✅ Sony MPC-2610 (WiFi)
- ✅ Sony A74 / ILCE-7M4 (USB)

## Options Query (OLED Menus)
The Teensy can request option lists over UDP using `CMD_GET_OPTIONS`:
- `OPT_ISO` (0x01)
- `OPT_WHITE_BALANCE` (0x02)
- `OPT_SHUTTER` (0x03)
- `OPT_FPS` (0x04)

Response payload:
- Byte 0: option id
- Bytes 1–2: value_type (CRSDK `CrDataType`)
- Bytes 3–4: count
- Bytes 5–8: current_value
- Following: list of 32-bit values (count entries)

## Status + Stills Capture (UDP)
New commands:
- `CMD_GET_STATUS` (0x30): battery/media status
- `CMD_CAPTURE_STILL` (0x31): payload 1 byte (`0`=release, `1`=AF+release)

Status response (11 x uint32 + model info):
1) `battery_level` (%)
2) `battery_remain` (%)
3) `battery_remain_unit` (1=percent)
4) `recording_media`
5) `movie_recording_media`
6) `media_slot1_status`
7) `media_slot1_remaining_number`
8) `media_slot1_remaining_time` (minutes)
9) `media_slot2_status`
10) `media_slot2_remaining_number`
11) `media_slot2_remaining_time` (minutes)

Followed by:
- `conn_type` (1=USB, 2=IP/Ethernet, 0=Unknown) [uint8]
- `model_len` [uint8]
- `model` (ASCII, `model_len` bytes)

Note: media remaining time is returned in minutes (converted from camera seconds). Battery fields are normalized to percent when available.

## CCU Status Polling (UDP)
The CCU should poll `CMD_GET_STATUS` to show:
- Battery percent (from fields 1–2)
- Slot remaining time in minutes (fields 8 and 11)

Example (movie mode):
- Slot1: 132 min (2h12m)
- Slot2: 156 min (2h36m)

Minimal CCU parsing (pseudo-code):
1. Send `CMD_GET_STATUS` with empty payload.
2. Parse 11 x uint32 values from response payload.
3. Use:
    - `battery_percent = payload[0]` (or payload[1])
    - `slot1_minutes = payload[7]`
    - `slot2_minutes = payload[10]`

## Autostart on Pi boot (systemd)
1) Copy the service file to systemd:
    - Source: [systemd/ccu-daemon.service](systemd/ccu-daemon.service)
    - Destination: /etc/systemd/system/ccu-daemon.service

2) Create the environment file:
    - Example: [systemd/ccu-daemon.env.example](systemd/ccu-daemon.env.example)
    - Destination: /etc/ccu-daemon.env
    - Required: `SONY_PASS`

3) Enable and start:
    - `sudo systemctl daemon-reload`
    - `sudo systemctl enable ccu-daemon`
    - `sudo systemctl start ccu-daemon`

4) Check logs:
    - `journalctl -u ccu-daemon -f`

## Multiple cameras with different credentials
Use per-slot environment variables. Slots 0..7 map to CCU targets A..H.

Recommended (Ethernet) per-slot variables:
- `SONY_ENABLE_N=1`
- `SONY_CAMERA_IP_N=...`
- `SONY_PASS_N=...`
- `SONY_USER_N=...` (optional)
- `SONY_FINGERPRINT_N=...` (optional)
- `SONY_ACCEPT_FINGERPRINT_N=1`

Example (two cameras):
- Slot 0 (A): `SONY_ENABLE_0=1`, `SONY_CAMERA_IP_0=192.168.0.70`, `SONY_PASS_0=Password1`
- Slot 1 (B): `SONY_ENABLE_1=1`, `SONY_CAMERA_IP_1=192.168.0.71`, `SONY_PASS_1=Password2`

Note: If you omit `SONY_CAMERA_IP_N`, slot selection is not deterministic with multiple cameras.
If no per-slot variables are set, the daemon falls back to slot 0 using the global `SONY_*` variables.

## File Locations
- **API**: `/home/whoosh/camera-control/pi_controller/build/working_sony_api.py`
- **Backend**: `/home/whoosh/camera-control/pi_controller/build/working_rec.sh`
- **RemoteCli**: `/home/whoosh/camera-control/RemoteCli`

## Troubleshooting

### Camera Not Responding
1. Check camera power and connection
2. Verify network connectivity (MPC-2610) or USB connection (A74)
3. Test with RemoteCli directly: `/home/whoosh/camera-control/RemoteCli`

### Recording Fails
1. Check camera mode (should be in video mode)
### Stills Capture Fails
1. Ensure the physical mode switch is set to stills
2. Close any camera menus
3. Confirm PC Remote is enabled for USB
2. Verify storage space on camera
3. Check recording settings on camera

### A74 Ethernet Connect
1. Confirm camera IP is reachable (same subnet as the Pi)
2. Set credentials in the shell environment before connecting
3. Set `SONY_CAMERA_IP` and (optional) `SONY_CAMERA_MAC` for deterministic connection

### API Import Errors
1. Ensure you're in the correct directory: `pi_controller/build`
2. Check file exists: `ls working_sony_api.py`
3. Verify Python 3 is installed: `python3 --version`
