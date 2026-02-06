#!/usr/bin/env python3
"""
Sony A74 USB Recording Automation
Uses the working RemoteCli binary to automate movie recording
"""

import subprocess
import time
import signal
import sys
from pathlib import Path

class SonyA74Controller:
    def __init__(self):
        self.remotecli_path = "/home/whoosh/CrSDK_v2.00.00_20251030a_Linux64ARMv8/build/RemoteCli"
        self.working_dir = "/home/whoosh/CrSDK_v2.00.00_20251030a_Linux64ARMv8/build"
        self.process = None
        
    def start_remotecli(self):
        """Start RemoteCli process"""
        print("Starting RemoteCli...")
        self.process = subprocess.Popen(
            [self.remotecli_path],
            cwd=self.working_dir,
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            bufsize=1
        )
        
        # Wait for initial startup
        time.sleep(2)
        return True
        
    def send_command(self, command, wait_time=1):
        """Send a command to RemoteCli"""
        if self.process:
            print(f"Sending: {command}")
            self.process.stdin.write(f"{command}\n")
            self.process.stdin.flush()
            time.sleep(wait_time)
            
    def connect_to_a74(self):
        """Connect to Sony A74 (ILCE-7M4)"""
        print("Connecting to Sony A74...")
        
        # Connect (Remote Control Mode)
        self.send_command("1", 2)
        
        # Select camera 2 (ILCE-7M4) 
        self.send_command("2", 2)
        
        # Connect to selected camera
        self.send_command("1", 3)
        
        print("Connected to Sony A74")
        return True
        
    def start_recording(self):
        """Start movie recording"""
        print("Starting movie recording...")
        
        # Shutter/Rec Operation Menu
        self.send_command("1", 1)
        
        # Movie Rec Button
        self.send_command("6", 1)
        
        # Confirm operation
        self.send_command("y", 1)
        
        # Down (Start Recording)
        self.send_command("2", 2)
        
        print("✅ Movie recording STARTED")
        return True
        
    def stop_recording(self):
        """Stop movie recording"""
        print("Stopping movie recording...")
        
        # Movie Rec Button
        self.send_command("6", 1)
        
        # Confirm operation
        self.send_command("y", 1)
        
        # Up (Stop Recording)
        self.send_command("1", 2)
        
        print("✅ Movie recording STOPPED")
        return True
        
    def disconnect(self):
        """Disconnect from camera"""
        print("Disconnecting...")
        
        # Return to REMOTE-MENU 
        self.send_command("0", 1)
        
        # Disconnect and return to top menu
        self.send_command("0", 2)
        
        # Exit
        self.send_command("x", 1)
        
        print("Disconnected")
        
    def cleanup(self):
        """Clean up process"""
        if self.process:
            try:
                self.process.terminate()
                self.process.wait(timeout=5)
            except:
                self.process.kill()
                
    def record_video(self, duration=5):
        """Complete recording workflow"""
        print(f"Sony A74 Recording for {duration} seconds")
        print("=" * 40)
        
        try:
            # Start RemoteCli
            if not self.start_remotecli():
                return False
                
            # Connect to A74
            if not self.connect_to_a74():
                return False
                
            # Start recording
            if not self.start_recording():
                return False
                
            # Record for specified duration
            print(f"Recording for {duration} seconds...")
            time.sleep(duration)
            
            # Stop recording
            self.stop_recording()
            
            # Disconnect
            self.disconnect()
            
            print("✅ Recording completed successfully!")
            return True
            
        except KeyboardInterrupt:
            print("\n⚠️  Recording interrupted by user")
            self.stop_recording()
            self.disconnect()
            return False
            
        except Exception as e:
            print(f"❌ Error during recording: {e}")
            return False
            
        finally:
            self.cleanup()

def main():
    # Parse command line arguments
    duration = 5  # Default 5 seconds
    if len(sys.argv) > 1:
        try:
            duration = int(sys.argv[1])
        except ValueError:
            print("Invalid duration. Using default 5 seconds.")
    
    # Create controller and record
    controller = SonyA74Controller()
    
    # Handle Ctrl+C gracefully
    def signal_handler(sig, frame):
        print("\n⚠️  Interrupted by user")
        controller.stop_recording()
        controller.cleanup()
        sys.exit(0)
        
    signal.signal(signal.SIGINT, signal_handler)
    
    # Record video
    success = controller.record_video(duration)
    sys.exit(0 if success else 1)

if __name__ == "__main__":
    main()