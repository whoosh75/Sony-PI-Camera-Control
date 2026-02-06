#!/usr/bin/env python3
"""
Sony A74 USB Recording Test Script
Automates RemoteCli to connect to ILCE-7M4 and test recording
"""

import subprocess
import time
import sys
import os

def main():
    print("🎬 Sony A74 USB Recording Test")
    print("===============================")
    
    # Change to the correct SDK directory
    os.chdir("/home/whoosh/CrSDK_v2.00.00_20251030a_Linux64ARMv8/build")
    
    # Commands to send to RemoteCli
    commands = [
        "2",     # Select camera 2 (ILCE-7M4)
        "1",     # Connect (Remote Control Mode)
        "y",     # Accept fingerprint
        "1",     # Shutter/Rec Operation Menu
        "6",     # Movie Rec Button
        "y",     # Operate movie recording button? (yes)
        "2",     # Down (start recording)
        "6",     # Movie Rec Button again
        "y",     # Operate movie recording button? (yes) 
        "1",     # Up (stop recording)
        "0",     # Return to REMOTE-MENU
        "0",     # Disconnect and return to top menu
        "x"      # Exit
    ]
    
    # Join commands with newlines
    input_data = "\n".join(commands) + "\n"
    
    print("📡 Starting RemoteCli and connecting to Sony A74...")
    print("🎥 Will start recording, wait 3 seconds, then stop...")
    
    try:
        # Start RemoteCli process
        process = subprocess.Popen(
            ["./RemoteCli"],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )
        
        # Send all commands at once
        stdout, stderr = process.communicate(input=input_data, timeout=30)
        
        # Print output
        print("\n📋 RemoteCli Output:")
        print("=" * 50)
        print(stdout)
        
        if stderr:
            print("\n⚠️  Errors:")
            print(stderr)
            
        print(f"\n✅ Process completed with exit code: {process.returncode}")
        
        # Look for success indicators in output
        if "MovieRecordingOperation_Result_OK" in stdout:
            print("🎉 SUCCESS: Recording commands executed with OK status!")
        elif "ILCE-7M4" in stdout:
            print("✅ Sony A74 detected successfully")
        else:
            print("❓ Check output above for results")
            
    except subprocess.TimeoutExpired:
        print("⏰ Timeout - terminating process")
        process.kill()
        return 1
    except Exception as e:
        print(f"❌ Error: {e}")
        return 1
    
    return 0

if __name__ == "__main__":
    sys.exit(main())