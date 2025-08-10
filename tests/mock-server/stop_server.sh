#!/bin/bash

# Stop the mock server for Disaster Party tests
# Usage: ./stop_server.sh [port]

PORT=${1:-8080}
URL="http://localhost:$PORT"

echo "Stopping Disaster Party mock server at $URL..."

python3 control.py shutdown --url "$URL"

if [ $? -eq 0 ]; then
    echo "Server shutdown command sent successfully"
else
    echo "Failed to send shutdown command"
    echo "Server may not be running or may be unreachable"
fi