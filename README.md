# Ultimate Hacking Keyboard firmware

This repository hosts the firmware of the [Ultimate Hacking Keyboard](https://ultimatehackingkeyboard.com/).

## Cloning the repository

Please make sure to clone this repo with:

`git clone --recursive git@github.com:UltimateHackingKeyboard/firmware.git`

This will download the dependent submodules, which are required to build the firmware.

## Importing the firmware

Install [Kinetis Design Studio](http://www.nxp.com/products/software-and-tools/run-time-software/kinetis-software-and-tools/ides-for-kinetis-mcus/kinetis-design-studio-integrated-development-environment-ide:KDS_IDE) (KDS), import the project by invoking *File -> Import -> General -> Existing Projects into Workspace*, select the *left* or *right* directory depending on the desired firmware, then click on the *Finish* button.

## Bootloader dependency

Before flashing the firmware of the *right keyboard half*, [the bootloader](https://github.com/UltimateHackingKeyboard/bootloader) must be flashed, otherwise the firmware will not be started. The reason is that the bootloader gets executed first, then it jumps to the firmware, which is offsetted.

## Building and flashing the firmware

*Please make sure to substitute vX with the actual version of your prototype (v6, v7, etc.)* below.

In KDS, click on *Run -> Run Configurations*, then select *C/C++ Application -> uhk-right vX release kboot* to flash the firmware via the bootloader.

Lastly, click on the *Debug* button to build and flash the firmware.

## Contributing

Want to contribute? Let us show you [how](/CONTRIBUTING.md).
