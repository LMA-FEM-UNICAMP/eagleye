#!/bin/bash

RESTART_DELAY=2

source install/setup.bash       

while true; do
  ros2 launch navsatfix2nmea navsatfix2nmea.launch.py
  EXIT_CODE=$?

  echo "Launch exited with code $EXIT_CODE"

  echo "Restarting in $RESTART_DELAY seconds..."
  sleep $RESTART_DELAY
done