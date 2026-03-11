#!/bin/sh
set -e

# Start Nginx in the background
echo "Starting Nginx (HTTPS) on port 5173..."
nginx -g "daemon on;"

# Start the C++ WebSocket server simulator in the foreground
echo "Starting MatrixServer Simulator..."
exec server_simulator "$@"
