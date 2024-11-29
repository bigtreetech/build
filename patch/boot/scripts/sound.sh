#!/bin/bash

export AUDIODRIVER=alsa
AUDIODEV=hw:1,0 play -q /boot/scripts/mp3/click.mp3 &
