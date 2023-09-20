#!/bin/bash

# Clear the kernel ring buffer to start fresh
sudo dmesg -C

# Run the test
./assignment-autotest/test/assignment8/drivertest.sh

# Wait a moment to ensure all logs are flushed
sleep 1

# Print out the recent kernel logs without filtering
dmesg

