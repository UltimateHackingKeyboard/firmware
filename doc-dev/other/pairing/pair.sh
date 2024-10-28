#!/bin/bash +x

# I am somewhat confused about ble address endianity. If you feel something is wrong, please let me know...

# Pairing process:
# - Enter pairing mode
# - (Optional) delete some or all bonds. If bonds are deleted outside of pairing mode, the device may try to reconnect and be treated as HID device, which urrently results in loosing usb connection.
# - Retrieve pairing data for both devices
# - Set pairing data for both devices
# - Start pairing for both devices. (They will automatically exit pairing mode after success or timeout.)

function extractData() {
    cat | grep '^[0-9].*' | sed 's/^ *[0-9]* *//;s/0 *$//'
}

dongleAddress=`./cmd.sh dongle 0 9 | extractData`
donglesPairedKeyboardAddress=`./cmd.sh dongle 0 10 | extractData`

echo "dongleAddress: $dongleAddress"
echo "donglesPairedKeyboardAddress: $donglesPairedKeyboardAddress"

#enter pairing mode
./cmd.sh dongle 0x1c > /dev/null
./cmd.sh right 0x1c > /dev/null

#delete all bonds
./cmd.sh dongle 0x1a 0 0 0 0 0 0 > /dev/null
./cmd.sh right 0x1a 0 0 0 0 0 0 > /dev/null

echo "All bonds deleted"

#(sed strips status code and removes one trailing 0 byte)
# the payload is:
# 6 bytes address
# 16 bytes r key
# 16 bytes c key

rightKey=`./cmd.sh right 0x16 | extractData`
dongleKey=`./cmd.sh dongle 0x16 | extractData`

echo "rightKey: $rightKey"
echo "dongleKey: $dongleKey"

pairingStatus=`./cmd.sh dongle 0 11 | extractData`
echo "dongle pairing status: $pairingStatus"

# write the payloads to the other device, and inform it of the peerId
# left 0
# right 1
# dongle 2
./cmd.sh right 0x17 2 $dongleKey > /dev/null
./cmd.sh dongle 0x17 1 $rightKey > /dev/null

echo "keys exchanged!"

# tell them to pair
./cmd.sh right 0x18 > /dev/null
./cmd.sh dongle 0x19 > /dev/null

echo "Pairing started!"

sleep 1

pairingStatus=`./cmd.sh dongle 0 11 | extractData`
echo "dongle pairing status: $pairingStatus"

echo "Hopefully, right <--> dongle are paired now; Tap key to continue..."
read a

#enter pairing mode
./cmd.sh left 0x1c > /dev/null
./cmd.sh right 0x1c > /dev/null

# delete left bonds
./cmd.sh left 0x1a 0 0 0 0 0 0 > /dev/null

# get keys
leftKey=`./cmd.sh left 0x16 | extractData`
rightKey=`./cmd.sh right 0x16 | extractData`

echo "leftKey: $leftKey"
echo "rightKey: $rightKey"

# exchange keys
./cmd.sh left 0x17 1 $rightKey
./cmd.sh right 0x17 0 $leftKey

# pair
./cmd.sh left 0x18
./cmd.sh right 0x19

sleep 1

echo "Hopefully, right <--> left are paired now"

# check if the devices are paired
#we care about address only in the key
rightPairedInDongle=`./cmd.sh dongle 0x1b $rightKey | extractData`
donglePairedInRight=`./cmd.sh right 0x1b $dongleKey | extractData`
leftPairedInDongle=`./cmd.sh dongle 0x1b $leftKey | extractData`

echo -e "ring <-> dongle paired: \n $rightPairedInDongle \n $donglePairedInRight \ndongle to left paired: \n $leftPairedInDongle"

