#!/bin/bash

username="biqu"
gcodes_path="/home/${username}/printer_data/gcodes"

if [ ! -d "/boot/gcode" ]; then
    exit -1
fi

if [ ! -d ${gcodes_path} ]; then
    exit -1
fi

if ls /boot/gcode/*.gcode > /dev/null 2>&1; then
    sudo mv /boot/gcode/*.gcode ${gcodes_path} -f
fi

sync
