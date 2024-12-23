#!/bin/bash

#================================
# enable `pad7_vibration` overlays
#================================

export AUDIODRIVER=alsa
AUDIODEV=hw:1,0 play -q /boot/scripts/mp3/click.mp3 &

echo 1 > /sys/class/leds/pad7_vibration/brightness
sleep 0.04
echo 0 > /sys/class/leds/pad7_vibration/brightness
