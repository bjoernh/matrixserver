#!/bin/bash
# Cube update script
# Logs output to /var/log/cube-update.log

LOG_FILE="/var/log/cube-update.log"

# Ensure log file exists and is writable
touch "$LOG_FILE" 2>/dev/null || true

{
    echo "--- Update started: $(date) ---"
    apt-get update
    apt-get upgrade -y
    echo "--- Update finished: $(date) ---"
} >> "$LOG_FILE" 2>&1
