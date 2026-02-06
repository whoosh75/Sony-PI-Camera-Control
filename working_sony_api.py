#!/usr/bin/env python3
"""
Sony Camera API - Clean interface using proven working scripts
Fast performance, reliable operation
"""
import subprocess
import os
import time

class SonyCameraAPI:
    """Professional Sony Camera API using working shell backend"""
    
    def __init__(self):
        self.build_dir = "/home/whoosh/camera-control/pi_controller/build"
        self.working_script = os.path.join(self.build_dir, "working_rec.sh")
        
        if not os.path.exists(self.working_script):
            raise FileNotFoundError(f"Working script not found: {self.working_script}")
    
    def _execute_command(self, command):
        """Execute camera command via working script"""
        try:
            result = subprocess.run(
                [self.working_script, command], 
                cwd=self.build_dir,
                capture_output=True, 
                text=True, 
                timeout=30
            )
            
            if result.returncode == 0:
                return True, result.stdout.strip()
            else:
                return False, result.stderr.strip()
                
        except subprocess.TimeoutExpired:
            return False, "Command timeout"
        except Exception as e:
            return False, f"Execution error: {str(e)}"
    
    def start_recording(self):
        """Start video recording"""
        print("🎬 Starting video recording...")
        success, output = self._execute_command("start")
        
        if success:
            print("✅ Recording started successfully!")
            return True
        else:
            print(f"❌ Recording start failed: {output}")
            return False
    
    def stop_recording(self):
        """Stop video recording"""
        print("⏹️ Stopping video recording...")
        success, output = self._execute_command("stop")
        
        if success:
            print("✅ Recording stopped successfully!")
            return True
        else:
            print(f"❌ Recording stop failed: {output}")
            return False
    
    def toggle_recording(self):
        """Toggle recording state (start if stopped, stop if started)"""
        print("🔄 Toggling recording state...")
        success, output = self._execute_command("toggle")
        
        if success:
            print("✅ Recording toggled successfully!")
            return True
        else:
            print(f"❌ Recording toggle failed: {output}")
            return False
    
    def take_photo(self):
        """Take a still photograph"""
        print("📸 Taking photo...")
        success, output = self._execute_command("photo")
        
        if success:
            print("✅ Photo captured successfully!")
            return True
        else:
            print(f"❌ Photo capture failed: {output}")
            return False
    
    def record_for_duration(self, seconds):
        """Record video for specified duration"""
        print(f"🎥 Recording for {seconds} seconds...")
        
        if not self.start_recording():
            return False
        
        time.sleep(seconds)
        return self.stop_recording()

def main():
    """Demo the Sony Camera API"""
    print("🎯 Sony Camera API Demo")
    print("=====================")
    
    try:
        # Initialize API
        camera = SonyCameraAPI()
        
        # Test recording sequence
        print("\n📹 Testing recording sequence...")
        
        # Start recording
        if camera.start_recording():
            time.sleep(3)  # Record for 3 seconds
            camera.stop_recording()
        
        print("\n📸 Testing photo capture...")
        camera.take_photo()
        
        print("\n🎉 API demo completed!")
            
    except Exception as e:
        print(f"❌ API Error: {e}")

if __name__ == "__main__":
    main()