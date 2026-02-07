# 🔄 Flexible Connection System for Sony Cameras

## Connection Priority System

### Multi-Path Redundancy
Each camera supports **3 connection methods** with automatic fallback:

```
1st Priority: Ethernet    (FX6, FX3 - fastest, most reliable)
2nd Priority: WiFi        (All cameras - flexible, mobile)  
3rd Priority: USB         (All cameras - direct, backup)
```

## 📱 Scalable 1-8 Camera Setup

### Production Scale Examples:

```bash
# 1-Camera Setup (Solo/Interview)
test_cameras 1          # Just main camera

# 3-Camera Setup (Basic Production)  
test_cameras 3          # Main, B-cam, Wide

# 6-Camera Setup (Professional)
test_cameras 6          # Full coverage + speciality angles

# 8-Camera Setup (Maximum)
test_cameras 8          # Complete multi-camera production
```

## 🎯 Camera Models & Connection Capabilities

| Camera | Ethernet | WiFi | USB | Notes |
|--------|----------|------|-----|-------|
| **FX6** | ✅ Primary | ✅ Backup | ✅ Emergency | Gigabit Ethernet preferred |
| **FX3** | ✅ Primary | ✅ Backup | ✅ Emergency | Gigabit Ethernet preferred |  
| **A74** | ❌ None | ✅ Primary | ✅ Backup | **WiFi/USB only** |

## 🔌 USB Connection Advantages

### Why USB is Critical for A74:
- **No Ethernet port** - WiFi can be unreliable in RF-heavy environments
- **Direct connection** - Not affected by network congestion
- **Stable power** - Camera can charge while connected  
- **Emergency backup** - Works when WiFi fails

### USB Connection Detection:
```bash
# Scan for connected Sony cameras
lsusb | grep -i sony

# Typical output:
# Bus 001 Device 004: ID 054c:0994 Sony Corp. ILCE-7M4 [A74]
# Bus 001 Device 005: ID 054c:0b20 Sony Corp. FX3
```

## 🚀 Quick Setup Commands

### Initialize Flexible System:
```bash
./setup_flexible_cameras.sh
source flexible_camera_config.sh
```

### Test Individual Cameras:
```bash
test_connection 1           # Auto-detect best connection for Camera 1
test_connection 4 USB       # Force USB connection for Camera 4 (A74)
test_connection 2 ETHERNET  # Force Ethernet for Camera 2 (FX6)
test_connection 6 WIFI      # Force WiFi for Camera 6 (A74)
```

### Production Workflows:
```bash
# Setup and test 4-camera production
setup_production 4

# Monitor all cameras continuously  
monitor_all 8

# Test specific connection type across cameras
for i in {1..8}; do test_connection $i USB; done
```

## 🔧 Connection Troubleshooting

### Automatic Fallback Logic:
1. **Try Ethernet** (if camera supports it)
2. **Fallback to WiFi** (if Ethernet fails)  
3. **Emergency USB** (if WiFi fails)
4. **Alert operator** if all methods fail

### Manual Connection Override:
```bash
# Force specific connection method
export SONY_CAM4_FORCE_CONNECTION="USB"    # Force A74 to USB
export SONY_CAM1_FORCE_CONNECTION="WIFI"   # Force FX6 to WiFi
```

## 📊 Connection Reliability Matrix

| Scenario | Ethernet | WiFi | USB | Recommended |
|----------|----------|------|-----|-------------|
| **Studio Production** | ✅ Primary | 🔄 Backup | 🆘 Emergency | Ethernet |
| **Location Shoot** | ⚠️ Limited | ✅ Primary | 🔄 Backup | WiFi + USB |
| **Mobile Setup** | ❌ None | ✅ Primary | ✅ Backup | WiFi + USB |
| **Emergency Backup** | ❌ Failed | ❌ Failed | ✅ Last Resort | USB Only |

## 🎬 Real-World Usage Examples

### Scenario 1: Studio Production (8 cameras)
```
CAM1-3 (FX6/FX3): Ethernet primary, WiFi backup
CAM4,6,8 (A74):   WiFi primary, USB backup
```

### Scenario 2: Location Shoot (4 cameras)  
```
CAM1-2 (FX6):     WiFi primary, USB backup
CAM3-4 (A74):     WiFi primary, USB backup  
```

### Scenario 3: Emergency Backup (all USB)
```
All cameras:      USB direct connection
Pi as USB hub:    Maximum control reliability
```

This system gives you **maximum flexibility** and **bulletproof redundancy** for any production scenario from 1-8 cameras! 🎯📹