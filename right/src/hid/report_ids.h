#ifndef __REPORT_IDS__
#define __REPORT_IDS__

#include "device.h"

enum report_ids {
#if DEVICE_IS_UHK60
    IN_KEYBOARD_6KRO = 0,
    IN_KEYBOARD_NKRO = 0,
    OUT_KEYBOARD_LEDS = 0,

    IN_MOUSE = 3,
    FEATURE_MOUSE = 3, // mouse needs nonzero report ID as workaround for Linux high-res scrolling bug

    IN_CONTROLS = 0,

    IN_COMMAND = 4, // shared with UHK80
    OUT_COMMAND = 4,

    IN_GAMEPAD = 0,
#else
    // due to Android HOGP limitation, only one HOGP instance can exist,
    // so the applications are merged into one instance with multiple TLCs

    // at least Windows doesn't allow report IDs of different types to be mapped to different TLCs
    // IN
    IN_KEYBOARD_6KRO = 1,
    IN_KEYBOARD_NKRO = 2,
    OUT_KEYBOARD_LEDS = 1,

    IN_MOUSE = 3,
    FEATURE_MOUSE = 3,

    IN_COMMAND = 4,
    OUT_COMMAND = 4,

    IN_CONTROLS = 5,

    IN_GAMEPAD = 6,
#endif
};

#endif // __REPORT_IDS__