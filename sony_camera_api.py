#!/usr/bin/env python3
"""
Sony Camera Direct API Controller
Provides clean direct API control for Sony cameras via RemoteCli automation
"""

import subprocess
import time
import threading
import queue
import os
from typing import Optional, Dict, List

class SonyCameraAPI:
    """Direct API control for Sony cameras via RemoteCli"""
    
    def __init__(self):
        self.sdk_path = "/home/whoosh/CrSDK_v2.00.00_20251030a_Linux64ARMv8/build"
        self.process: Optional[subprocess.Popen] = None
        self.command_queue = queue.Queue()
        self.connected_camera = None
        self.is_recording = False
        
    def initialize(self) -> bool:
        """Initialize the Sony SDK and detect cameras"""
        try:
            os.chdir(self.sdk_path)
            
            # Test basic connectivity
            result = subprocess.run(
                ["./RemoteCli"],
                input="x\n",
                capture_output=True,
                text=True,
                timeout=10
            )
            
            if "Remote SDK successfully initialized" in result.stdout:
                print("✅ Sony SDK initialized successfully")
                return True
            else:
                print("❌ SDK initialization failed")
                return False
                
        except Exception as e:
            print(f"❌ Initialization error: {e}")
            return False
    
    def get_cameras(self) -> List[Dict[str, str]]:
        """Get list of available cameras"""
        try:
            os.chdir(self.sdk_path)
            
            result = subprocess.run(
                ["./RemoteCli"],
                input="x\n",
                capture_output=True,
                text=True,
                timeout=10
            )
            
            cameras = []
            lines = result.stdout.split('\n')
            
            for line in lines:
                if '] ' in line and '(' in line and ')' in line:
                    # Parse camera info: [1] MPC-2610 (50:26:EF:B8:3F:2C)
                    parts = line.split('] ', 1)
                    if len(parts) == 2:
                        number = parts[0].replace('[', '').strip()
                        model_and_id = parts[1]
                        
                        # Extract model and ID
                        if '(' in model_and_id and ')' in model_and_id:
                            model = model_and_id.split('(')[0].strip()
                            camera_id = model_and_id.split('(')[1].split(')')[0]
                            
                            cameras.append({
                                'number': number,
                                'model': model,
                                'id': camera_id
                            })
            
            return cameras
            
        except Exception as e:
            print(f"❌ Error getting cameras: {e}")
            return []
    
    def connect_to_sony_a74(self) -> bool:
        """Connect specifically to Sony A74 (ILCE-7M4)"""
        cameras = self.get_cameras()
        
        sony_a74 = None
        for camera in cameras:
            if 'ILCE-7M4' in camera['model']:
                sony_a74 = camera
                break
        
        if not sony_a74:
            print("❌ Sony A74 (ILCE-7M4) not found")
            return False
        
        print(f"🔗 Connecting to Sony A74: {sony_a74['model']} ({sony_a74['id']})")
        
        try:
            os.chdir(self.sdk_path)
            
            # Connect to the Sony A74
            commands = [
                sony_a74['number'],  # Select camera number
                "1",                 # Remote Control Mode
                "y"                  # Accept fingerprint
            ]
            
            self.process = subprocess.Popen(
                ["./RemoteCli"],
                stdin=subprocess.PIPE,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True
            )
            
            # Send connection commands
            for cmd in commands:
                self.process.stdin.write(cmd + '\n')
                self.process.stdin.flush()
                time.sleep(0.5)
            
            self.connected_camera = sony_a74
            print("✅ Connected to Sony A74")
            return True
            
        except Exception as e:
            print(f"❌ Connection error: {e}")
            return False
    
    def start_recording(self) -> bool:
        """Start movie recording on Sony A74 using Movie Rec Button Down"""
        if not self.connected_camera or not self.process:
            print("❌ Not connected to camera")
            return False
        
        if self.is_recording:
            print("⚠️  Already recording")
            return True
        
        try:
            print("🎬 Starting recording...")
            
            # Navigate to Movie Rec Button and send Down command
            commands = [
                "1",  # Shutter/Rec Operation Menu
                "6",  # Movie Rec Button
                "y",  # Operate movie recording button? (yes)
                "2"   # Down (start recording)
            ]
            
            for cmd in commands:
                self.process.stdin.write(cmd + '\n')
                self.process.stdin.flush()
                time.sleep(0.3)
            
            self.is_recording = True
            print("✅ Recording started")
            return True
            
        except Exception as e:
            print(f"❌ Start recording error: {e}")
            return False
    
    def stop_recording(self) -> bool:
        """Stop movie recording on Sony A74 using Movie Rec Button Up"""
        if not self.connected_camera or not self.process:
            print("❌ Not connected to camera")
            return False
        
        if not self.is_recording:
            print("⚠️  Not currently recording")
            return True
        
        try:
            print("⏹️ Stopping recording...")
            
            # Send Movie Rec Button Up command
            commands = [
                "6",  # Movie Rec Button (already in Shutter/Rec menu)
                "y",  # Operate movie recording button? (yes)
                "1"   # Up (stop recording)
            ]
            
            for cmd in commands:
                self.process.stdin.write(cmd + '\n')
                self.process.stdin.flush()
                time.sleep(0.3)
            
            self.is_recording = False
            print("✅ Recording stopped")
            return True
            
        except Exception as e:
            print(f"❌ Stop recording error: {e}")
            return False
    
    def record_for_duration(self, duration_seconds: int) -> bool:
        """Record for a specific duration then stop"""
        if not self.start_recording():
            return False
        
        print(f"📹 Recording for {duration_seconds} seconds...")
        time.sleep(duration_seconds)
        
        return self.stop_recording()
    
    def disconnect(self):
        """Disconnect from camera and cleanup"""
        if self.process:
            try:
                # Send exit commands
                self.process.stdin.write("0\n")  # Return to REMOTE-MENU
                self.process.stdin.write("0\n")  # Disconnect
                self.process.stdin.write("x\n")  # Exit
                self.process.stdin.flush()
                
                # Wait for process to finish
                self.process.wait(timeout=5)
                
            except Exception:
                self.process.terminate()
            finally:
                self.process = None
                self.connected_camera = None
                self.is_recording = False
                print("🔌 Disconnected from camera")

def main():
    """Example usage of the Sony Camera API"""
    print("🎥 Sony Camera Direct API Test")
    print("==============================")
    
    # Create API instance
    camera_api = SonyCameraAPI()
    
    try:
        # Initialize
        if not camera_api.initialize():
            return 1
        
        # Get available cameras
        cameras = camera_api.get_cameras()
        print(f"📷 Found {len(cameras)} cameras:")
        for cam in cameras:
            print(f"  [{cam['number']}] {cam['model']} ({cam['id']})")
        
        # Connect to Sony A74
        if not camera_api.connect_to_sony_a74():
            return 1
        
        # Record for 5 seconds
        camera_api.record_for_duration(5)
        
        # Disconnect
        camera_api.disconnect()
        
        print("🎉 Direct API test completed successfully!")
        return 0
        
    except KeyboardInterrupt:
        print("\n⏹️ Test interrupted by user")
        camera_api.disconnect()
        return 0
    except Exception as e:
        print(f"❌ Test failed: {e}")
        camera_api.disconnect()
        return 1

if __name__ == "__main__":
    import sys
    sys.exit(main())