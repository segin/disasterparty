#!/bin/bash

# Start the mock server for Disaster Party tests
# Usage: ./start_server.sh [port]

PORT=${1:-8080}

echo "Starting Disaster Party mock server on port $PORT..."
echo "Set DP_MOCK_SERVER=http://localhost:$PORT to use with tests"
echo "Press Ctrl+C to stop the server"
echo ""

python3 main.py --port $PORT