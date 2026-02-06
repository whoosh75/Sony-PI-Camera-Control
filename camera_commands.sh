#!/bin/bash

# Camera Control Quick Commands Script
# Easy interface for controlling Sony cameras

echo "🎬 Sony Camera Control Interface"
echo "================================"

# Set library path
export LD_LIBRARY_PATH="/home/whoosh/CrSDK_v2.00.00_20251030a_Linux64ARMv8/external/crsdk:/home/whoosh/CrSDK_v2.00.00_20251030a_Linux64ARMv8/external/crsdk/CrAdapter:/home/whoosh/CrSDK_v2.00.00_20251030a_Linux64ARMv8/external/opencv/Linux"

# Change to pi_controller directory
cd /home/whoosh/camera-control/pi_controller

# Check if camera test is compiled
if [ ! -f "./build/simple_camera_test" ]; then
    echo "⚠️  Camera control test not compiled."
    echo "   Building now..."
    cd build
    if ! make simple_camera_test; then
        echo "❌ Build failed. Please check compiler errors."
        exit 1
    fi
    cd ..
fi

# Quick command functions
test_connection() {
    echo "🔌 Testing camera connection..."
    echo -e "start\nquit" | ./build/simple_camera_test
}

start_recording() {
    echo "🔴 Starting recording..."
    echo -e "start\nquit" | ./build/simple_camera_test
}

stop_recording() {
    echo "⏹️  Stopping recording..."
    echo -e "stop\nquit" | ./build/simple_camera_test
}

capture_photo() {
    echo "📸 Capturing photo..."
    echo -e "photo\nquit" | ./build/simple_camera_test
}

set_iso() {
    local iso=$1
    if [ -z "$iso" ]; then
        echo "Usage: set_iso <value>"
        echo "Common values: 100, 200, 400, 800, 1600, 3200, 6400"
        return 1
    fi
    
    echo "🎚️  Setting ISO $iso..."
    echo -e "iso $iso\nquit" | ./build/simple_camera_test
}

show_status() {
    echo "📊 Camera Status..."
    echo -e "status\nquit" | ./build/simple_camera_test
}

interactive_mode() {
    echo "🎯 Starting interactive camera control..."
    ./build/simple_camera_test
}

# Interactive menu if no parameters
if [ $# -eq 0 ]; then
    echo ""
    echo "🎯 Quick Commands:"
    echo "  test               # Test camera connection"
    echo "  start              # Start recording"
    echo "  stop               # Stop recording"
    echo "  photo              # Capture photo"
    echo "  iso 800            # Set ISO value"
    echo "  status             # Show camera status"
    echo "  interactive        # Start interactive mode"
    echo ""
    echo "💡 Examples:"
    echo "  ./camera_commands.sh test"
    echo "  ./camera_commands.sh start"
    echo "  ./camera_commands.sh iso 1600"
    echo "  ./camera_commands.sh interactive"
    echo ""
    echo "📝 Note: Default camera IP is 192.168.1.110"
    exit 0
fi

# Execute command
case $1 in
    "test"|"connect")
        test_connection
        ;;
    "start"|"record")
        start_recording
        ;;
    "stop")
        stop_recording
        ;;
    "photo"|"capture")
        capture_photo
        ;;
    "iso")
        set_iso $2
        ;;
    "status")
        show_status
        ;;
    "interactive"|"i")
        interactive_mode
        ;;
    *)
        echo "❓ Unknown command: $1"
        echo "Available commands: test, start, stop, photo, iso, status, interactive"
        exit 1
        ;;
esac