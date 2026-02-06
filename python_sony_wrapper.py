#!/usr/bin/env python3
"""
SOLUTION 1: Python wrapper using RemoteCli (WORKS but slower)
This provides immediate working solution while C++ SDK is being fixed.
"""
import subprocess
import time

class SonyAPIWrapper:
    """Simple wrapper around RemoteCli for immediate functionality"""
    
    def __init__(self):
        self.remotecli_path = "/home/whoosh/CrSDK_v2.00.00_20251030a_Linux64ARMv8/build/RemoteCli"
        self.current_device = None
        
    def list_cameras(self):
        """Enumerate available cameras"""
        print("📡 Enumerating Sony cameras...")
        
        result = subprocess.run([self.remotecli_path, "-l"], 
                              capture_output=True, text=True)
        
        if result.returncode == 0:
            print("✅ Camera enumeration successful")
            # Parse output to find cameras
            lines = result.stdout.strip().split('\n')
            cameras = []
            for line in lines:
                if "detected" in line and "[" in line:
                    print(f"🎥 {line}")
                    cameras.append(line)
            return cameras
        else:
            print(f"❌ Camera enumeration failed: {result.stderr}")
            return []
    
    def connect_to_device(self, device_num=2):
        """Connect to Sony device (2 = A74)"""
        print(f"🔗 Connecting to device {device_num}...")
        self.current_device = device_num
        return True
    
    def start_recording(self):
        """Start video recording using RemoteCli"""
        if not self.current_device:
            print("❌ No device connected")
            return False
            
        print("🎬 Starting recording via RemoteCli...")
        
        # RemoteCli command to start recording
        cmd = [self.remotecli_path, "-d", str(self.current_device), "-c", "MovieRecord", "Down"]
        
        result = subprocess.run(cmd, capture_output=True, text=True)
        
        if result.returncode == 0:
            print("✅ Recording started!")
            return True
        else:
            print(f"❌ Recording start failed: {result.stderr}")
            return False
    
    def stop_recording(self):
        """Stop video recording using RemoteCli"""
        if not self.current_device:
            print("❌ No device connected")
            return False
            
        print("⏹️ Stopping recording via RemoteCli...")
        
        # RemoteCli command to stop recording  
        cmd = [self.remotecli_path, "-d", str(self.current_device), "-c", "MovieRecord", "Up"]
        
        result = subprocess.run(cmd, capture_output=True, text=True)
        
        if result.returncode == 0:
            print("✅ Recording stopped!")
            return True
        else:
            print(f"❌ Recording stop failed: {result.stderr}")
            return False

def test_python_api():
    """Test the Python wrapper approach"""
    print("🐍 PYTHON WRAPPER APPROACH (RemoteCli)")
    print("=====================================")
    
    api = SonyAPIWrapper()
    
    # List cameras
    cameras = api.list_cameras()
    if not cameras:
        print("❌ No cameras found")
        return
    
    # Connect to Sony A74 (device 2)
    if not api.connect_to_device(2):
        print("❌ Failed to connect")
        return
    
    # Test recording
    if api.start_recording():
        print("📹 Recording for 5 seconds...")
        time.sleep(5)
        api.stop_recording()
    
    print("🎉 Python wrapper test completed!")

if __name__ == "__main__":
    test_python_api()