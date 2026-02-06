#!/bin/bash

# Working Sony Camera Recording Script
# Uses the verified Movie Rec Button (Toggle) with DOWN/UP sequence

REMOTE_CLI="/home/whoosh/CrSDK_v2.00.00_20251030a_Linux64ARMv8/build/RemoteCli"

case "${1:-}" in
    "start")
        echo "🎬 Starting recording..."
        expect -c "
        set timeout 20
        spawn $REMOTE_CLI
        expect \"input>\" { send \"1\r\" }
        expect \"input>\" { send \"1\r\" }
        expect \"(y/n)\" { send \"y\r\" }
        expect \"SSH password\" { send \"Password1\r\" }
        expect \"input>\" { send \"1\r\" }
        expect \"input>\" { send \"7\r\" }
        expect \"input>\" { send \"2\r\" }
        expect \"input>\" { send \"0\r\" }
        expect \"input>\" { send \"0\r\" }
        expect \"input>\" { send \"x\r\" }
        expect eof
        "
        echo "✅ Recording started!"
        ;;
        
    "stop")
        echo "⏹️ Stopping recording..."
        expect -c "
        set timeout 20
        spawn $REMOTE_CLI
        expect \"input>\" { send \"1\r\" }
        expect \"input>\" { send \"1\r\" }
        expect \"(y/n)\" { send \"y\r\" }
        expect \"SSH password\" { send \"Password1\r\" }
        expect \"input>\" { send \"1\r\" }
        expect \"input>\" { send \"7\r\" }
        expect \"input>\" { send \"3\r\" }
        expect \"input>\" { send \"0\r\" }
        expect \"input>\" { send \"0\r\" }
        expect \"input>\" { send \"x\r\" }
        expect eof
        "
        echo "✅ Recording stopped!"
        ;;
        
    "test")
        echo "🧪 Testing 3-second recording..."
        $0 start
        sleep 3
        $0 stop
        echo "🎉 Test complete!"
        ;;
        
    *)
        echo "Usage: $0 [start|stop|test]"
        echo "  start - Start recording (DOWN)"
        echo "  stop  - Stop recording (UP)" 
        echo "  test  - 3-second test"
        exit 1
        ;;
esac