# 8-Camera Sony Production Setup

## Quick Start Guide

### 1. Initialize 8-Camera Environment
```bash
./setup_8cameras.sh
source /tmp/8camera_env.sh
```

### 2. Test Individual Cameras
```bash
test_camera 1    # Test main FX6
test_camera 4    # Test close-up A74
test_all_cameras # Test all 8 cameras
```

### 3. Camera Layout Template

```
📹 8-Camera Production Layout:

CAM1 (FX6)  - Main Camera      - 192.168.33.91
CAM2 (FX6)  - B-Camera         - 192.168.33.92  
CAM3 (FX3)  - Wide Shot        - 192.168.33.93
CAM4 (A74)  - Close-up         - 192.168.33.94
CAM5 (FX3)  - Stage Left       - 192.168.33.95
CAM6 (A74)  - Overhead         - 192.168.33.96
CAM7 (FX6)  - Side Angle       - 192.168.33.97
CAM8 (A74)  - Detail Shot      - 192.168.33.98
```

## Network Requirements

- **Gigabit switch** for 8-camera bandwidth
- **IP Range**: 192.168.33.91-98 reserved for cameras
- **Control Station**: 192.168.33.99+ (Pi, laptops, etc.)
- **Consistent passwords** across all cameras

## Camera Models Supported

| Model | CRSDK Identifier | Quantity | Roles |
|-------|------------------|----------|-------|
| Sony FX6 | `CrCameraDeviceModel_ILME_FX6` | 3x | Main, B-Cam, Side |
| Sony FX3 | `CrCameraDeviceModel_ILME_FX3` | 3x | Wide, Stage |
| Sony A74 | `CrCameraDeviceModel_ILCE_7M4` | 3x | Close, Overhead, Detail |

## Professional Operations

### Synchronized Recording
```bash
# Start recording on cameras 1-4 (main coverage)
sync_record 1 2 3 4

# Start recording on all 8 cameras
sync_record 1 2 3 4 5 6 7 8
```

### Batch Settings
```bash
# Set ISO 800 on all cameras
set_all_iso 800

# Set f/2.8 on all cameras  
set_all_aperture 2.8
```

## Troubleshooting 8-Camera Setup

- **Connection issues**: Test cameras individually first
- **Network congestion**: Use gigabit switch, avoid WiFi
- **Power management**: Ensure adequate power for 8 cameras
- **Storage**: Monitor camera storage across all units