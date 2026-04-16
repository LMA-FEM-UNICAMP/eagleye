#!/bin/bash
# Usage: ./gps.sh <IP> <PORT>

IP="$1"
PORT="$2"

# check arguments
if [ -z "$IP" ] || [ -z "$PORT" ]; then
    echo "Usage: $0 <IP> <PORT>"
    exit 1
fi

sleep 2

gpsd tcp://"$IP":"$PORT" -N -n
