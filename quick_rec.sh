#!/bin/bash

# Quick Sony Camera Recording Commands
# Fixed version using working input file method

REMOTE_CLI="/home/whoosh/CrSDK_v2.00.00_20251030a_Linux64ARMv8/build/RemoteCli"

echo "🎬 Quick Camera Recording"

quick_record() {
    local action="$1"
    echo "📹 ${action}..."
    cd "$(dirname "$REMOTE_CLI")"
    
    if [ "$action" = "Starting recording" ]; then
        # Set movie mode and start recording
        cat > temp_quick.txt << 'EOF'
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
    else
        # Stop recording
        cat > temp_quick.txt << 'EOF'
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
    fi
    
    ./RemoteCli < temp_quick.txt > /dev/null 2>&1
    rm -f temp_quick.txt
    echo "✅ Done!"
}

case "${1:-}" in
    "start"|"s")
        quick_record "Starting recording"
        ;;
    "stop"|"x") 
        quick_record "Stopping recording"
        ;;
    *)
        echo "Usage: $0 [start|stop|s|x]"
        echo "  start, s - Start recording"
        echo "  stop, x  - Stop recording"
        ;;
esac