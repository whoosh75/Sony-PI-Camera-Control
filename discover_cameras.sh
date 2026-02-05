#!/bin/bash

echo "🔍 Sony Camera Discovery Tool"
echo "==============================="

# Scan for devices on network
echo "📡 Scanning network for cameras..."
network=$(ip route | grep eth0 | grep -E "192\.168\.|10\.|172\." | head -1 | awk '{print $1}')

if [ -n "$network" ]; then
    echo "Scanning network: $network"
    nmap -sn $network 2>/dev/null | grep -E "Nmap scan report|MAC Address" | while read line; do
        if echo "$line" | grep -q "Nmap scan report"; then
            ip=$(echo "$line" | awk '{print $5}')
            echo "Found device: $ip"
        elif echo "$line" | grep -q "MAC Address"; then
            mac=$(echo "$line" | awk '{print $3}')
            vendor=$(echo "$line" | awk -F'(' '{print $2}' | tr -d ')')
            echo "  MAC: $mac ($vendor)"
            
            # Check if this might be a Sony camera
            if echo "$vendor" | grep -iq "sony"; then
                echo "  🎥 POTENTIAL SONY CAMERA DETECTED!"
                echo "  Suggested config:"
                echo "    export SONY_CAMERA_IP=\"$ip\""
                echo "    export SONY_CAMERA_MAC=\"$mac\""
                echo ""
            fi
        fi
    done
else
    echo "❌ Could not determine network range"
fi

echo ""
echo "📋 Manual camera configuration:"
echo "For each camera, you need:"
echo "  1. IP address (from camera menu or network scan above)"
echo "  2. MAC address (from 'ip neigh show <camera_ip>' after ping)"
echo "  3. Camera model (FX6=ILME_FX6, FX3=ILME_FX3, A74=ILCE_7M4)"
echo "  4. SSH password (usually 'Password1' or set in camera)"

echo ""
echo "🎯 8-Camera Production Setup:"
echo "=============================="
echo ""
echo "# Camera 1 - Main FX6"
echo "export SONY_CAM1_IP=\"192.168.33.91\""
echo "export SONY_CAM1_MODEL=\"ILME_FX6\""
echo "export SONY_CAM1_ROLE=\"main\""
echo ""
echo "# Camera 2 - B-Camera FX6" 
echo "export SONY_CAM2_IP=\"192.168.33.92\""
echo "export SONY_CAM2_MODEL=\"ILME_FX6\""
echo "export SONY_CAM2_ROLE=\"bcam\""
echo ""
echo "# Camera 3 - Wide FX3"
echo "export SONY_CAM3_IP=\"192.168.33.93\""
echo "export SONY_CAM3_MODEL=\"ILME_FX3\""
echo "export SONY_CAM3_ROLE=\"wide\""
echo ""
echo "# Camera 4 - Close-up A74"
echo "export SONY_CAM4_IP=\"192.168.33.94\""
echo "export SONY_CAM4_MODEL=\"ILCE_7M4\""
echo "export SONY_CAM4_ROLE=\"close\""
echo ""
echo "# Camera 5 - Stage FX3"
echo "export SONY_CAM5_IP=\"192.168.33.95\""
echo "export SONY_CAM5_MODEL=\"ILME_FX3\""
echo "export SONY_CAM5_ROLE=\"stage\""
echo ""
echo "# Camera 6 - Overhead A74"
echo "export SONY_CAM6_IP=\"192.168.33.96\""
echo "export SONY_CAM6_MODEL=\"ILCE_7M4\""
echo "export SONY_CAM6_ROLE=\"overhead\""
echo ""
echo "# Camera 7 - Side FX6"
echo "export SONY_CAM7_IP=\"192.168.33.97\""
echo "export SONY_CAM7_MODEL=\"ILME_FX6\""
echo "export SONY_CAM7_ROLE=\"side\""
echo ""
echo "# Camera 8 - Detail A74"
echo "export SONY_CAM8_IP=\"192.168.33.98\""
echo "export SONY_CAM8_MODEL=\"ILCE_7M4\""
echo "export SONY_CAM8_ROLE=\"detail\""
echo ""
echo "# Global settings for all cameras"
echo "export SONY_GLOBAL_PASS=\"Password1\""
echo "export SONY_ACCEPT_FINGERPRINT=\"1\""
echo ""
echo "🎬 Pro Tips for 8-Camera Setup:"
echo "• Use gigabit switch for network backbone"
echo "• Consider separate VLAN for camera traffic"  
echo "• Plan IP ranges: .91-.98 for cameras, .99+ for control"
echo "• Use consistent SSH passwords across all cameras"
echo "• Test individual cameras before multi-camera operations"