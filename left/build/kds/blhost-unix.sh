#!/bin/sh
set -e # fail the script if a command fails

PATH=$PATH:/usr/local/bin # This should make node and npm accessible on OSX.
firmware_image=`pwd`/$1
usb_dir=../../../lib/agent/packages/usb
usb_binding=$usb_dir/node_modules/usb/build/Release/usb_bindings.node

case "$(uname -s)" in
   Linux)
     blhost_path=linux/amd64
     ;;
   Darwin)
     blhost_path=mac
     ;;
   *)
     echo 'Your operating system is not supported.'
     exit 1
     ;;
esac

blhost="../../../lib/bootloader/bin/Tools/blhost/$blhost_path/blhost --usb 0x1d50,0x6121 --buspal i2c,0x10,100k"

set -x # echo on

#if [ ! -f $usb_binding ]; then
#   cd $usb_dir
#   npm install
#fi

$usb_dir/jump-to-slave-bootloader.js
$usb_dir/jump-to-bootloader.js buspal
sleep 2
$blhost get-property 1
$blhost flash-erase-all-unsecure
$blhost write-memory 0x0 "$firmware_image"
$blhost reset
sleep 4
$usb_dir/send-kboot-command.js reset 0x10
#../../../lib/bootloader/bin/Tools/blhost/$blhost_path/blhost --usb 0x1d50,0x6121 reset
