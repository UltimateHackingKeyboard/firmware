#!/bin/sh
set -x # echo on

jump-to-bootloader.js
sleep 2 &&
blhost --usb 0x15a2,0x0073 flash-erase-all 0 &&
blhost --usb 0x15a2,0x0073 flash-image $1 &&
blhost --usb 0x15a2,0x0073 reset
