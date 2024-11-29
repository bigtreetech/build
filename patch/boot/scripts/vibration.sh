#!/bin/bash

#================================
# enable `pad7_vibration` overlays
#================================

echo 1 > /sys/class/leds/pad7_vibration/brightness
sleep 0.04
echo 0 > /sys/class/leds/pad7_vibration/brightness
