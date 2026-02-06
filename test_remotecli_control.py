#!/usr/bin/env python3
"""
Simple test to verify RemoteCli can be controlled programmatically
"""

import subprocess
import time

def test_remotecli_control():
    remotecli_path = "/home/whoosh/CrSDK_v2.00.00_20251030a_Linux64ARMv8/build/RemoteCli"
    working_dir = "/home/whoosh/CrSDK_v2.00.00_20251030a_Linux64ARMv8/build"
    
    print("Testing RemoteCli programmatic control...")
    
    try:
        # Start RemoteCli
        process = subprocess.Popen(
            [remotecli_path],
            cwd=working_dir,
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )
        
        # Give it time to start
        time.sleep(3)
        
        # Send exit command
        process.stdin.write("x\n")
        process.stdin.flush()
        
        # Wait for exit
        stdout, stderr = process.communicate(timeout=10)
        
        print("✅ RemoteCli control test successful")
        print(f"Exit code: {process.returncode}")
        
        # Look for expected output patterns
        if "RemoteSampleApp" in stdout:
            print("✅ RemoteCli started correctly")
        if "Release SDK resources" in stdout:
            print("✅ RemoteCli exited cleanly")
            
        return True
        
    except subprocess.TimeoutExpired:
        print("⚠️  RemoteCli didn't exit in time")
        process.kill()
        return False
        
    except Exception as e:
        print(f"❌ Error: {e}")
        return False

if __name__ == "__main__":
    test_remotecli_control()