# Ultimate Hacking Keyboard Kinetis firmware

This repository hosts the firmware of the [Ultimate Hacking Keyboard](https://ultimatehackingkeyboard.com/), targeted at NXP Kinetis microcontrollers.

The [original firmware](https://github.com/UltimateHackingKeyboard/firmware) is already functional, but it's written for AVR microcontrollers. Kinetis microcontrollers are vastly more powerful and cost the same, allowing for future growth, so the decision has been made to port the firmware.

The current code runs on an FRDM-K22F dev board and implements a composite USB device that exposes a keyboard and mouse HID interface. It is based on the `usb_device_composite_hid_mouse_hid_keyboard` KSDK 2.0 demo.

## Build

Please make sure to clone this repo with:

`git clone --recursive git@github.com:UltimateHackingKeyboard/firmware-kinetis.git`

You're encouraged to install the [Kinetis Design Studio (KDS) IDE](http://www.nxp.com/products/software-and-tools/run-time-software/kinetis-software-and-tools/ides-for-kinetis-mcus/kinetis-design-studio-integrated-development-environment-ide:KDS_IDE) and import the project by invoking File -> Import -> General -> Existing Projects into Workspace, select the `right` directory, and click on the Finish button. At this point, you should be able to build the firmware in KDS.

Alternatively, you can use the build scripts of the `right/build/armgcc` directory.
