#ifndef __USB_HEADER__
#define __USB_HEADER__

#include <zephyr/device.h>

typedef enum { // Any way to rely on the definition of keyboard_app::rollover instead of duplicating it?
    Rollover_NKey = 0,
    Rollover_6Key = 1,
} rollover_t;

#ifdef __cplusplus
extern "C" {
#endif

extern void usb_init(bool gamepad_enable); // TODO: add parameter to select initial mode (gamepad yes/no, keyboard 6/N KRO)
extern uint8_t USB_GetKeyboardRollover(void);
extern void USB_SetKeyboardRollover(uint8_t mode);

#ifdef __cplusplus
}
#endif

#endif // __USB_HEADER__
