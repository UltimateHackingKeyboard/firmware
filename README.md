# Ultimate Hacking Keyboard firmware

This repository hosts the firmware of the [Ultimate Hacking Keyboard](https://ultimatehackingkeyboard.com/).

## Cloning the repository

Please make sure to clone this repo with:

`git clone --recursive git@github.com:UltimateHackingKeyboard/firmware.git`

This will download the dependent submodules which are required to build the firmware.

## Bootloader dependency

Before flashing the firmware of the right keyboard half, [the bootloader](https://github.com/UltimateHackingKeyboard/bootloader) must be flashed to the microcontroller, otherwise the firmware will not be started. The reason is that the bootloader executes first, then it jumps to the firmware.

## Importing the firmware

Install [Kinetis Design Studio](http://www.nxp.com/products/software-and-tools/run-time-software/kinetis-software-and-tools/ides-for-kinetis-mcus/kinetis-design-studio-integrated-development-environment-ide:KDS_IDE) (KDS) and import the project by invoking **File -> Import -> General -> Existing Projects** into Workspace, select the `right` or `left` directory depending on the firmware to be built, then click on the Finish button. At this point, you should be able to build the firmware in KDS.

## Building and flashing the firmware

In KDS, click on Run > Debug Configurations, then you have two choices:

1. You can select **C/C++ Application > uhk-right v7 release kboot** to flash the firmware via the bootloader.

2. You can select **GDB SEGGER J-Link Debugging > uhk-right v7 release jlink** to flash the firmware via a SEGGER J-Link USB debug probe. In this case, the target device (left or right keyboard half, or add-on module) must be powered, and it must be connected to the debug probe.

## Contributing

Want to contribute? Let us show you [how](/CONTRIBUTING.md).
