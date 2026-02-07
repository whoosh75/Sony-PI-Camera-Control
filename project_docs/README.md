# Sony PI - Camera Control for Raspberry Pi - Updated 2026-02-06

A camera control solution for Sony cinema cameras (MPC-2610, A74) with CLI automation working but **true API blocked by SDK initialization failure**.

## ⚠️ STATUS: CLI AUTOMATION WORKING - TRUE API BLOCKED

**CLI Automation (Slow)**: ✅ Working via RemoteCli wrapper  
**True CRSDK API (Fast)**: ❌ **BLOCKED - SDK initialization fails with error code 1**  
**Performance Issue**: CLI automation too slow for production use with MPC-2610

### 🔍 Current Technical Status
- **CLI Automation**: RemoteCli menu navigation works but introduces significant latency
- **CRSDK Initialization**: All custom programs fail `SCRSDK::Init()` with error code 1  
- **Library Dependencies**: OpenCV libraries linked correctly but SDK still fails
- **Performance Gap**: Need direct API calls for fast camera control, not menu automation
- **Root Blocker**: Unknown SDK initialization requirement preventing true API access

---

## 🎯 Features

### ✅ What Works (CLI Automation)
- ✅ **Menu Navigation** - RemoteCli successfully navigates camera menus
- ✅ **Recording Commands** - Start/stop recording via menu automation  
- ✅ **Photo Capture** - Still photo capture through menu system
- ✅ **Dual Camera Support** - Both MPC-2610 and A74 detected
- ✅ **Authentication** - Reliable camera connection established

### ❌ What's Blocked (True API)
- ❌ **Direct CRSDK API** - SDK initialization fails in all custom programs
- ❌ **Fast Performance** - Menu automation introduces unacceptable latency 
- ❌ **Production Speed** - Current solution too slow for real-world use
- ❌ **Direct Commands** - Cannot use `SCRSDK::SendCommand()` for instant control

## 🔧 Hardware Requirements

- **Raspberry Pi** (ARM64) with Ethernet connectivity
- **Sony MPC-2610 Cinema Camera** (or compatible Sony professional camera)
- **Network Connection** - Direct Ethernet or switch connection
- **Sony Camera Remote SDK v2.00.00** (Linux ARM64 version)

## 🚀 Quick Start

### 1. Environment Setup

```bash
# Set camera connection details
export SONY_CAMERA_IP="192.168.33.94"          # Your camera's IP
export SONY_CAMERA_MAC="50:26:EF:B8:3F:2C"     # Your camera's MAC address
export SONY_ACCEPT_FINGERPRINT="1"              # Auto-accept SSH fingerprint
export SONY_PASS="Password1"                    # Camera SSH password

# Set Sony SDK library path
export LD_LIBRARY_PATH="/path/to/CrSDK/external/crsdk:/path/to/CrSDK/external/crsdk/CrAdapter:/path/to/CrSDK/external/opencv/Linux"
```

### 2. Current Solution: CLI Automation (Slow)

```bash
cd pi_controller/build

# Test CLI automation wrapper
python3 working_sony_api.py
```

**⚠️ Performance Warning:**
```
🎯 Sony Camera API Demo
=====================

📹 Testing 3-second recording...
🎥 Recording for 3 seconds...
🎬 Starting video recording...  # <- Menu navigation delay
✅ Recording started!             # <- 2-3 second latency
⏹️ Stopping video recording...
✅ Recording stopped!
```

**Issue**: Each command involves full menu navigation through RemoteCli, causing **2-3 second delays** unsuitable for production use.

### 3. Blocked: True API Implementation

```cpp
// This is what we NEED but fails:
SCRSDK::SendCommand(device_handle, 
                   SCRSDK::CrCommandId_MovieRecord, 
                   SCRSDK::CrCommandParam_Down);  // Instant execution
```

**Problem**: All SDK initialization attempts fail:
```bash
./working_sdk_test
# Output: Failed to initialize SDK with error: 1
```

### 4. Build Additional Tools (Optional)

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