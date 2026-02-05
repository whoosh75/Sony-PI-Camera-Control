#!/bin/bash

# Flexible Multi-Camera Connection Setup
# Supports 1-8 cameras with Ethernet, WiFi, and USB fallback options

echo "🎬 Flexible Sony Camera Connection Setup"
echo "======================================="
echo "Supports: 1-8 cameras | Ethernet + WiFi + USB"

# Function to create camera configuration
create_camera_config() {
    local cam_num=$1
    local model=$2
    local name=$3
    local eth_ip=$4
    local wifi_ip=$5
    
    cat << EOF
# Camera ${cam_num} - ${name}
export SONY_CAM${cam_num}_NAME="${name}"
export SONY_CAM${cam_num}_MODEL="${model}"

# Primary: Ethernet (if available)
export SONY_CAM${cam_num}_ETH_IP="${eth_ip}"
export SONY_CAM${cam_num}_ETH_MAC=""  # Auto-detect

# Secondary: WiFi 
export SONY_CAM${cam_num}_WIFI_IP="${wifi_ip}"
export SONY_CAM${cam_num}_WIFI_MAC=""  # Auto-detect

# Backup: USB (especially for A74)
export SONY_CAM${cam_num}_USB_PATH="/dev/sony_camera_${cam_num}"
export SONY_CAM${cam_num}_USB_SERIAL=""  # Auto-detect

# Connection priority: Ethernet -> WiFi -> USB
export SONY_CAM${cam_num}_PRIORITY="ETHERNET,WIFI,USB"

EOF
}

# Generate flexible configurations for different setups
echo "Creating flexible camera configurations..."

cat << 'HEADER' > /tmp/flexible_camera_config.sh
#!/bin/bash
# Flexible Camera Configuration - Production Ready
# Automatically handles 1-8 cameras with multiple connection methods

# Global settings
export SONY_GLOBAL_PASS="Password1"
export SONY_ACCEPT_FINGERPRINT="1"
export SONY_CONNECTION_TIMEOUT="10"
export SONY_RETRY_ATTEMPTS="3"
export SONY_AUTO_FALLBACK="true"

# Network ranges
export ETHERNET_RANGE="192.168.33"    # .91-.98 for cameras
export WIFI_RANGE="10.32.56"          # .201-.208 for cameras  
export PI_ETHERNET_IP="192.168.33.99"
export PI_WIFI_IP="10.32.56.199"

HEADER

# Camera configurations (1-8)
{
    # Camera 1 - Main FX6
    create_camera_config 1 "ILME_FX6" "MAIN-FX6" "192.168.33.91" "10.32.56.201"
    
    # Camera 2 - B-Camera FX6  
    create_camera_config 2 "ILME_FX6" "BCAM-FX6" "192.168.33.92" "10.32.56.202"
    
    # Camera 3 - Wide FX3
    create_camera_config 3 "ILME_FX3" "WIDE-FX3" "192.168.33.93" "10.32.56.203"
    
    # Camera 4 - Close A74 (WiFi/USB primary - no ethernet)
    create_camera_config 4 "ILCE_7M4" "CLOSE-A74" "" "10.32.56.204"
    
    # Camera 5 - Stage FX3
    create_camera_config 5 "ILME_FX3" "STAGE-FX3" "192.168.33.95" "10.32.56.205"
    
    # Camera 6 - Overhead A74 (WiFi/USB primary - no ethernet)
    create_camera_config 6 "ILCE_7M4" "OVERHEAD-A74" "" "10.32.56.206"
    
    # Camera 7 - Side FX6
    create_camera_config 7 "ILME_FX6" "SIDE-FX6" "192.168.33.97" "10.32.56.207"
    
    # Camera 8 - Detail A74 (WiFi/USB primary - no ethernet)
    create_camera_config 8 "ILCE_7M4" "DETAIL-A74" "" "10.32.56.208"
    
} >> /tmp/flexible_camera_config.sh

# Add helper functions
cat << 'FUNCTIONS' >> /tmp/flexible_camera_config.sh

# Helper functions for connection management
test_connection() {
    local cam_num=$1
    local connection_type=${2:-"AUTO"}
    
    if [ -z "$cam_num" ] || [ "$cam_num" -lt 1 ] || [ "$cam_num" -gt 8 ]; then
        echo "Usage: test_connection <1-8> [ETHERNET|WIFI|USB|AUTO]"
        return 1
    fi
    
    eval "local name=\$SONY_CAM${cam_num}_NAME"
    echo "🎥 Testing Camera $cam_num ($name) - $connection_type"
    
    case $connection_type in
        "ETHERNET"|"ETH")
            test_ethernet_connection $cam_num
            ;;
        "WIFI"|"WIRELESS")
            test_wifi_connection $cam_num
            ;;
        "USB")
            test_usb_connection $cam_num
            ;;
        "AUTO"|*)
            test_all_connections $cam_num
            ;;
    esac
}

test_ethernet_connection() {
    local cam_num=$1
    eval "local eth_ip=\$SONY_CAM${cam_num}_ETH_IP"
    
    if [ -z "$eth_ip" ]; then
        echo "❌ Ethernet: No IP configured"
        return 1
    fi
    
    echo "📡 Testing Ethernet: $eth_ip"
    if ping -c 1 -W 2 $eth_ip &>/dev/null; then
        echo "✅ Ethernet: $eth_ip reachable"
        
        # Get MAC address
        local mac=$(ip neigh show $eth_ip 2>/dev/null | awk '{print $5}')
        if [ -n "$mac" ]; then
            echo "✅ MAC: $mac"
            eval "export SONY_CAM${cam_num}_ETH_MAC=$mac"
        fi
        return 0
    else
        echo "❌ Ethernet: $eth_ip not reachable"
        return 1
    fi
}

test_wifi_connection() {
    local cam_num=$1
    eval "local wifi_ip=\$SONY_CAM${cam_num}_WIFI_IP"
    
    echo "📶 Testing WiFi: $wifi_ip"
    if ping -c 1 -W 3 $wifi_ip &>/dev/null; then
        echo "✅ WiFi: $wifi_ip reachable"
        
        # Get MAC address
        local mac=$(ip neigh show $wifi_ip 2>/dev/null | awk '{print $5}')
        if [ -n "$mac" ]; then
            echo "✅ MAC: $mac"
            eval "export SONY_CAM${cam_num}_WIFI_MAC=$mac"
        fi
        return 0
    else
        echo "❌ WiFi: $wifi_ip not reachable"
        return 1
    fi
}

test_usb_connection() {
    local cam_num=$1
    eval "local usb_path=\$SONY_CAM${cam_num}_USB_PATH"
    
    echo "🔌 Testing USB connections..."
    
    # Scan for Sony USB devices
    local usb_devices=$(lsusb | grep -i sony | wc -l)
    echo "Found $usb_devices Sony USB device(s)"
    
    if [ $usb_devices -gt 0 ]; then
        echo "✅ USB: Sony devices detected"
        lsusb | grep -i sony | head -3
        return 0
    else
        echo "❌ USB: No Sony devices found"
        return 1
    fi
}

test_all_connections() {
    local cam_num=$1
    eval "local name=\$SONY_CAM${cam_num}_NAME"
    
    echo "🔄 Testing all connection methods for Camera $cam_num ($name)"
    local success=false
    
    # Try Ethernet first (if configured)
    eval "local eth_ip=\$SONY_CAM${cam_num}_ETH_IP"
    if [ -n "$eth_ip" ]; then
        if test_ethernet_connection $cam_num; then
            echo "🎯 Ethernet connection READY"
            success=true
        fi
    fi
    
    # Try WiFi
    if test_wifi_connection $cam_num; then
        echo "🎯 WiFi connection READY"  
        success=true
    fi
    
    # Try USB
    if test_usb_connection $cam_num; then
        echo "🎯 USB connection READY"
        success=true
    fi
    
    if [ "$success" = true ]; then
        echo "✅ Camera $cam_num has working connection(s)"
    else
        echo "❌ Camera $cam_num: No working connections found"
    fi
}

# Production functions
setup_production() {
    local num_cameras=${1:-8}
    echo "🎬 Setting up production with $num_cameras cameras"
    
    for i in $(seq 1 $num_cameras); do
        eval "local name=\$SONY_CAM${i}_NAME"
        if [ -n "$name" ]; then
            echo "Configuring Camera $i: $name"
            test_all_connections $i
        fi
    done
}

# Quick connection test for specific number of cameras
test_cameras() {
    local num_cameras=${1:-8}
    echo "🧪 Testing first $num_cameras cameras..."
    
    for i in $(seq 1 $num_cameras); do
        test_connection $i
        echo ""
    done
}

# Monitor all active cameras
monitor_all() {
    local num_cameras=${1:-8}
    echo "👀 Monitoring $num_cameras cameras..."
    
    while true; do
        clear
        echo "🎬 Camera Status Monitor - $(date)"
        echo "================================"
        
        for i in $(seq 1 $num_cameras); do
            eval "local name=\$SONY_CAM${i}_NAME"
            if [ -n "$name" ]; then
                printf "CAM%d %-12s: " $i "$name"
                test_all_connections $i >/dev/null 2>&1
                if [ $? -eq 0 ]; then
                    echo "✅ ONLINE"
                else
                    echo "❌ OFFLINE"
                fi
            fi
        done
        
        echo ""
        echo "Press Ctrl+C to stop monitoring"
        sleep 5
    done
}

FUNCTIONS

chmod +x /tmp/flexible_camera_config.sh

echo ""
echo "✅ Created flexible camera configuration"
echo "📄 Location: /tmp/flexible_camera_config.sh"
echo ""
echo "🎯 Supported Setups:"
echo "  • 1-8 cameras (scalable)"
echo "  • Ethernet + WiFi + USB connections"  
echo "  • Automatic fallback on connection failure"
echo "  • Sony FX6, FX3, A74 models"
echo ""
echo "🚀 Quick Start:"
echo "  source /tmp/flexible_camera_config.sh"
echo ""
echo "🔧 Connection Testing:"
echo "  test_connection 1           # Test camera 1 (auto)"
echo "  test_connection 4 USB       # Test camera 4 via USB"
echo "  test_cameras 4              # Test first 4 cameras"
echo "  setup_production 6          # Setup 6-camera production"
echo "  monitor_all 8               # Monitor all 8 cameras"
echo ""
echo "🎬 Production Examples:"
echo "  • 1-camera: Just camera 1 (main)"
echo "  • 3-camera: Cameras 1,2,3 (main, b-cam, wide)" 
echo "  • 6-camera: Cameras 1-6 (full coverage)"
echo "  • 8-camera: All cameras (maximum setup)"

# Copy to permanent location
cp /tmp/flexible_camera_config.sh ~/camera-control/
echo ""
echo "📋 Permanent config saved to: ~/camera-control/flexible_camera_config.sh"