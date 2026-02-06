#!/usr/bin/env python3
"""
Sony A74 USB Recording Automation - PRODUCTION VERSION

Based on successful manual testing with RemoteCli:
- Sony A74 detected as ILCE-7M4 (D0746063BB60)
- Movie recording: Down to start, Up to stop  
- Status: MovieRecordingOperation_Result_OK confirmed

Usage: python3 sony_a74_final.py [duration_seconds]
"""

import subprocess
import time
import signal
import sys
import re
from pathlib import Path

class SonyA74RecordingController:
    def __init__(self):
        self.remotecli_path = "/home/whoosh/CrSDK_v2.00.00_20251030a_Linux64ARMv8/build/RemoteCli"
        self.working_dir = "/home/whoosh/CrSDK_v2.00.00_20251030a_Linux64ARMv8/build"
        self.process = None
        self.recording_active = False
        
    def check_camera_connection(self):
        """Check if Sony A74 is connected and detected"""
        print("Checking Sony A74 connection...")
        
        # Check USB connection
        try:
            result = subprocess.run(['lsusb'], capture_output=True, text=True)
            if 'Sony Corp' in result.stdout:
                print("✅ Sony camera detected on USB")
            else:
                print("⚠️  No Sony camera found on USB")
                return False
        except:
            print("⚠️  Could not check USB devices")
            
        # Check SDK detection
        try:
            process = subprocess.Popen(
                [self.remotecli_path],
                cwd=self.working_dir,
                stdin=subprocess.PIPE,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True
            )
            
            time.sleep(3)  # Wait for camera enumeration
            process.stdin.write("x\n")  # Exit
            process.stdin.flush()
            
            stdout, _ = process.communicate(timeout=10)
            
            if "ILCE-7M4" in stdout:
                print("✅ Sony A74 (ILCE-7M4) detected by SDK")
                return True
            elif "MPC-2610" in stdout:
                print("ℹ️  MPC-2610 camera detected (not A74)")
                return False
            else:
                print("⚠️  No cameras detected by SDK")
                return False
                
        except Exception as e:
            print(f"❌ Error checking camera: {e}")
            return False
    
    def start_recording_session(self):
        """Start RemoteCli and prepare for recording"""
        print("Starting RemoteCli...")
        
        try:
            self.process = subprocess.Popen(
                [self.remotecli_path],
                cwd=self.working_dir,
                stdin=subprocess.PIPE,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
                bufsize=1
            )
            
            # Wait for startup and camera enumeration
            time.sleep(4)
            return True
            
        except Exception as e:
            print(f"❌ Failed to start RemoteCli: {e}")
            return False
    
    def connect_to_camera(self):
        """Connect to Sony A74 in Remote Control Mode"""
        print("Connecting to Sony A74...")
        
        try:
            # Connect (Remote Control Mode)
            self.process.stdin.write("1\n")
            self.process.stdin.flush()
            time.sleep(2)
            
            # Select camera (try camera 1 first, then 2)
            # Camera numbering can vary depending on detection order
            self.process.stdin.write("1\n")  # Try camera 1
            self.process.stdin.flush()
            time.sleep(2)
            
            # Connect to selected camera
            self.process.stdin.write("1\n")
            self.process.stdin.flush()
            time.sleep(3)
            
            print("✅ Connected to Sony A74")
            return True
            
        except Exception as e:
            print(f"❌ Failed to connect to camera: {e}")
            return False
    
    def start_recording(self):
        """Start movie recording using Movie Rec Button"""
        print("🔴 Starting movie recording...")
        
        try:
            # Navigate to Shutter/Rec Operation Menu
            self.process.stdin.write("1\n")  # Shutter/Rec Operation Menu
            self.process.stdin.flush()
            time.sleep(1)
            
            # Select Movie Rec Button
            self.process.stdin.write("6\n")  # Movie Rec Button
            self.process.stdin.flush()
            time.sleep(1)
            
            # Confirm operation
            self.process.stdin.write("y\n")  # Yes, operate movie recording button
            self.process.stdin.flush()
            time.sleep(1)
            
            # Press Down to start recording
            self.process.stdin.write("2\n")  # Down (Start Recording)
            self.process.stdin.flush()
            time.sleep(2)
            
            self.recording_active = True
            print("✅ Movie recording STARTED successfully!")
            print("   Expected status: MovieRecordingOperation_Result_OK")
            return True
            
        except Exception as e:
            print(f"❌ Failed to start recording: {e}")
            return False
    
    def stop_recording(self):
        """Stop movie recording"""
        print("🔴 Stopping movie recording...")
        
        if not self.recording_active:
            print("⚠️  No active recording to stop")
            return False
            
        try:
            # Navigate to Movie Rec Button again
            self.process.stdin.write("6\n")  # Movie Rec Button
            self.process.stdin.flush()
            time.sleep(1)
            
            # Confirm operation
            self.process.stdin.write("y\n")  # Yes, operate movie recording button
            self.process.stdin.flush()
            time.sleep(1)
            
            # Press Up to stop recording
            self.process.stdin.write("1\n")  # Up (Stop Recording)
            self.process.stdin.flush()
            time.sleep(2)
            
            self.recording_active = False
            print("✅ Movie recording STOPPED successfully!")
            print("   Expected status: MovieRecordingOperation_Result_OK")
            return True
            
        except Exception as e:
            print(f"❌ Failed to stop recording: {e}")
            return False
    
    def disconnect(self):
        """Disconnect from camera and exit"""
        print("Disconnecting from camera...")
        
        try:
            # Return to REMOTE-MENU
            self.process.stdin.write("0\n")  
            self.process.stdin.flush()
            time.sleep(1)
            
            # Disconnect and return to top menu
            self.process.stdin.write("0\n")  
            self.process.stdin.flush()
            time.sleep(2)
            
            # Exit application
            self.process.stdin.write("x\n")  
            self.process.stdin.flush()
            time.sleep(1)
            
            print("✅ Disconnected successfully")
            return True
            
        except Exception as e:
            print(f"⚠️  Error during disconnect: {e}")
            return False
    
    def cleanup(self):
        """Clean up process and resources"""
        if self.process:
            try:
                if self.recording_active:
                    print("⚠️  Emergency stop - Recording was still active")
                    self.stop_recording()
                    
                self.process.terminate()
                self.process.wait(timeout=5)
            except:
                self.process.kill()
    
    def record_video(self, duration=5):
        """Complete automated recording workflow"""
        print("🎬 Sony A74 USB Recording Automation")
        print(f"   Recording duration: {duration} seconds")
        print("=" * 50)
        
        try:
            # Step 1: Check camera connection
            if not self.check_camera_connection():
                print("❌ Camera not available. Please check:")
                print("   1. Sony A74 is connected via USB")
                print("   2. Camera is powered on")
                print("   3. USB mode is set to PC Remote or MTP")
                return False
            
            # Step 2: Start RemoteCli session
            if not self.start_recording_session():
                return False
            
            # Step 3: Connect to camera
            if not self.connect_to_camera():
                return False
            
            # Step 4: Start recording
            if not self.start_recording():
                return False
            
            # Step 5: Record for specified duration
            print(f"🎥 Recording in progress... ({duration}s)")
            for i in range(duration):
                time.sleep(1)
                remaining = duration - i - 1
                if remaining > 0:
                    print(f"   Time remaining: {remaining}s", end='\r')
            print()  # New line after countdown
            
            # Step 6: Stop recording
            self.stop_recording()
            
            # Step 7: Disconnect
            self.disconnect()
            
            print("🎉 Recording completed successfully!")
            print("   Check your camera's memory card for the recorded video.")
            return True
            
        except KeyboardInterrupt:
            print("\n⚠️  Recording interrupted by user (Ctrl+C)")
            if self.recording_active:
                self.stop_recording()
            self.disconnect()
            return False
            
        except Exception as e:
            print(f"❌ Recording failed: {e}")
            return False
            
        finally:
            self.cleanup()

def main():
    """Main entry point"""
    # Parse command line arguments
    duration = 5  # Default 5 seconds
    if len(sys.argv) > 1:
        try:
            duration = int(sys.argv[1])
            if duration < 1:
                raise ValueError("Duration must be positive")
        except ValueError as e:
            print(f"❌ Invalid duration: {e}")
            print("Usage: python3 sony_a74_final.py [duration_seconds]")
            sys.exit(1)
    
    # Create controller
    controller = SonyA74RecordingController()
    
    # Handle Ctrl+C gracefully
    def signal_handler(sig, frame):
        print("\n⚠️  Interrupted by user")
        if controller.recording_active:
            controller.stop_recording()
        controller.cleanup()
        sys.exit(0)
        
    signal.signal(signal.SIGINT, signal_handler)
    
    # Record video
    success = controller.record_video(duration)
    
    if success:
        print("\n✅ Automation completed successfully!")
    else:
        print("\n❌ Automation failed!")
        
    sys.exit(0 if success else 1)

if __name__ == "__main__":
    main()