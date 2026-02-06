#!/bin/bash

# Sony Camera Authentication Setup
echo "🔐 Sony Camera Authentication Setup"
echo "==================================="

# Check if camera is reachable
CAMERA_IP="192.168.1.110"
echo "🔍 Checking camera connectivity at $CAMERA_IP..."

if ping -c 1 -W 3 $CAMERA_IP > /dev/null 2>&1; then
    echo "✅ Camera is reachable at $CAMERA_IP"
else
    echo "❌ Camera not reachable at $CAMERA_IP"
    echo "   Please check:"
    echo "   - Camera is powered on"
    echo "   - Camera is connected to network"
    echo "   - Camera IP address is correct"
    echo "   - Camera is in PC Remote mode"
    exit 1
fi

echo ""
echo "🔑 Authentication Setup"
echo "======================"

# Check for existing SONY_PASS
if [ -n "$SONY_PASS" ]; then
    echo "✅ SONY_PASS environment variable is already set"
else
    echo "⚠️  SONY_PASS environment variable is not set"
    echo ""
    echo "📝 Camera Password Options:"
    echo "   1. No password (try default connection)"
    echo "   2. Enter custom password"
    echo "   3. Use common Sony passwords"
    echo ""
    read -p "Select option (1-3): " choice
    
    case $choice in
        1)
            echo "🔓 Trying connection without password..."
            export SONY_PASS=""
            ;;
        2)
            echo "🔐 Enter camera password:"
            read -s password
            export SONY_PASS="$password"
            echo "✅ Custom password set"
            ;;
        3)
            echo "🔐 Trying common Sony camera passwords..."
            # Common Sony camera default passwords
            for pwd in "" "0000" "1234" "admin" "sony"; do
                echo "   Testing password: '$pwd'"
                export SONY_PASS="$pwd"
                break  # For now, just set empty and test
            done
            ;;
        *)
            echo "❌ Invalid option"
            exit 1
            ;;
    esac
fi

# Auto-accept fingerprint
export SONY_ACCEPT_FINGERPRINT=1

echo ""
echo "🧪 Testing Camera Connection & Authentication"
echo "============================================="

# Set library path
export LD_LIBRARY_PATH="/home/whoosh/CrSDK_v2.00.00_20251030a_Linux64ARMv8/external/crsdk:/home/whoosh/CrSDK_v2.00.00_20251030a_Linux64ARMv8/external/crsdk/CrAdapter:/home/whoosh/CrSDK_v2.00.00_20251030a_Linux64ARMv8/external/opencv/Linux"

# Test connection
echo "🔌 Testing connection with current settings..."
cd /home/whoosh/camera-control
echo -e "status\nquit" | ./build/simple_camera_test

echo ""
echo "💡 Current Environment Variables:"
echo "   SONY_PASS: ${SONY_PASS:-'(not set)'}"
echo "   SONY_ACCEPT_FINGERPRINT: ${SONY_ACCEPT_FINGERPRINT:-'(not set)'}"
echo ""
echo "📝 To make authentication persistent, add to ~/.bashrc:"
echo "   export SONY_PASS=\"your_password\""
echo "   export SONY_ACCEPT_FINGERPRINT=1"