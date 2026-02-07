# Sony Camera API Documentation

## ⚠️ Important: This is CLI Automation, Not True API

**Performance Warning**: This solution uses menu automation through RemoteCli, causing 2-3 second delays per command. This is **NOT suitable for production use** that requires fast camera control.

## What This Actually Does

```python
from working_sony_api import SonyCameraAPI
camera = SonyCameraAPI()
camera.start_recording()  # Internally: subprocess -> shell script -> RemoteCli -> menu navigation
```

**Real execution flow**:
1. Python calls shell script via `subprocess.run()`
2. Shell script feeds input to RemoteCli 
3. RemoteCli navigates camera menus step-by-step
4. Multiple round trips cause 2-3 second latency

## Performance Reality

### CLI Automation (Current Implementation)
- **Latency**: 2-3 seconds per command
- **Method**: Menu navigation automation
- **Suitability**: ❌ Too slow for production use

### True API (Blocked - What You Need)
- **Latency**: Sub-second response  
- **Method**: Direct `SCRSDK::SendCommand()` calls
- **Status**: ❌ **BLOCKED - SDK initialization fails**

## What's Actually Working vs What's Needed

### ✅ CLI Automation Methods (Slow)
```python
camera.start_recording()           # 2-3 sec delay via menu automation
camera.stop_recording()            # 2-3 sec delay via menu automation  
camera.toggle_recording()          # 2-3 sec delay via menu automation
camera.take_photo()                # 2-3 sec delay via menu automation
```

### ❌ True API (Blocked - What You Actually Need)
```cpp
// This is what should work but doesn't:
SCRSDK::SendCommand(device_handle, SCRSDK::CrCommandId_MovieRecord, SCRSDK::CrCommandParam_Down);
// Would be instant, but SCRSDK::Init() fails with error 1
```

## Technical Blocker

All attempts to use the Sony CRSDK directly fail:

```bash
$ ./working_sdk_test
Testing Sony SDK with OpenCV wrapper...
Initialize Remote SDK...
Failed to initialize SDK with error: 1
```

**Root Cause**: Unknown SDK initialization requirement preventing direct API access

## Use Cases

### ✅ Acceptable Use (Testing Only)
- Manual testing and development
- Proof of concept demonstrations  
- Non-time-critical recording

### ❌ Unsuitable Use (Production)
- Real-time camera control
- Fast switching between record/stop
- Any application requiring sub-second response times
- Professional video production

## Supported Cameras
- Sony MPC-2610 (WiFi connection)
- Sony A74 / ILCE-7M4 (USB connection)

## File Locations
- **Main API**: `pi_controller/build/working_sony_api.py`
- **Backend Script**: `pi_controller/build/working_rec.sh` 
- **RemoteCli Executable**: `/home/whoosh/camera-control/RemoteCli`

## Example Integration

```python
#!/usr/bin/env python3
import time
from working_sony_api import SonyCameraAPI

def record_sequence():
    """Example recording sequence"""
    camera = SonyCameraAPI()
    
    # Record multiple clips
    for i in range(3):
        print(f"Recording clip {i+1}/3...")
        camera.record_for_duration(5)
        time.sleep(2)  # Pause between clips
    
    # Take some photos
    print("Taking photos...")
    for i in range(3):
        camera.take_photo()
        time.sleep(1)
    
    print("Recording sequence completed!")

if __name__ == "__main__":
    record_sequence()
```

## Error Handling
All methods return boolean success/failure status. Check return values to handle errors:

```python
camera = SonyCameraAPI()

if camera.start_recording():
    print("Recording started successfully")
    time.sleep(10)
    
    if camera.stop_recording():
        print("Recording stopped successfully")
    else:
        print("Failed to stop recording")
else:
    print("Failed to start recording")
```

The API provides detailed console output showing the exact status of each operation.