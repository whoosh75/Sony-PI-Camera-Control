#!/bin/bash

# Sony A74 USB Recording Automation Script
# Based on successful manual RemoteCli testing

REMOTECLI_PATH="/home/whoosh/CrSDK_v2.00.00_20251030a_Linux64ARMv8/build/RemoteCli"
RECORD_DURATION=${1:-5}  # Default 5 seconds, can be overridden

echo "Sony A74 USB Recording Automation"
echo "Recording duration: ${RECORD_DURATION} seconds"
echo "======================================="

# Create command sequence for RemoteCli
COMMANDS="1
2
1
6
y
2
"

echo "Starting RemoteCli and connecting to Sony A74..."
echo "Commands to execute:"
echo "  1 - Connect (Remote Control Mode)"
echo "  2 - Select ILCE-7M4 (Sony A74)"
echo "  1 - Shutter/Rec Operation Menu" 
echo "  6 - Movie Rec Button"
echo "  y - Confirm operation"
echo "  2 - Down (Start Recording)"

# Start recording
echo "$COMMANDS" | timeout 10 "$REMOTECLI_PATH" &
REMOTECLI_PID=$!

echo "Recording started! Waiting ${RECORD_DURATION} seconds..."
sleep "$RECORD_DURATION"

# Stop recording commands
STOP_COMMANDS="6
y
1
0
0
x
"

echo "Stopping recording..."
echo "$STOP_COMMANDS" | "$REMOTECLI_PATH" &

echo "Recording completed!"
echo "Check camera for recorded video file."