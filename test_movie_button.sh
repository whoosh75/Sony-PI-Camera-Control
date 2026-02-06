#!/bin/bash

# Test Movie Rec Button directly (option 6)
# This should work regardless of Toggle support

REMOTE_CLI="/home/whoosh/CrSDK_v2.00.00_20251030a_Linux64ARMv8/build/RemoteCli"

echo "🎬 Testing Movie Rec Button (option 6)"
echo "====================================="

cd "$(dirname "$REMOTE_CLI")"

echo "📹 Pressing Movie Rec Button..."

# Simple sequence: 1 → 1 → y → admin → Password1 → 1 → 6 → y
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
    echo "6"         # Movie Rec Button
    sleep 1
    echo "y"         # Confirm "Operate the movie recording button"
    sleep 3          # Wait for recording action
    echo "0"         # Return to REMOTE-MENU
    sleep 1
    echo "0"         # Return to TOP-MENU
    sleep 1
    echo "x"         # Exit
) | ./RemoteCli

echo ""
echo "✅ Movie Rec Button command sent!"
echo "🎥 Check the camera screen to see if recording started"
echo ""
echo "💡 To stop recording, run this script again:"
echo "   $0"