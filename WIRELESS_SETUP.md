# Sony Camera Wireless Connectivity Guide

## 🔌 Connection Options for Sony Cameras

### Supported Connection Types

| Camera Model | Ethernet | WiFi | USB |
|-------------|----------|------|-----|
| **Sony FX6** | ✅ Gigabit | ✅ WiFi 5 | ✅ USB-C |
| **Sony FX3** | ✅ Gigabit | ✅ WiFi 5 | ✅ USB-C |
| **Sony A74** | ❌ No Ethernet | ✅ WiFi 5 | ✅ USB-C |

**Important**: A74 cameras **ONLY support WiFi** - no Ethernet port available!

## 📶 Wireless Setup Options

### Option 1: Mixed Network (Recommended for 8-Cameras)
```
Pi ─── Ethernet ──── Gigabit Switch ──── WiFi Access Point
                           │
                    ┌─────────────────┐
                    │   FX6 Cameras   │ (Ethernet for stability)
                    │   FX3 Cameras   │ (Ethernet for high bandwidth)
                    └─────────────────┘
                           │
                       WiFi Network
                           │
                    ┌─────────────────┐
                    │   A74 Cameras   │ (WiFi only option)
                    └─────────────────┘
```

### Option 2: All-Wireless Setup
```
Pi ─── WiFi ──── WiFi Router/AP ──── All 8 Cameras (WiFi)
```

## 🚀 Wireless Configuration

### Configure Pi for Camera WiFi Network

```bash
# Check current wireless status
iwconfig wlan0

# Scan for camera WiFi networks
sudo iwlist wlan0 scan | grep -E "ESSID|Address|Quality"

# Connect to camera WiFi network
sudo wpa_passphrase "CAMERA_NETWORK" "password" >> /etc/wpa_supplicant/wpa_supplicant.conf
sudo wpa_cli -i wlan0 reconfigure
```

### Camera WiFi Network Settings

**For Sony cameras, typical WiFi settings:**
```bash
# Camera Access Point Mode
export CAMERA_WIFI_SSID="DIRECT-[camera_name]"
export CAMERA_WIFI_PASS="[camera_password]"

# Infrastructure Mode (via router)
export CAMERA_WIFI_NETWORK="PRODUCTION_CAMERAS"
export CAMERA_WIFI_PASS="ProductionPass123"
```

## ⚡ Performance Considerations

### Bandwidth Requirements (8-Camera Setup)

| Operation | Per Camera | 8 Cameras Total |
|-----------|------------|-----------------|
| **Control Commands** | 1-10 Kbps | 80 Kbps |
| **Live View (Low)** | 2-5 Mbps | 40 Mbps |
| **Live View (High)** | 10-15 Mbps | 120 Mbps |
| **File Transfer** | 50-100 Mbps | 800 Mbps |

### Network Recommendations

#### ✅ Good for Wireless:
- **Camera control commands** (ISO, aperture, record start/stop)
- **Status monitoring** (battery, recording state)
- **Low-quality preview streams**

#### ⚠️ Challenging for Wireless:
- **High-quality live view** from multiple cameras
- **Simultaneous file downloads** from all cameras
- **Real-time monitoring** of 8 camera feeds

#### ❌ Avoid on Wireless:
- **Professional live streaming** of multiple cameras
- **Large file transfers** during active production

## 🔧 Optimal Setup for Your 8-Camera System

### Professional Recommendation

```bash
# Hybrid approach - best of both worlds
# Ethernet for high-bandwidth cameras (FX6/FX3)
ETHERNET_CAMERAS="192.168.33.91-97"  # FX6 and FX3 cameras

# WiFi for cameras without ethernet (A74) and mobile setups
WIFI_CAMERAS="10.32.56.201-203"      # A74 cameras on WiFi
```

### Network Configuration Script

```bash
#!/bin/bash
# Dual-network camera setup

# Ethernet cameras (FX6, FX3)
export SONY_CAM1_IP="192.168.33.91"  # FX6 Main (Ethernet)
export SONY_CAM2_IP="192.168.33.92"  # FX6 B-Cam (Ethernet) 
export SONY_CAM3_IP="192.168.33.93"  # FX3 Wide (Ethernet)
export SONY_CAM5_IP="192.168.33.95"  # FX3 Stage (Ethernet)
export SONY_CAM7_IP="192.168.33.97"  # FX6 Side (Ethernet)

# WiFi cameras (A74 - no ethernet available)
export SONY_CAM4_IP="10.32.56.201"   # A74 Close (WiFi)
export SONY_CAM6_IP="10.32.56.202"   # A74 Overhead (WiFi)
export SONY_CAM8_IP="10.32.56.203"   # A74 Detail (WiFi)

# Different connection methods
ethernet_cameras=(1 2 3 5 7)
wifi_cameras=(4 6 8)
```

## 📶 WiFi Quality Tips

### For Reliable Wireless Operation:

1. **Dedicated 5GHz Network** for cameras only
2. **High-quality WiFi 6 router** (WiFi 6E even better)
3. **Minimize interference** - avoid 2.4GHz devices
4. **Strategic AP placement** - line of sight to cameras
5. **Backup Ethernet** for critical cameras when possible

### Camera WiFi Settings:

```bash
# A74 cameras (WiFi only) - optimized settings
export SONY_A74_WIFI_MODE="Infrastructure"  # Not AP mode
export SONY_A74_WIFI_CHANNEL="36"           # 5GHz channel
export SONY_A74_WIFI_BANDWIDTH="80MHz"      # Maximum bandwidth
```

## 🎬 Production Workflow

**For your 8-camera setup, I recommend:**

1. **Ethernet**: FX6 and FX3 cameras (5 cameras) - stable, high bandwidth
2. **WiFi**: A74 cameras (3 cameras) - no choice, WiFi only
3. **Pi Connection**: Ethernet primary, WiFi secondary for A74 access

This gives you **maximum reliability** for your professional production while accommodating the A74's WiFi-only limitation.

Would you like me to create the dual-network configuration scripts for this setup?