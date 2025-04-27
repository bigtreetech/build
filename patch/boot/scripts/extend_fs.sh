#!/bin/bash

sudo /usr/lib/armbian/armbian-resize-filesystem start

sudo sed -i '/^.\/extend_fs.sh/s/^/#/' /boot/scripts/btt_init.sh
