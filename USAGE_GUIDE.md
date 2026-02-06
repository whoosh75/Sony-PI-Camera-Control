# USAGE GUIDE - Sony Camera API

## Quick Start

### 1. Basic Recording
```bash
cd /home/whoosh/camera-control/pi_controller/build
python3 working_sony_api.py
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
2. Verify storage space on camera
3. Check recording settings on camera

### API Import Errors
1. Ensure you're in the correct directory: `pi_controller/build`
2. Check file exists: `ls working_sony_api.py`
3. Verify Python 3 is installed: `python3 --version`