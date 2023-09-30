#!/bin/bash
echo "/dev/aesdchar contents:"
cat /dev/aesdchar
echo "dmesg contents:"
sudo dmesg | grep aesdchar
echo "syslog contents:"
sudo grep -i "aesdchar" /var/log/syslog

