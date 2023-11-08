#ifndef __USB_HEADER__
#define __USB_HEADER__

#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

// TODO: add parameter to select initial mode (gamepad yes/no, keyboard 6/N KRO)
extern void usb_init(bool gamepad_enable);

#ifdef __cplusplus
}
#endif

extern void sendUsbReports(void);

#endif // __USB_HEADER__
