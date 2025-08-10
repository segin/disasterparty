#!/bin/bash

# Start the mock server for Disaster Party tests
# Usage: ./start_server.sh [port] [--foreground]

PORT=${1:-8080}
FOREGROUND_FLAG=""

# Check if --foreground is passed as any argument
for arg in "$@"; do
    if [ "$arg" = "--foreground" ]; then
        FOREGROUND_FLAG="--foreground"
        break
    fi
done

echo "Starting Disaster Party mock server on port $PORT..."
echo "Set DP_MOCK_SERVER=http://localhost:$PORT to use with tests"

if [ -n "$FOREGROUND_FLAG" ]; then
    echo "Running in foreground mode - Press Ctrl+C to stop the server"
    echo ""
    python3 main.py --port $PORT --foreground
else
    echo "Running in daemon mode - Use './control.py shutdown' to stop"
    echo "Use './control.py status' to check server status"
    echo "Use './control.py restart' to restart the server"
    echo ""
    python3 main.py --port $PORT
fi