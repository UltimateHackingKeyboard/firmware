#pragma once

#include "device.h"

enum report_ids {
#if DEVICE_IS_UHK60
    IN_KEYBOARD_6KRO = 1,
    IN_KEYBOARD_NKRO = 2,
    OUT_KEYBOARD_LEDS = 1,
    FEATURE_KEYBOARD_ATTRIBUTES = 1,

    IN_MOUSE = 3,
    FEATURE_MOUSE =
        3, // mouse needs nonzero report ID as workaround for Linux high-res scrolling bug
    // https://bugzilla.kernel.org/show_bug.cgi?id=220144

    IN_CONTROLS = 0,

    IN_COMMAND = 0,
    OUT_COMMAND = 0,

    IN_GAMEPAD = 0,

    FEATURE_LEFT_LAMP_ATTRS = 8,
    FEATURE_LEFT_LAMP_ATTRS_REQ = 9,
    FEATURE_LEFT_LAMP_ATTRS_RSP = 10,
    FEATURE_LEFT_LAMP_MULTI_UPDATE = 11,
    FEATURE_LEFT_LAMP_RANGE_UPDATE = 12,
    FEATURE_LEFT_LAMP_CONTROL = 13,

    FEATURE_RIGHT_LAMP_ATTRS = 14,
    FEATURE_RIGHT_LAMP_ATTRS_REQ = 15,
    FEATURE_RIGHT_LAMP_ATTRS_RSP = 16,
    FEATURE_RIGHT_LAMP_MULTI_UPDATE = 17,
    FEATURE_RIGHT_LAMP_RANGE_UPDATE = 18,
    FEATURE_RIGHT_LAMP_CONTROL = 19,

#else
    // due to Android HOGP limitation, only one HOGP instance can exist,
    // so the applications are merged into one instance with multiple TLCs

    // at least Windows doesn't allow report IDs of different types to be mapped to different TLCs
    // IN
    IN_KEYBOARD_6KRO = 1,
    IN_KEYBOARD_NKRO = 2,
    OUT_KEYBOARD_LEDS = 1,
    FEATURE_KEYBOARD_ATTRIBUTES = 1,

    IN_MOUSE = 3,
    FEATURE_MOUSE = 3,

    IN_COMMAND = 4,
    OUT_COMMAND = 4,

    IN_CONTROLS = 5,

    IN_GAMEPAD = 6,
#endif
};
