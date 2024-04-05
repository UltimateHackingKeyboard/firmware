#include "layer.h"
#include "lufa/HIDClassCommon.h"

const char* LayerNames[LayerId_Count] = {
    "base",
    "mod",
    "fn",
    "mouse",
    "fn2",
    "fn3",
    "fn4",
    "fn5",
    "shift",
    "ctrl",
    "alt",
    "super",
};

layer_config_t LayerConfig[LayerId_Count] = {
    { .layerIsDefined = true, .exactModifierMatch = false, .modifierLayerMask = 0},
    { .layerIsDefined = true, .exactModifierMatch = false, .modifierLayerMask = 0},
    { .layerIsDefined = true, .exactModifierMatch = false, .modifierLayerMask = 0},
    { .layerIsDefined = true, .exactModifierMatch = false, .modifierLayerMask = 0},

    // fn2 - fn5
    { .layerIsDefined = false, .exactModifierMatch = false, .modifierLayerMask = 0},
    { .layerIsDefined = false, .exactModifierMatch = false, .modifierLayerMask = 0},
    { .layerIsDefined = false, .exactModifierMatch = false, .modifierLayerMask = 0},
    { .layerIsDefined = false, .exactModifierMatch = false, .modifierLayerMask = 0},

    // Shift, Control, Alt, Super
    { .layerIsDefined = false, .exactModifierMatch = false, .modifierLayerMask = HID_KEYBOARD_MODIFIER_LEFTSHIFT | HID_KEYBOARD_MODIFIER_RIGHTSHIFT },
    { .layerIsDefined = false, .exactModifierMatch = false, .modifierLayerMask = HID_KEYBOARD_MODIFIER_LEFTCTRL | HID_KEYBOARD_MODIFIER_RIGHTCTRL },
    { .layerIsDefined = false, .exactModifierMatch = false, .modifierLayerMask = HID_KEYBOARD_MODIFIER_LEFTALT | HID_KEYBOARD_MODIFIER_RIGHTALT },
    { .layerIsDefined = false, .exactModifierMatch = false, .modifierLayerMask = HID_KEYBOARD_MODIFIER_LEFTGUI | HID_KEYBOARD_MODIFIER_RIGHTGUI },
};

