# Ultimate Hacking Keyboard firmware

[![Build Status](https://travis-ci.org/UltimateHackingKeyboard/firmware.svg?branch=master)](https://travis-ci.org/UltimateHackingKeyboard/firmware)

This repository hosts the firmware of the [Ultimate Hacking Keyboard](https://ultimatehackingkeyboard.com/).

## Updating to the latest firmware

Want to update your UHK to the latest firmware version? Simply download the [latest release of Agent](https://github.com/UltimateHackingKeyboard/agent/releases/latest) which includes the latest firmware version. You'll be easily able to update the firmware within Agent.

## Cloning the repository

Please make sure to clone this repo with:

`git clone --recursive git@github.com:UltimateHackingKeyboard/firmware.git`

This will download the dependent submodules, which are required to build the firmware.

## Importing the firmware

Install [Kinetis Design Studio](http://www.nxp.com/products/software-and-tools/run-time-software/kinetis-software-and-tools/ides-for-kinetis-mcus/kinetis-design-studio-integrated-development-environment-ide:KDS_IDE) (KDS), import the project by invoking *File -> Import -> General -> Existing Projects into Workspace*, select the *left* or *right* directory depending on the desired firmware, then click on the *Finish* button.

## Building and flashing the firmware

For the left keyboard half, make sure to power it via the right keyboard half (which must be powered via USB). Also connect the left keyboard half to your SEGGER J-Link USB debug probe (which must also be connected via USB). Then in KDS, click on *Run -> Run Configurations*, select *GDB SEGGER J-Link Debugging -> uhk60-left_release_jlink*, and click on the *Debug* button.

For the right keyboard half, flash [the bootloader](https://github.com/UltimateHackingKeyboard/bootloader) first.

At this point, you can flash the right firmware via USB from KDS. To achieve this, you must build [Agent](https://github.com/UltimateHackingKeyboard/agent) that is Git submodule of the this repo and located in the `lib/agent` directory.  Then in KDS, click on *Run -> Run Configurations*, select *C/C++ Application -> uhk60-right_release_kboot*, and click on the *Run* button.

From this point on, you can upgrade the firmwares of both halves via USB by using the uhk60-left_release_kboot and uhk60-right_release_kboot run configurations. Alternatively, you can use your SEGGER J-Link probe.

## Contributing

Want to contribute? Let us show you [how](/CONTRIBUTING.md).
