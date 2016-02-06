# Ultimate Hacking Keyboard Kinetis firmware

This repository hosts the firmware of the [Ultimate Hacking Keyboard](https://ultimatehackingkeyboard.com/), targeted at NXP Kinetis microcontrollers.

The [original firmware](https://github.com/UltimateHackingKeyboard/firmware) is already functional, but it's written for AVR microcontrollers. Kinetis microcontrollers are vastly more powerful and cost the same, allowing for future growth, so the decision has been made to port the firmware.

The current code runs on an FRDM-K22F dev board and implements a composite USB device that exposes a keyboard and mouse HID interface. It is based on the usb_device_composite_hid_mouse_hid_keyboard KSDK 2.0 demo.

## Build

1. Install Kinetis Design studio (KDS), KSDK 2.0 built for the FRDM-K22F, and set them up to be able to build the USB examples.
2. Export the `KSDK_DIR` environment variable to point to your KSDK installation directory.
3. If you wish to use KDS then:
 - Go to File -> Import -> General -> Existing Projects into Workspace, select the `build` directory, and click on the Finish button 
 - In Project Explorer right-click on the `uhk-right` project -> Properties -> Resource -> Linked Resources -> Path Variables -> edit `KSDK_DIR` to point to your KSDK installation directory.
4. At this point, you can build the firmware by using KDS or at the command line by using the scripts of the build directory.
 
## Future work

Initially, the low-level functionality has to be implemented, such as:
* A USB device that exposes a keyboard, mouse and raw HID interface
* Using kboot for the right keyboard half to directly interface with the host
* Using kboot for the left keyboard half to indirectly interface with the host via the right keyboard half using UART

Next up, the features of the AVR firmware should to be ported by using an FRDM-K0[235]Z dev board acting as the left keyboard half:
 * Make the left half send keypress events to the right half via UART. 
 * Implement a 2x2 keyboard matrix on for the dev boards and make it work in the firmware - make the keypresses send keycodes to the host or move the mouse.

Finally, implement all the advanced features, such as:
 * Receive USB control request to read and write the EEPROM.
 * Parse the content of the EEPROM to extract keymaps, macros, and other configuration information.
