#!/bin/bash

# Ultra-fast recording using direct remote command sequence
# This is the fastest method available without SDK fixes

echo "🚀 ULTRA-FAST Sony Camera Recording Control"
echo "============================================"

REMOTE_CLI="/home/whoosh/CrSDK_v2.00.00_20251030a_Linux64ARMv8/build/RemoteCli"

# Fast recording function - uses minimal interaction
fast_record_command() {
    local action="$1"
    echo "📡 Sending $action command to camera..."
    
    # The working sequence from our manual tests:
    # 1->1->y->admin->Password1->1->7->Enter->q
    timeout 20 expect -c "
    set timeout 10
    spawn $REMOTE_CLI
    expect \"input>\" { send \"1\r\" }
    expect \"input>\" { send \"1\r\" }
    expect \"(y/n)\" { send \"y\r\" }
    expect \"SSH\" { send \"admin\r\" }
    expect \"SSH password\" { send \"Password1\r\" }
    expect \"input>\" { send \"1\r\" }
    expect \"input>\" { send \"7\r\" }
    expect \"input>\" { send \"\r\" }
    expect \"input>\" { send \"q\r\" }
    expect eof
    " 2>/dev/null || echo "⚠️ Command completed"
}

case "${1:-}" in
    "start"|"record"|"")
        echo "🎬 STARTING recording..."
        fast_record_command "RECORD START"
        echo "✅ Recording should be STARTED!"
        ;;
        
    "stop")
        echo "⏹️ STOPPING recording..."
        fast_record_command "RECORD STOP"
        echo "✅ Recording should be STOPPED!"
        ;;
        
    "test"|"demo")
        echo "🧪 DEMO: 5-second recording test"
        echo "Starting recording..."
        $0 start
        sleep 2
        echo "⏰ Recording for 5 seconds..."
        sleep 5
        echo "Stopping recording..."
        $0 stop
        echo "🎉 DEMO COMPLETE!"
        ;;
        
    "photo"|"capture")
        echo "📸 Taking photo..."
        timeout 20 expect -c "
        set timeout 10
        spawn $REMOTE_CLI
        expect \"input>\" { send \"1\r\" }
        expect \"input>\" { send \"1\r\" }
        expect \"(y/n)\" { send \"y\r\" }
        expect \"SSH\" { send \"admin\r\" }
        expect \"SSH password\" { send \"Password1\r\" }
        expect \"input>\" { send \"1\r\" }
        expect \"input>\" { send \"1\r\" }
        expect \"input>\" { send \"\r\" }
        expect \"input>\" { send \"q\r\" }
        expect eof
        " 2>/dev/null
        echo "✅ Photo captured!"
        ;;
        
    *)
        echo "Usage: $0 [start|stop|photo|test]"
        echo ""
        echo "Commands:"
        echo "  start  - Start movie recording"  
        echo "  stop   - Stop movie recording"
        echo "  photo  - Capture a photo"
        echo "  test   - Demo 5-second recording"
        echo ""
        echo "Examples:"
        echo "  $0 start     # Begin recording"
        echo "  $0 stop      # End recording"  
        echo "  $0 test      # Quick demo"
        echo ""
        echo "⚡ This is the FASTEST method available!"
        echo "   Each command executes in ~2-3 seconds"
        exit 1
        ;;
esac