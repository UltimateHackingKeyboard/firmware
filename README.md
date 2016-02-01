# Ultimate Hacking Keyboard Kinetis firmware

This repository will host the firmware of the [Ultimate Hacking Keyboard](https://ultimatehackingkeyboard.com/), targeted at NXP Kinetis microcontrollers.

The [original firmware](https://github.com/UltimateHackingKeyboard/firmware) is already functional, but it's written for AVR microcontrollers. Kinetis microcontrollers are vastly more powerful and cost the same, allowing for future growth, so a decision has been made to port the firmware.

The current code runs on an FRDM-K22F dev board and implements a composite USB device that exposes a keyboard and mouse HID interface. It was created by fusing the `dev_hid_audio_bm_frdmk22f` and `dev_hid_keyboard_bm_frdmk22f` KSDK examples.

## Build

1. Install Kinetis Design studio (KDS), KSDK 1.3, and set them up to be able to build the USB examples.
2. Export the `KSDK_DIR` environment variable to point to the KSDK installation directory.
3. If you wish to use KDS then:
 - Go to File -> Import -> General -> Existing Projects into Workspace, select the `build` directory, and click on the Finish button 
 - Right click on the `uhk-right` project -> Properties -> Resource -> Linked Resources -> Path Variables -> edit `KSDK_DIR` to point to your KSDK installation directory.
4. At this point you can build the firmware by using KDS or at the command line by using the scripts of the build directory.
 
## Known bugs

The USB device does not enumerate properly, yielding the following errors on Linux:

```
[25630.799145] usb 1-11: new full-speed USB device number 96 using xhci_hcd
[25635.906816] usb 1-11: device descriptor read/64, error -71
[25636.122623] usb 1-11: device descriptor read/64, error -71
[25636.338444] usb 1-11: new full-speed USB device number 97 using xhci_hcd
[25636.450357] usb 1-11: device descriptor read/64, error -71
[25636.666170] usb 1-11: device descriptor read/64, error -71
[25636.882022] usb 1-11: new full-speed USB device number 98 using xhci_hcd
[25636.882170] usb 1-11: Device not responding to setup address.
[25637.085938] usb 1-11: Device not responding to setup address.
[25637.289680] usb 1-11: device not accepting address 98, error -71
[25637.401584] usb 1-11: new full-speed USB device number 99 using xhci_hcd
[25637.401745] usb 1-11: Device not responding to setup address.
[25637.605469] usb 1-11: Device not responding to setup address.
```

## Future work

Initially, the low-level functionality has to be implemented, such as:
* A USB device that exposes a keyboard, mouse and raw HID interface
* Using kboot for the right keyboard half to directly interface with the host
* Using kboot for the left keyboard half to indirectly interface with the host via the right keyboard half using UART
