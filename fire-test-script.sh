#!/bin/bash

sudo ./aesd-char-driver/aesdchar_load 
sudo chmod a+w /dev/aesdchar
./assignment-autotest/test/assignment8/drivertest.sh 
sudo ./aesd-char-driver/aesdchar_unload 
