#!/bin/bash

file=send-command.ts
pwd=`pwd`


function determineUsbDeviceArg() {
    DEVICE=$1
    DEVICEUSBID=""

    case $DEVICE in
        left|uhk-80-left)
            DEVICEUSBID="--vid=0x37a8 --pid=7 --usb-interface=2"
            ;;
        right|uhk-80-right)
            DEVICEUSBID="--vid=0x37a8 --pid=9 --usb-interface=2"
            ;;
        dongle|uhk-dongle)
            DEVICEUSBID="--vid=0x37a8 --pid=5 --usb-interface=2"
            ;;
        uhk-60)
            ;;
    esac

    echo $DEVICEUSBID
}

ADDR=$(determineUsbDeviceArg $1)
shift

#ARGS=--log=usb

echo "executing" $pwd/$file $ADDR $ARGS  $@
$pwd/$file $ADDR $ARGS $@

