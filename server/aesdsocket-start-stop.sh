#!/bin/sh

# Function to handle the start
start() {
  echo "Starting aesdsocket in daemon mode..."
  ./usr/bin/aesdsocket -d
}

# Function to handle the stop
stop() {
  echo "Stopping aesdsocket..."
  pkill -TERM 'aesdsocket'
  # Perform any additional cleanup steps here
}

case "$1" in
  start)
    start
    ;;
  stop)
    stop
    ;;
  *)
    echo "Usage: $0 {start|stop}"
    exit 1
    ;;
esac

exit 0

