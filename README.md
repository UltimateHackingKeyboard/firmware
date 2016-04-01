# Ultimate Hacking Keyboard firmware

This repository hosts the firmware of the [Ultimate Hacking Keyboard](https://ultimatehackingkeyboard.com/).

The firmware of the right keyboard half currently runs on an FRDM-K22F dev board and implements a composite USB device.

The firmware of the left keyboard half currently runs on an FRDM-KL03Z dev board and communicates with the right half via I2C.

Here's a short [demonstration video](https://www.youtube.com/watch?v=MrMaoW_NA_U) for your viewing pleasure.

## Build

Please make sure to clone this repo with:

`git clone --recursive git@github.com:UltimateHackingKeyboard/firmware.git`

You're encouraged to install the [Kinetis Design Studio (KDS) IDE](http://www.nxp.com/products/software-and-tools/run-time-software/kinetis-software-and-tools/ides-for-kinetis-mcus/kinetis-design-studio-integrated-development-environment-ide:KDS_IDE) and import the project by invoking File -> Import -> General -> Existing Projects into Workspace, select the `right` directory, and click on the Finish button. At this point, you should be able to build the firmware in KDS.

Alternatively, you can use the build scripts of the `right/build/armgcc` directory.
