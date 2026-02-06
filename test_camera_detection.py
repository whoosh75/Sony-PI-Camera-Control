#!/usr/bin/env python3
"""
Test camera detection with RemoteCli
"""

import subprocess
import time

def test_camera_detection():
    remotecli_path = "/home/whoosh/CrSDK_v2.00.00_20251030a_Linux64ARMv8/build/RemoteCli"
    working_dir = "/home/whoosh/CrSDK_v2.00.00_20251030a_Linux64ARMv8/build"
    
    print("Testing camera detection with RemoteCli...")
    
    try:
        # Start RemoteCli
        process = subprocess.Popen(
            [remotecli_path],
            cwd=working_dir,
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True
        )
        
        # Give it time to enumerate cameras
        time.sleep(5)
        
        # Send exit command
        process.stdin.write("x\n")
        process.stdin.flush()
        
        # Get output
        stdout, _ = process.communicate(timeout=10)
        
        print("RemoteCli Output:")
        print("=" * 50)
        print(stdout)
        print("=" * 50)
        
        # Check for camera detection
        if "ILCE-7M4" in stdout:
            print("✅ Sony A74 detected!")
        else:
            print("⚠️  Sony A74 not detected")
            
        if "MPC-2610" in stdout:
            print("✅ MPC-2610 camera detected!")
        else:
            print("⚠️  MPC-2610 not detected")
            
        return True
        
    except Exception as e:
        print(f"❌ Error: {e}")
        return False

if __name__ == "__main__":
    test_camera_detection()