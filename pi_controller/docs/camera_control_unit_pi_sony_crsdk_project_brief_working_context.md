# Camera Control Unit (CCU)

**Platform**: Raspberry Pi (ARM64)  
**Language**: C++17  
**Editor/Tooling**: VS Code + CMake + GCC 14  
**SDK**: Sony Camera Remote SDK (CRSDK) v2.00.00 (Linux ARM64)

---

## 1. Project Purpose

The Camera Control Unit (CCU) is a **headless daemon** running on a Raspberry Pi that:

1. Connects to Sony cinema cameras (e.g. Burano / Venice / MPC-series) **over Ethernet** using Sony CRSDK.
2. Exposes a **simple UDP protocol** for external controllers (Teensy TX, CLI tools, etc.).
3. Translates external commands (e.g. **RUN / STOP**) into Sony camera commands via CRSDK.

This system is intended for **professional camera control** and must be:
- Deterministic
- Robust to reconnects
- Non-interactive (no stdin prompts)

---

## 2. High-Level Architecture

```
┌────────────┐      UDP       ┌──────────────────┐      CRSDK       ┌──────────────┐
│ Controller │ ─────────────▶ │ ccu_daemon (Pi)  │ ──────────────▶ │ Sony Camera  │
│ (Teensy / │                │                  │                │ (Ethernet)  │
│  ccu_cli) │ ◀───────────── │                  │ ◀───────────── │              │
└────────────┘      ACK       └──────────────────┘     Status       └──────────────┘
```

### Components

- **ccu_daemon**  
  Long-running UDP server + Sony backend

- **sony_backend**  
  Thin wrapper around CRSDK (connect, fingerprint, commands)

- **ccu_cli**  
  Minimal test client to send RUN/STOP commands

---

## 3. Repository Layout

```
camera-control/
├── pi_controller/
│   ├── CMakeLists.txt
│   ├── src/
│   │   ├── main.cpp              # UDP daemon entry point
│   │   ├── sony_backend.hpp      # SonyBackend class definition
│   │   ├── sony_backend.cpp      # CRSDK integration
│   │   ├── udp_server.cpp        # UDP RX/TX
│   │   └── protocol.cpp          # Packet definitions
│   └── build/                    # out-of-tree build (ignored)
│
├── pi_controller/
│   └── docs/
│       └── camera_control_unit_pi_sony_crsdk_project_brief_working_context.md
│
└── README.md
```

---

## 4. Build & Run Instructions

### 4.1 Dependencies

- Raspberry Pi OS 64-bit
- `cmake >= 3.25`
- `g++ >= 13` (14 currently used)
- Sony CRSDK unpacked to:

```
/home/whoosh/CrSDK_v2.00.00_20251030a_Linux64ARMv8
```

### 4.2 Build

```bash
cd ~/camera-control/pi_controller
mkdir -p build
cd build

cmake -DCRSDK_ROOT=/home/whoosh/CrSDK_v2.00.00_20251030a_Linux64ARMv8 ..
cmake --build . -j
```

### 4.3 Run daemon (Ethernet camera)

```bash
sudo -E env \
SONY_PASS="Password1" \
LD_LIBRARY_PATH="/home/whoosh/CrSDK_v2.00.00_20251030a_Linux64ARMv8/build:
/home/whoosh/CrSDK_v2.00.00_20251030a_Linux64ARMv8/external/crsdk:
/home/whoosh/CrSDK_v2.00.00_20251030a_Linux64ARMv8/external/crsdk/CrAdapter:
/home/whoosh/CrSDK_v2.00.00_20251030a_Linux64ARMv8/external/opencv/Linux" \
./ccu_daemon 5555
```

> **Important**: Username is **not required**. CRSDK sample uses SSH password only.

### 4.4 Send commands

```bash
./ccu_cli 127.0.0.1 5555 run ff
./ccu_cli 127.0.0.1 5555 stop ff
```

---

## 5. Sony CRSDK Integration (Current State)

### Working

- CRSDK init + direct Ethernet camera object creation (IP + MAC)
- Fingerprint retrieval and padded Base64 handling
- Authenticated connect with password and fingerprint
- Record start/stop on MPC-2610 using MovieRecButtonToggle (Down -> Up)
- Retry/backoff for record start until camera is ready
- UDP server and protocol plumbing
- Direct CRSDK API control confirmed; CLI automation is no longer used

### Known Limits (MPC-2610 / Burano)

1. **Some properties are not exposed**
  - `CrDeviceProperty_MovieRecButtonToggleEnableStatus` not present
  - `CrDeviceProperty_RecordingState` not present
  - `CrDeviceProperty_IsoSensitivity` not present
  - `CrDeviceProperty_Movie_Recording_FrameRateSetting` may not be exposed

2. **Record start returns `0x8402` even when it works**
  - Camera starts recording despite `CrError_Api_InvalidCalled`
  - The daemon can treat `0x8402` as success when enabled

---

## 6. Design Constraints (Must Preserve)

- No interactive input (no stdin prompts)
- Daemon must auto-reconnect
- No Sony sample code modifications
- All camera control flows through SonyBackend

---

## 7. Immediate Next Tasks (TODO)

### Critical

1. **Make run/stop resilient for MPC-2610**
  - Keep Down -> Up toggle sequence
  - Keep retries on start
  - Optional soft-success for `0x8402`

2. **Confirm property support per camera**
  - ISO/WB/FPS may be model-specific
  - Use property list to decide supported controls

### Functional

3. Implement camera status polling (if supported)
4. Extend protocol beyond RUN/STOP

---

## 8. Ground Rules for AI / Codex

- DO NOT invent CRSDK APIs
- Always check headers in:
  `CrSDK_v2.00.00_20251030a_Linux64ARMv8/app/CRSDK/`
- Prefer minimal diffs
- Be explicit about file names and paste locations
- Never assume interactive input

---

## 9. Success Criteria

The project is considered **complete** when:

- `ccu_daemon` connects to the camera reliably
- `ccu_cli run/stop` triggers camera record reliably
- No manual intervention is required after boot

---

End of document

