#ifndef __USB_HEADER__
#define __USB_HEADER__

#include <zephyr/device.h>

// TODO: add parameter to select initial mode (gamepad yes/no, keyboard 6/N KRO)
void usb_init(const device* dev);

#endif // __USB_HEADER__