# Ultimate Hacking Keyboard Kinetis firmware

This repository hosts the firmware of the [Ultimate Hacking Keyboard](https://ultimatehackingkeyboard.com/), targeted at NXP Kinetis microcontrollers.

The [original firmware](https://github.com/UltimateHackingKeyboard/firmware) is already functional, but it's written for AVR microcontrollers. Kinetis microcontrollers are vastly more powerful and cost the same, allowing for future growth, so the decision has been made to port the firmware.

The current code runs on an FRDM-K22F dev board and implements a composite USB device that exposes a keyboard and mouse HID interface. It is based on the `usb_device_composite_hid_mouse_hid_keyboard` KSDK 2.0 demo.

## Build

Please make sure to clone this repo with:

`git clone --recursive git@github.com:UltimateHackingKeyboard/firmware-kinetis.git`

You can install the [Kinetis Design Studio (KDS) IDE](http://www.nxp.com/products/software-and-tools/run-time-software/kinetis-software-and-tools/ides-for-kinetis-mcus/kinetis-design-studio-integrated-development-environment-ide:KDS_IDE) and import the project by invoking File -> Import -> General -> Existing Projects into Workspace, select the `right` directory, and click on the Finish button. At this point, you should be able to build the firmware in KDS.

Alternatively, you can use the build scripts of the `right/build/armgcc` directory.

## Future work

Initially, the low-level functionality has to be implemented, such as:
* A USB device that exposes a keyboard, mouse and raw HID interface
* Using kboot for the right keyboard half to directly interface with the host
* Using kboot for the left keyboard half to indirectly interface with the host via the right keyboard half using UART

Next up, the features of the AVR firmware should to be ported by using an FRDM-K0[235]Z dev board acting as the left keyboard half:
 * Make the left half send keypress events to the right half via UART.
 * Breadboard a 2x2 keyboard matrix wired per dev board and make them work in the firmware. Make the keypresses send keycodes to the host or move the mouse.

Finally, implement all the advanced features, such as:
 * Send USB control request to write the EEPROM.
 * Read the EEPROM via the raw HID interface.
 * Parse the content of the EEPROM to extract keymaps, macros, and other configuration information.
