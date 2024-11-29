#!/bin/bash

cd /boot/scripts

#./extend_fs.sh &

./copy_gcode.sh &

./system_cfg.sh &

./connect_wifi.sh &

# regular sync to prevent data loss when direct power outage
./sync.sh &
