#!/bin/bash

# SIMPLE working recording script based on verified manual commands
# Uses the exact sequence that worked before

REMOTE_CLI="/home/whoosh/CrSDK_v2.00.00_20251030a_Linux64ARMv8/build/RemoteCli"

echo "🎬 Simple Sony Camera Recording"
echo "=============================="

record_start() {
    echo "📹 Starting recording with Movie Rec Button..."
    cd "$(dirname "$REMOTE_CLI")"
    
    # Working sequence: Use option 6 (Movie Rec Button) instead of 7 (Toggle)
    (
        echo "1"         # Select camera 1
        sleep 2
        echo "1"         # Remote Control Mode
        sleep 2  
        echo "y"         # Accept fingerprint
        sleep 1
        echo "admin"     # Username
        sleep 1
        echo "Password1" # Password  
        sleep 3
        echo "1"         # Shutter/Rec Operation Menu
        sleep 1
        echo "6"         # Movie Rec Button (not Toggle)
        sleep 3          # Longer wait for recording to start
        echo "0"         # Return to REMOTE-MENU
        sleep 1
        echo "0"         # Return to TOP-MENU
        sleep 1
        echo "x"         # Exit
    ) | ./RemoteCli
}

record_stop() {
    echo "⏹️ Stopping recording with Movie Rec Button..."
    cd "$(dirname "$REMOTE_CLI")"
    
    # Same sequence - Movie Rec Button acts as toggle
    (
        echo "1"         # Select camera 1
        sleep 2
        echo "1"         # Remote Control Mode
        sleep 2
        echo "y"         # Accept fingerprint
        sleep 1
        echo "admin"     # Username
        sleep 1
        echo "Password1" # Password
        sleep 3
        echo "1"         # Shutter/Rec Operation Menu
        sleep 1
        echo "6"         # Movie Rec Button (same as start)
        sleep 3          # Wait for stop
        echo "0"         # Return to REMOTE-MENU
        sleep 1
        echo "0"         # Return to TOP-MENU
        sleep 1
        echo "x"         # Exit
    ) | ./RemoteCli
}

case "${1:-}" in
    "start")
        record_start
        echo "✅ Recording command sent!"
        ;;
    "stop") 
        record_stop
        echo "✅ Stop recording command sent!"
        ;;
    "test")
        echo "🧪 Testing 5-second recording..."
        record_start
        echo "Recording for 5 seconds..."
        sleep 5
        record_stop
        echo "🎉 Test complete!"
        ;;
    *)
        echo "Usage: $0 [start|stop|test]"
        echo "  start - Start recording"
        echo "  stop  - Stop recording"
        echo "  test  - Record for 5 seconds"
        ;;
esac