# Sony PI - Camera Control for Raspberry Pi

A **complete camera control solution** for Sony cinema cameras (MPC-2610) running on Raspberry Pi with Sony Camera Remote SDK v2.00.00.

## ✅ STATUS: FULLY WORKING CONNECTION

**Connection to Sony MPC-2610 camera successfully established!** 🎉

This project solves the challenging Sony CRSDK authentication and connection issues on Linux ARM64 platforms through direct IP connection methodology.

---

## 🎯 Features

- ✅ **Direct IP Connection** - Bypasses unreliable enumeration
- ✅ **Sony MPC-2610 Support** - Professional cinema camera control
- ✅ **Raspberry Pi Optimized** - ARM64 Linux compatibility
- ✅ **Authentication Solved** - SSH-based camera authentication
- ✅ **Complete Documentation** - Extensive troubleshooting guide
- 🚧 **Camera Control API** - Ready for operation implementation

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

### 2. Test Connection

```bash
cd pi_controller

# Compile test application
g++ -o test_direct_simple test_direct_simple.cpp \
    -I/path/to/CrSDK/app \
    -L/path/to/CrSDK/external/crsdk \
    -lCr_Core -std=c++11 -fsigned-char

# Test camera connection
./test_direct_simple
```

**Expected Result:**
```
Direct camera connection test for MPC-2610...
Using MAC: 50:26:EF:B8:3F:2C
Creating camera object for IP 192.168.33.94 with model MPC_2610...
Camera object created successfully!
SUCCESS: Direct IP connection to MPC-2610 camera established!
Camera model: MPC-2610
Camera name: MPC-2610
```

### 3. Build Camera Control System

```bash
cd pi_controller
mkdir build && cd build
cmake ..
make
```

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