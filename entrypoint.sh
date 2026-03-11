#!/bin/sh
set -e

# Start Nginx in the background
echo "Starting Nginx (HTTPS) on port 5173..."
nginx -g "daemon on;"

if [ $# -eq 0 ]; then
  echo "Starting MatrixServer Simulator (fallback)..."
  exec server_simulator
else
  echo "Starting command: $@"
  exec "$@"
fi
