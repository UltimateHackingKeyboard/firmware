#ifndef __REPORT_IDS__
#define __REPORT_IDS__

// at least Windows doesn't allow report IDs of different types to be mapped to different TLCs
enum report_ids {
    // IN
    IN_KEYBOARD_6KRO = 1,
    IN_KEYBOARD_NKRO = 2,
    IN_MOUSE = 3,
    IN_COMMAND = 4,
    IN_CONTROLS = 5,
    IN_GAMEPAD = 6,
    // OUT
    OUT_KEYBOARD_LEDS = 1,
    OUT_COMMAND = 4,
    // FEATURE
    FEATURE_MOUSE = 3,
};

#endif // __REPORT_IDS__