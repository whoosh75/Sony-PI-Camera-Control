#!/bin/bash

# Fast movie recording using optimized RemoteCli automation
# Direct authentication flow for maximum speed

REMOTE_CLI="/home/whoosh/CrSDK_v2.00.00_20251030a_Linux64ARMv8/build/RemoteCli"

case "${1:-}" in
    "start"|"")
        echo "🎬 Starting recording INSTANTLY..."
        expect -c "
        set timeout 15
        spawn $REMOTE_CLI
        expect \"Connect to camera with input number\"
        send \"1\r\"
        expect \"(1) Connect (Remote Control Mode)\"
        send \"1\r\"
        expect \"Are you sure you want to continue connecting ? (y/n)\"
        send \"y\r\"
        expect \"Please input user name\"  
        send \"admin\r\"
        expect \"Please input password\"
        send \"Password1\r\"
        expect \"0. Shutter\"
        send \"7\r\"
        expect \"Movie Rec Button\"
        send \"\r\"
        sleep 0.2
        send \"q\r\"
        expect eof
        "
        echo "✅ Recording started!"
        ;;
        
    "stop")
        echo "⏹️ Stopping recording INSTANTLY..."
        expect -c "
        set timeout 15
        spawn $REMOTE_CLI
        expect \"Connect to camera with input number\"
        send \"1\r\"
        expect \"(1) Connect (Remote Control Mode)\"
        send \"1\r\"
        expect \"Are you sure you want to continue connecting ? (y/n)\"
        send \"y\r\"
        expect \"Please input user name\"  
        send \"admin\r\"
        expect \"Please input password\"
        send \"Password1\r\"
        expect \"0. Shutter\"
        send \"7\r\"
        expect \"Movie Rec Button\"
        send \"\r\"
        sleep 0.2
        send \"q\r\"
        expect eof
        "
        echo "✅ Recording stopped!"
        ;;
        
    "test")
        echo "🧪 Testing 3-second recording..."
        echo "Starting..."
        $0 start
        sleep 3
        echo "Stopping..."
        $0 stop
        echo "✅ Test complete!"
        ;;
        
    *)
        echo "Usage: $0 [start|stop|test]"
        echo "  start - Start recording instantly" 
        echo "  stop  - Stop recording instantly"
        echo "  test  - Record for 3 seconds"
        exit 1
        ;;
esac