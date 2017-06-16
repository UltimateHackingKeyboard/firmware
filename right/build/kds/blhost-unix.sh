#!/bin/sh
set -e # fail the script if a command fails

PATH=$PATH:/usr/local/bin # This should make node and npm accessible on OSX.
firmware_image=`pwd`/$1
usb_dir=../../../lib/agent/usb
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

blhost="../../../lib/bootloader/bin/Tools/blhost/$blhost_path/blhost --usb 0x1d50,0x6120"

set -x # echo on

if [ ! -f $usb_binding ]; then
   cd $usb_dir
   npm install
fi

$usb_dir/jump-to-bootloader.js
$blhost flash-security-disable 0403020108070605
$blhost flash-erase-region 0xc000 475136
$blhost flash-image $firmware_image
$blhost reset
