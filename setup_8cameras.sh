#!/bin/bash

# 8-Camera Sony Setup Script
# Quick configuration for professional multi-camera productions

echo "🎬 Sony 8-Camera Production Setup"
echo "=================================="

# Network Configuration Check
echo "📡 Checking network setup..."
PI_IP=$(ip route get 8.8.8.8 | awk '{for(i=1;i<=NF;i++)if($i=="src")print $(i+1)}' | head -1)
echo "Raspberry Pi IP: $PI_IP"

# Suggested camera IP range
CAMERA_BASE="192.168.33"
echo "Suggested camera range: $CAMERA_BASE.91-98"

# Environment variables for 8 cameras
cat << 'EOF' > /tmp/8camera_env.sh
#!/bin/bash
# 8-Camera Environment Configuration

# Global settings
export SONY_ACCEPT_FINGERPRINT="1"
export SONY_GLOBAL_PASS="Password1"

# Camera 1 - Main Camera (FX6)
export SONY_CAM1_IP="192.168.33.91"
export SONY_CAM1_MODEL="ILME_FX6"
export SONY_CAM1_NAME="MAIN-FX6"
export SONY_CAM1_ROLE="main"

# Camera 2 - B-Camera (FX6) 
export SONY_CAM2_IP="192.168.33.92"
export SONY_CAM2_MODEL="ILME_FX6"
export SONY_CAM2_NAME="BCAM-FX6"
export SONY_CAM2_ROLE="bcam"

# Camera 3 - Wide Shot (FX3)
export SONY_CAM3_IP="192.168.33.93"
export SONY_CAM3_MODEL="ILME_FX3"
export SONY_CAM3_NAME="WIDE-FX3"
export SONY_CAM3_ROLE="wide"

# Camera 4 - Close-up (A74)
export SONY_CAM4_IP="192.168.33.94"
export SONY_CAM4_MODEL="ILCE_7M4"
export SONY_CAM4_NAME="CLOSE-A74"
export SONY_CAM4_ROLE="close"

# Camera 5 - Stage Left (FX3)
export SONY_CAM5_IP="192.168.33.95"
export SONY_CAM5_MODEL="ILME_FX3"
export SONY_CAM5_NAME="STAGE-FX3"
export SONY_CAM5_ROLE="stage"

# Camera 6 - Overhead (A74)
export SONY_CAM6_IP="192.168.33.96"
export SONY_CAM6_MODEL="ILCE_7M4"
export SONY_CAM6_NAME="OVERHEAD-A74"
export SONY_CAM6_ROLE="overhead"

# Camera 7 - Side Angle (FX6)
export SONY_CAM7_IP="192.168.33.97"
export SONY_CAM7_MODEL="ILME_FX6"
export SONY_CAM7_NAME="SIDE-FX6"
export SONY_CAM7_ROLE="side"

# Camera 8 - Detail Shot (A74)
export SONY_CAM8_IP="192.168.33.98"
export SONY_CAM8_MODEL="ILCE_7M4"
export SONY_CAM8_NAME="DETAIL-A74"
export SONY_CAM8_ROLE="detail"

# Helper functions
export_camera_vars() {
    for i in {1..8}; do
        eval "export SONY_CAMERA_IP=\$SONY_CAM${i}_IP"
        eval "export SONY_CAMERA_MODEL=\$SONY_CAM${i}_MODEL"
        echo "Camera $i: \$SONY_CAM${i}_IP (\$SONY_CAM${i}_MODEL)"
    done
}

test_camera() {
    local cam_num=$1
    if [ -z "$cam_num" ]; then
        echo "Usage: test_camera <1-8>"
        return 1
    fi
    
    eval "local ip=\$SONY_CAM${cam_num}_IP"
    eval "local model=\$SONY_CAM${cam_num}_MODEL"
    eval "local name=\$SONY_CAM${cam_num}_NAME"
    
    echo "🎥 Testing Camera $cam_num ($name) at $ip..."
    
    # Network test
    if ping -c 1 -W 2 $ip &>/dev/null; then
        echo "✅ Network: $ip reachable"
        
        # Get MAC address
        local mac=$(ip neigh show $ip 2>/dev/null | awk '{print $5}')
        if [ -n "$mac" ]; then
            echo "✅ MAC: $mac"
        else
            echo "⚠️  MAC: Not in ARP table (run: ping $ip)"
        fi
        
        # Test camera connection (if compiled)
        if [ -f "./test_direct_simple" ]; then
            export SONY_CAMERA_IP=$ip
            export SONY_CAMERA_MAC=$mac
            echo "🔌 Testing CRSDK connection..."
            ./test_direct_simple
        fi
    else
        echo "❌ Network: $ip not reachable"
    fi
    echo ""
}

# Test all cameras
test_all_cameras() {
    echo "🎬 Testing all 8 cameras..."
    for i in {1..8}; do
        test_camera $i
    done
}

EOF

chmod +x /tmp/8camera_env.sh

echo ""
echo "✅ Created 8-camera environment script"
echo "📄 Location: /tmp/8camera_env.sh"
echo ""
echo "🚀 Quick start:"
echo "  source /tmp/8camera_env.sh    # Load environment"
echo "  test_camera 1                 # Test individual camera"
echo "  test_all_cameras             # Test all 8 cameras"
echo ""
echo "📋 Your 8-camera setup:"
echo "  • 3x Sony FX6 (Main, B-Cam, Side)"
echo "  • 3x Sony FX3 (Wide, Stage)"  
echo "  • 3x Sony A74 (Close, Overhead, Detail)"
echo "  • IP Range: 192.168.33.91-98"
echo ""
echo "🔧 Copy environment to permanent location:"
echo "  cp /tmp/8camera_env.sh ~/camera-control/"
echo "  echo 'source ~/camera-control/8camera_env.sh' >> ~/.bashrc"