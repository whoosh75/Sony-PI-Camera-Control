# Sony PI - Camera Control for Raspberry Pi - Updated 2026-02-07

A camera control solution for Sony cameras (MPC-2610, A74) using CRSDK with a UDP daemon for CCU control.

## ✅ CURRENT STATUS

**CRSDK USB (A74)**: ✅ Working (record/settings/stills via UDP daemon)  
**A74 Record Start/Stop**: ✅ Frozen baseline (validated 2026-02-08)  
**CRSDK Ethernet (MPC-2610)**: ⚠️ Partial (auth OK; some calls return 0x8402)  
**UDP Daemon (CCU1)**: ✅ RUNSTOP, GET_OPTIONS, GET_STATUS, CAPTURE_STILL, DISCOVER  
**Systemd Autostart**: ✅ Service installed and documented  
**Multi‑camera**: ✅ Per‑slot env configuration (A..H)

### 🔍 Current Technical Notes
- USB control does not require fingerprint/password.
- Ethernet control still hits 0x8402 for some operations; direct IP connection is preferred.
- Status payload now appends `conn_type` and `model`.

---

## 🎯 Features

### ✅ What Works
- ✅ **Direct CRSDK API (USB)** - A74 record/settings/stills
- ✅ **UDP Daemon** - CCU1 protocol: RUNSTOP/GET_OPTIONS/GET_STATUS/CAPTURE_STILL/DISCOVER
- ✅ **Battery/Media Status** - Normalized to percent + minutes
- ✅ **Model/Connection Type** - Appended to status payload
- ✅ **Systemd Autostart** - Service + env file
- ✅ **Multi‑camera Slots** - Per‑slot env variables (A..H)

### CCU ↔ Pi Discovery
- CCU sends `CMD_DISCOVER` (0x32) when a Sony type is selected.
- Pi handles discovery and attempts `connect_first_camera()`.
- Details: [docs/ccu_pi_discovery_transfer.md](docs/ccu_pi_discovery_transfer.md)

### ⚠️ In Progress / Partial
- ⚠️ **Ethernet Control** - MPC‑2610 still returns 0x8402 for some API calls

## 🔧 Hardware Requirements

- **Raspberry Pi** (ARM64) with Ethernet connectivity
- **Sony MPC-2610 Cinema Camera** (or compatible Sony professional camera)
- **Network Connection** - Direct Ethernet or switch connection
- **Sony Camera Remote SDK v2.00.00** (Linux ARM64 version)

## 🚀 Quick Start

### 1. Environment Setup (USB)

```bash
export SONY_PASS="Password1"  # Only required for Ethernet; USB ignores it
```

### 2. Start the UDP daemon

```bash
./start_ccu.sh 5555
```

### 3. Build Additional Tools (Optional)

```bash
cd pi_controller
mkdir build && cd build
cmake .. -DCRSDK_ROOT=/path/to/your/sony_sdk
make
```

**Available Programs:**
- `working_sony_api.py` - ✅ Working Python API (main interface)
- `working_rec.sh` - ✅ Working shell script backend
- `ccu_diag` - Camera diagnostic tool
- `ccu_daemon` - Multi-camera daemon

## 📡 Network Configuration

Your Raspberry Pi and Sony camera must be on the same network:

```bash
# Check Pi network interface
ip addr show eth0

# Verify camera connectivity  
ping 192.168.33.94

# Check ARP table for camera MAC
ip neigh show 192.168.33.94
```

## 🔍 Technical Architecture

### Connection Method: Direct IP vs Enumeration

This project uses **direct IP connection** instead of Sony's standard enumeration:

**❌ Enumeration Approach (Unreliable):**
```cpp
SCRSDK::EnumCameraObjects(&enumInfo);  // Fails with 0x00008703
```

**✅ Direct IP Approach (Reliable):**
```cpp
SCRSDK::CreateCameraObjectInfoEthernetConnection(
    &pCam, 
    SCRSDK::CrCameraDeviceModelList::CrCameraDeviceModel_MPC_2610,
    ipAddr, macBuf, 1
);
```

### Key Technical Insights

1. **Camera Model Critical**: Must use `CrCameraDeviceModel_MPC_2610` specifically
2. **Compilation Flags**: `-fsigned-char` required for Sony CRSDK headers
3. **Authentication**: SSH password + fingerprint handling
4. **MAC Address**: Required for reliable Ethernet connection

## 📚 Project Structure

```
Sony-PI/
├── pi_controller/                 # Main camera control application
│   ├── src/                      # Source code
│   │   ├── sony_backend.cpp      # Camera connection & control logic
│   │   ├── sony_backend.hpp      # Backend interface
│   │   ├── protocol.cpp          # Communication protocol
│   │   └── sony_sample/          # Sony SDK wrapper classes
│   ├── test_direct_simple.cpp    # Connection test application
│   ├── CMakeLists.txt            # Build configuration
│   └── build/                    # Compiled binaries
├── sony_crsdk_authentication_connection_findings.md  # Complete technical documentation
├── SHORTCUTS.md                  # Development shortcuts
├── README.md                     # This file
└── start_ccu.sh                 # Startup script
```

## 🛠️ Development

### Building from Source

```bash
# Install dependencies
sudo apt update
sudo apt install cmake g++ build-essential

# Clone repository
git clone https://github.com/yourusername/Sony-PI.git
cd Sony-PI

# Build project
cd pi_controller
mkdir build && cd build
cmake ..
make
```

### Testing Changes

```bash
# Quick connection test
./test_direct_simple

# Full diagnostic
./build/ccu_diag

# Compare with Sony's reference
/path/to/CrSDK/build/RemoteCli
```

## 📖 Documentation

### Complete Technical Guide
See [`sony_crsdk_authentication_connection_findings.md`](sony_crsdk_authentication_connection_findings.md) for:
- **Detailed connection troubleshooting**
- **Sony CRSDK API reference**
- **Authentication flow analysis**  
- **Compilation requirements**
- **Network setup guide**

### Key Command Reference
See [`SHORTCUTS.md`](SHORTCUTS.md) for:
- **Development shortcuts**
- **Common build commands**
- **Testing procedures**

### Freeze Baseline
See [`docs/A74_RECORD_FREEZE_2026-02-08.md`](docs/A74_RECORD_FREEZE_2026-02-08.md) for the locked A74 REC behavior and non-regression rule.

## 🎬 Camera Operations (Coming Soon)

With connection established, implement camera control:

```cpp
// Example camera operations
SCRSDK::SendCommand(deviceHandle, CrCommandId_Release, CrCommandParam_Down);
SCRSDK::SetDeviceProperty(deviceHandle, CrDeviceProperty_ISO, &isoValue, sizeof(isoValue));
```

## 🤝 Contributing

This project welcomes contributions! Areas for development:

- **Camera Control Operations** - ISO, aperture, shutter speed control
- **Live View Implementation** - Real-time camera preview
- **Content Transfer** - Photo/video download from camera
- **Web Interface** - Remote camera control via browser
- **Multiple Camera Support** - Control multiple cameras simultaneously

## 📝 License

This project is open source. Please respect Sony's Camera Remote SDK license terms.

## 🙏 Acknowledgments

- **Sony Corporation** - Camera Remote SDK
- **Raspberry Pi Foundation** - ARM64 Linux platform
- **Open Source Community** - Development tools and libraries

---

**Status**: ✅ Connection Solved | 🚧 Camera Control In Development

**Last Updated**: February 5, 2026
