# Ultimate Hacking Keyboard firmware

[![Build Status](https://travis-ci.org/UltimateHackingKeyboard/firmware.svg?branch=master)](https://travis-ci.org/UltimateHackingKeyboard/firmware)

This repository hosts the firmware of the [Ultimate Hacking Keyboard](https://ultimatehackingkeyboard.com/).

## Cloning the repository

Please make sure to clone this repo with:

`git clone --recursive git@github.com:UltimateHackingKeyboard/firmware.git`

This will download the dependent submodules, which are required to build the firmware.

## Importing the firmware

Install [Kinetis Design Studio](http://www.nxp.com/products/software-and-tools/run-time-software/kinetis-software-and-tools/ides-for-kinetis-mcus/kinetis-design-studio-integrated-development-environment-ide:KDS_IDE) (KDS), import the project by invoking *File -> Import -> General -> Existing Projects into Workspace*, select the *left* or *right* directory depending on the desired firmware, then click on the *Finish* button.

## Building and flashing the firmware

For the left keyboard half, make sure to power it via the right keyboard half (which must be powered via USB). Also connect the left keyboard half to your SEGGER J-Link USB debug probe (which must also be connected via USB). Then in KDS, click on *Run -> Run Configurations*, select *GDB SEGGER J-Link Debugging -> uhk-left release jlink*, and click on the *Debug* button.

For the right keyboard half, flash [the bootloader](https://github.com/UltimateHackingKeyboard/bootloader) first. Then in KDS, click on *Run -> Run Configurations*, select *C/C++ Application -> uhk-right release blhost*, and click on the *Debug* button. Please note that this update method only works on Linux out of the box. On other operating systems, you have to execute the relevant commands of the [blhost-unix.sh](right/build/kds/blhost-unix.sh) script.

## Contributing

Want to contribute? Let us show you [how](/CONTRIBUTING.md).
