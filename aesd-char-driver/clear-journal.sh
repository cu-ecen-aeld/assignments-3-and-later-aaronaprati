#!/bin/bash

sudo rm -rf /var/log/journal/* /run/log/journal/* 
sudo systemctl restart systemd-journald

