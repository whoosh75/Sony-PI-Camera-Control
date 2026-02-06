#!/bin/bash

# WORKING Sony Camera Recording Script
# Uses the verified Movie Rec Button with proper mode setting

REMOTE_CLI="/home/whoosh/CrSDK_v2.00.00_20251030a_Linux64ARMv8/build/RemoteCli"

echo "🎬 Sony Camera Recording Control"
echo "================================"

record_start() {
    echo "📹 Setting Movie Mode and Starting recording..."
    cd "$(dirname "$REMOTE_CLI")"
    
    # Working sequence: Set movie mode first, then start recording
    cat > temp_record_start.txt << 'EOF'
1
1
y
Password1
2
16
1
0
1
6
y
2
0
0
x
EOF
    ./RemoteCli < temp_record_start.txt > /dev/null 2>&1
    rm -f temp_record_start.txt
}

record_stop() {
    echo "⏹️ Stopping recording..."
    cd "$(dirname "$REMOTE_CLI")"
    
    # Stop recording sequence
    cat > temp_record_stop.txt << 'EOF'
1
1
y
Password1
1
6
y
1
0
0
x
EOF
    ./RemoteCli < temp_record_stop.txt > /dev/null 2>&1
    rm -f temp_record_stop.txt
}

case "${1:-}" in
    "start")
        record_start
        echo "✅ Recording STARTED!"
        echo "📹 Check your camera screen to confirm recording"
        ;;
    "stop")
        record_stop
        echo "✅ Recording STOPPED!"
        echo "🎥 Check your camera screen to confirm stopped"
        ;;
    "test")
        echo "🧪 Testing 5-second recording..."
        record_start
        echo "⏳ Recording for 5 seconds..."
        sleep 5
        record_stop
        echo "🎉 Test complete! Check camera for recorded video."
        ;;
    *)
        echo "Usage: $0 [start|stop|test]"
        echo ""
        echo "Commands:"
        echo "  start - Start movie recording"
        echo "  stop  - Stop movie recording"
        echo "  test  - Record for 5 seconds"
        echo ""
        echo "Examples:"
        echo "  $0 start"
        echo "  $0 stop"
        echo "  $0 test"
        echo ""
        echo "🎯 This uses the WORKING Movie Rec Button sequence!"
        ;;
esac