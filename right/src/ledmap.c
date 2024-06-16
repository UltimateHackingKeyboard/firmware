#include "key_action.h"
#include "keymap.h"
#include "layer.h"
#include "layer_switcher.h"
#include "ledmap.h"
#include "slave_drivers/is31fl3xxx_driver.h"
#include "config_parser/config_globals.h"
#include "debug.h"
#include "slot.h"
#include "config_manager.h"

#ifdef __ZEPHYR__
#include "keyboard/leds.h"
#include "keyboard/oled/screens/pairing_screen.h"
#include "state_sync.h"
#else
#include "device/device.h"
#endif

#define RGB(R, G, B) { .red = (R), .green = (G), .blue = (B)}
#define MONO(M) { .red = (M) }

static const rgb_t black = RGB(0x00, 0x00, 0x00);
static const rgb_t white = RGB(0xff, 0xff, 0xff);

backlighting_mode_t TemporaryBacklightingMode = BacklightingMode_Unspecified;

#if DEVICE_ID == DEVICE_ID_UHK60V1

static rgb_t LedMap[SLOT_COUNT][MAX_KEY_COUNT_PER_MODULE] = {
    // All three values must be set to 0 for unused

    // Right keyboard half
    {
        // IS31FL3737 has a weird memory layout for PWM values:
        // 0x?0 - 0x?5 and 0x?8 - 0x?D
        // where ? is 0-B

        // Row 1
        MONO(0), // 7
        MONO(1), // 8
        MONO(2), // 9
        MONO(3), // 0
        MONO(4), // -
        MONO(5), // =
        MONO(6), // Backspace

        // Row 2
        MONO(16), // U
        MONO(17), // I
        MONO(18), // O
        MONO(19), // P
        MONO(20), // [
        MONO(21), // ]
        MONO(22), // Backslash
        MONO(32), // Y

        // Row 3
        MONO(33), // J
        MONO(34), // K
        MONO(35), // L
        MONO(36), // ;
        MONO(37), // '
        MONO(38), // Enter
        MONO(48), // H

        // Row 4
        MONO(49), // N
        MONO(50), // M
        MONO(51), // ,
        MONO(52), // .
        MONO(53), // /
        MONO(54), // Right Shift
        MONO(0), // Unused

        // Row 5
        MONO(65), // Right Space
        MONO(0), // Right Mod (no backlight)
        MONO(67), // Right Fn
        MONO(68), // Right Alt
        MONO(69), // Right Super
        MONO(70), // Right Control
    },

    // Left keyboard half
    {
        // IS31FL3737 has a weird memory layout for PWM values:
        // 0x?0 - 0x?5 and 0x?8 - 0x?D
        // where ? is 0-B

        // Row 1
        MONO(0), // `
        MONO(1), // 1
        MONO(2), // 2
        MONO(3), // 3
        MONO(4), // 4
        MONO(5), // 5
        MONO(6), // 6

        // Row 2
        MONO(16), // Tab
        MONO(17), // Q
        MONO(18), // W
        MONO(19), // E
        MONO(20), // R
        MONO(0), // Unused
        MONO(22), // T

        // Row 3
        MONO(32), // Mouse
        MONO(33), // A
        MONO(34), // S
        MONO(35), // D
        MONO(36), // F
        MONO(0), // Unused
        MONO(38), // G

        // Row 4
        MONO(48), // ANSI Left Shift
        MONO(49), // ISO Key
        MONO(50), // Z
        MONO(51), // X
        MONO(52), // C
        MONO(53), // V
        MONO(54), // B

        // Row 5
        MONO(64), // Left Control
        MONO(65), // Left Super
        MONO(66), // Left Alt
        MONO(67), // Left Fn
        MONO(0), // Left Space (no backlight)
        MONO(69), // Left Mod
        MONO(0), // Unused
    },

    // Left module
    {
        RGB(0x05, 0x04, 0x03),
        RGB(0x02, 0x01, 0x00),
        RGB(0x07, 0x06, 0x08),
    },

    // Right module
    {
    },
};

#elif DEVICE_ID == DEVICE_ID_UHK60V2

static rgb_t LedMap[SLOT_COUNT][MAX_KEY_COUNT_PER_MODULE] = {
    // All three values must be set to 0 for unused

    // Right keyboard half
    {
        // IS31FL3737 has a weird memory layout for PWM values:
        // 0x?0 - 0x?5 and 0x?8 - 0x?D
        // where ? is 0-B

        // Row 1
        RGB(0x90, 0xA0, 0x20), // 7
        RGB(0x91, 0xA1, 0x21), // 8
        RGB(0x92, 0xA2, 0x22), // 9
        RGB(0x93, 0xA3, 0x23), // 0
        RGB(0x94, 0xA4, 0x24), // -
        RGB(0x95, 0xA5, 0x25), // =
        RGB(0x98, 0xA8, 0x28), // Backspace
        // Row 2
        RGB(0x99, 0xA9, 0x29), // U
        RGB(0x9A, 0xAA, 0x2A), // I
        RGB(0x9B, 0xAB, 0x2B), // O
        RGB(0x9C, 0xAC, 0x2C), // P
        RGB(0x60, 0x70, 0x80), // [
        RGB(0x61, 0x71, 0x81), // ]
        RGB(0x62, 0x72, 0x82), // Backslash
        RGB(0x9D, 0xAD, 0x2D), // Y

        // Row 3
        RGB(0x30, 0x40, 0x50), // J
        RGB(0x31, 0x41, 0x51), // K
        RGB(0x32, 0x42, 0x52), // L
        RGB(0x33, 0x43, 0x53), // ;
        RGB(0x34, 0x44, 0x54), // '
        RGB(0x35, 0x45, 0x55), // Enter
        RGB(0x3D, 0x4D, 0x5D), // H

        // Row 4
        RGB(0x39, 0x49, 0x59), // N
        RGB(0x3A, 0x4A, 0x5A), // M
        RGB(0x3B, 0x4B, 0x5B), // ,
        RGB(0x3C, 0x4C, 0x5C), // .
        RGB(0x38, 0x48, 0x58), // /
        RGB(0x63, 0x73, 0x83), // Right Shift
        RGB(0x00, 0x00, 0x00), // Unused

        // Row 5
        RGB(0x69, 0x79, 0x89), // Right Space
        RGB(0x00, 0x00, 0x00), // Right Mod (no backlight)
        RGB(0x6A, 0x7A, 0x8A), // Right Fn
        RGB(0x68, 0x78, 0x88), // Right Alt
        RGB(0x64, 0x74, 0x84), // Right Super
        RGB(0x65, 0x75, 0x85), // Right Control
    },

    // Left keyboard half
    {
        // IS31FL3737 has a weird memory layout for PWM values:
        // 0x?0 - 0x?5 and 0x?8 - 0x?D
        // where ? is 0-B

        // Row 1
        RGB(0x6A, 0x7A, 0x8A), // `
        RGB(0x6B, 0x7B, 0x8B), // 1
        RGB(0x6C, 0x7C, 0x8C), // 2
        RGB(0x9A, 0xAA, 0xBA), // 3
        RGB(0x9B, 0xAB, 0xBB), // 4
        RGB(0x9C, 0xAC, 0xBC), // 5
        RGB(0x9D, 0xAD, 0xBD), // 6

        // Row 2
        RGB(0x00, 0x10, 0x20), // Tab
        RGB(0x02, 0x12, 0x22), // Q
        RGB(0x04, 0x14, 0x24), // W
        RGB(0x08, 0x18, 0x28), // E
        RGB(0x0A, 0x1A, 0x2A), // R
        RGB(0x00, 0x00, 0x00), // Unused
        RGB(0x0C, 0x1C, 0x2c), // T

        // Row 3
        RGB(0x01, 0x11, 0x21), // Mouse
        RGB(0x03, 0x13, 0x23), // A
        RGB(0x05, 0x15, 0x25), // S
        RGB(0x09, 0x19, 0x29), // D
        RGB(0x0B, 0x1B, 0x2B), // F
        RGB(0x00, 0x00, 0x00), // Unused
        RGB(0x0D, 0x1D, 0x2D), // G

        // Row 4
        RGB(0x30, 0x40, 0x50), // ANSI Left Shift
//        RGB(0x6D, 0x7D, 0x8D), // ISO Left Shift
        RGB(0x32, 0x42, 0x52), // ISO Key
        RGB(0x34, 0x44, 0x54), // Z
        RGB(0x38, 0x48, 0x58), // X
        RGB(0x3A, 0x4A, 0x5A), // C
        RGB(0x3B, 0x4B, 0x5B), // V
        RGB(0x3D, 0x4D, 0x5D), // B

        // Row 5
        RGB(0x31, 0x41, 0x51), // Left Control
        RGB(0x33, 0x43, 0x53), // Left Super
        RGB(0x35, 0x45, 0x55), // Left Alt
        RGB(0x39, 0x49, 0x59), // Left Fn
        RGB(0x00, 0x00, 0x00), // Left Space (no backlight)
        RGB(0x3C, 0x4C, 0x5C), // Left Mod
        RGB(0x00, 0x00, 0x00), // Unused
    },

    // Left module
    {
        RGB(0x05, 0x04, 0x03),
        RGB(0x02, 0x01, 0x00),
        RGB(0x07, 0x06, 0x08),
    },

    // Right module
    {
    },
};

#else
// UHK 80
static rgb_t LedMap[SLOT_COUNT][MAX_KEY_COUNT_PER_MODULE] = {
    // Right keyboard half
    {
        RGB(158,157,159), // 7
        RGB(122,121,123), // 8
        RGB(86,85,87),    // 9
        RGB(50,49,51),    // 0
        RGB(14,13,15),    // -
        RGB(32,31,33),    // =
        RGB(194,193,195), // backspace
        RGB(160,161,162), // u
        RGB(124,125,126), // i
        RGB(88,89,90),    // o
        RGB(52,53,54),    // p
        RGB(16,17,18),    // [
        RGB(34,35,36),    // ]
        RGB(196,197,198), // |
        RGB(145,146,153), // y
        RGB(109,110,117), // j
        RGB(73,74,81),    // k
        RGB(37,38,45),    // l
        RGB(1,2,9),       // ;
        RGB(19,20,27),    // '
        RGB(181,182,189), // enter
        RGB(147,148,149), // h
        RGB(151,150,152), // n
        RGB(111,112,113), // m
        RGB(3,4,5),       // ,
        RGB(21,22,23),    // .
        RGB(39,40,41),    // /
        RGB(183,184,185), // shift
        RGB(0,0,0),       // unused
        RGB(115,114,116), // space
        RGB(79,78,80),    // inner case button
        RGB(75,76,77),    // fn
        RGB(7,6,8),       // alt
        RGB(25,24,26),    // super
        RGB(187,186,188), // ctrl
        RGB(155,154,156), // f7
        RGB(119,118,120), // f8
        RGB(83,82,84),    // f9
        RGB(47,46,48),    // f10
        RGB(11,10,12),    // f11
        RGB(29,28,30),    // f12
        RGB(191,190,192), // print
        RGB(140,139,141), // del
        RGB(176,175,177), // ins
        RGB(173,172,174), // scrl lck
        RGB(137,136,138), // pause
        RGB(178,179,180), // home
        RGB(142,143,144), // pg up
        RGB(163,164,171), // end
        RGB(127,128,135), // pg down
        RGB(93,94,95),    // dbl arrow left
        RGB(165,166,167), // arrow up
        RGB(129,130,131), // dbl arrow right
        RGB(97,96,98),    // arrow left
        RGB(169,168,170), // arrow down
        RGB(133,132,134), // arrow right
        RGB(43,42,44),    // right case button
    },

    // Left keyboard half
    {
        RGB(145,150,151), // tilde
        RGB(55,60,61),    // 1
        RGB(37,42,43),    // 2
        RGB(73,78,79),    // 3
        RGB(109,114,115), // 4
        RGB(127,132,133), // 5
        RGB(181,186,187), // 6
        RGB(152,153,146), // tab
        RGB(62,63,56),    // q
        RGB(44,45,38),    // w
        RGB(80,81,74),    // e
        RGB(116,117,110), // r
        RGB(0,0,0),       // unused
        RGB(134,135,128), // t
        RGB(154,155,156), // mouse
        RGB(64,65,66),    // a
        RGB(46,47,48),    // s
        RGB(82,83,84),    // d
        RGB(118,119,120), // f
        RGB(0,0,0),       // 19
        RGB(136,137,138), // g
        RGB(157,158,160), // shift
        RGB(0,0,0),       // unused
        RGB(49,50,52),    // z
        RGB(85,86,88),    // x
        RGB(121,122,124), // c
        RGB(139,140,142), // v
        RGB(193,194,196), // b
        RGB(159,162,161), // ctrl
        RGB(69,72,71),    // super
        RGB(51,54,53),    // alt
        RGB(141,144,143), // fn
        RGB(123,126,125), // inner case button
        RGB(195,198,197), // mod
        RGB(147,148,149), // esc
        RGB(57,58,59),    // f1
        RGB(39,40,41),    // f2
        RGB(75,76,77),    // f3
        RGB(111,112,113), // f4
        RGB(129,130,131), // f5
        RGB(183,184,185), // f6
        RGB(87,90,89),    // left case button
    },

    // Left module
    {
    },

    // Right module
    {
    },
};

#endif

static void setPerKeyRgb(const rgb_t* color, uint8_t slotId, uint8_t keyId)
{
    const rgb_t *ledMapItem = &LedMap[slotId][keyId];
    if (ledMapItem->red == 0 && ledMapItem->green == 0 && ledMapItem->blue == 0) {
        return;
    }
#ifndef __ZEPHYR__
    LedDriverValues[slotId][ledMapItem->red] = color->red * KeyBacklightBrightness / 255;
    float brightnessDivisor = slotId == SlotId_LeftModule ? 2 : 1;
    LedDriverValues[slotId][ledMapItem->green] = color->green * KeyBacklightBrightness / brightnessDivisor / 255;
    LedDriverValues[slotId][ledMapItem->blue] = color->blue * KeyBacklightBrightness / 255;
#else
    bool matchesRightHalf = DEVICE_IS_UHK80_RIGHT && slotId == SlotId_RightKeyboardHalf;
    bool matchesLeftHalf = DEVICE_IS_UHK80_LEFT && slotId == SlotId_LeftKeyboardHalf;
    if (matchesRightHalf || matchesLeftHalf) {
        Uhk80LedDriverValues[ledMapItem->red] = color->red;
        Uhk80LedDriverValues[ledMapItem->green] = color->green;
        Uhk80LedDriverValues[ledMapItem->blue] = color->blue;
    }
#endif
}

static void setPerKeyMonochromatic(const rgb_t* color, uint8_t slotId, uint8_t keyId)
{
    const rgb_t *ledMapItem = &LedMap[slotId][keyId];
    if (ledMapItem->red == 0 && keyId != 0) {
        return;
    }
#ifndef __ZEPHYR__
    uint8_t value = ((uint16_t)color->red + color->green + color->blue) / 3;
    LedDriverValues[slotId][ledMapItem->red] = value * KeyBacklightBrightness / 255;
#endif
}

static void setPerKeyColor(const rgb_t* color, color_mode_t mode, uint8_t slotId, uint8_t keyId)
{
    switch (mode) {
        case ColorMode_Rgb:
            setPerKeyRgb(color, slotId, keyId);
            break;
        case ColorMode_Monochromatic:
            setPerKeyMonochromatic(color, slotId, keyId);
            break;
    }
}

static color_mode_t determineMode(uint8_t slotId)
{
#if DEVICE_ID == DEVICE_ID_UHK60V1
    if (slotId <= SlotId_LeftKeyboardHalf) {
        return ColorMode_Monochromatic;
    } else {
        return ColorMode_Rgb;
    }
#else
    return ColorMode_Rgb;
#endif
}

static void updateLedsByConstantRgbStrategy() {
    for (uint8_t slotId=0; slotId<SLOT_COUNT; slotId++) {
        color_mode_t colorMode = determineMode(slotId);
        for (uint8_t keyId=0; keyId<MAX_KEY_COUNT_PER_MODULE; keyId++) {
            key_action_t *keyAction = &CurrentKeymap[ActiveLayer][slotId][keyId];
            if (keyAction->colorOverridden) {
                setPerKeyColor(&keyAction->color, colorMode, slotId, keyId);
            } else {
                setPerKeyColor(&Cfg.LedMap_ConstantRGB, colorMode, slotId, keyId);
            }
        }
    }
}

static const rgb_t* determineFunctionalMonochromatic(key_action_t* keyAction) {
    if (keyAction->type == KeyActionType_None) {
        return &black;
    } else {
        return &white;
    }
}


static rgb_t* determineFunctionalRgb(key_action_t* keyAction) {
    key_action_color_t keyActionColor;
    switch (keyAction->type) {
        case KeyActionType_Keystroke:
            if (keyAction->keystroke.scancode && keyAction->keystroke.modifiers) {
                keyActionColor = KeyActionColor_Shortcut;
            } else if (keyAction->keystroke.modifiers) {
                keyActionColor = KeyActionColor_Modifier;
            } else {
                keyActionColor = KeyActionColor_Scancode;
            }
            break;
        case KeyActionType_SwitchLayer:
            keyActionColor = KeyActionColor_SwitchLayer;
            break;
        case KeyActionType_Mouse:
            keyActionColor = KeyActionColor_Mouse;
            break;
        case KeyActionType_SwitchKeymap:
            keyActionColor = KeyActionColor_SwitchKeymap;
            break;
        case KeyActionType_PlayMacro:
            keyActionColor = KeyActionColor_Macro;
            break;
        default:
            keyActionColor = KeyActionColor_None;
            break;
    }
    return &Cfg.KeyActionColors[keyActionColor];
}

static const rgb_t* determineFunctionalColor(key_action_t* keyAction, color_mode_t mode)
{
    switch (mode) {
        case ColorMode_Rgb:
            return determineFunctionalRgb(keyAction);
        default:
        case ColorMode_Monochromatic:
            return determineFunctionalMonochromatic(keyAction);
    }
}

static key_action_t* getEffectiveActionColor(uint8_t slotId, uint8_t keyId) {
    key_action_t *keyAction = &CurrentKeymap[ActiveLayer][slotId][keyId];
    if (keyAction->type == KeyActionType_None && IS_MODIFIER_LAYER(ActiveLayer)) {
        keyAction = &CurrentKeymap[LayerId_Base][slotId][keyId];
    }
    return keyAction;
}

static void updateLedsByFunctionalStrategy() {
    for (uint8_t slotId=0; slotId<SLOT_COUNT; slotId++) {
        color_mode_t colorMode = determineMode(slotId);
        for (uint8_t keyId=0; keyId<MAX_KEY_COUNT_PER_MODULE; keyId++) {
            key_action_t *keyAction = getEffectiveActionColor(slotId, keyId);

            const rgb_t* keyActionColor = determineFunctionalColor(keyAction, colorMode);

            if (keyAction->colorOverridden) {
                setPerKeyColor(&keyAction->color, colorMode, slotId, keyId);
            } else {
                setPerKeyColor(keyActionColor, colorMode, slotId, keyId);
            }
        }
    }
}

static void updateLedsByNumpadStrategy() {
#ifdef __ZEPHYR__
    for (uint8_t slotId=0; slotId<SLOT_COUNT; slotId++) {
        color_mode_t colorMode = determineMode(slotId);
        for (uint8_t keyId=0; keyId<MAX_KEY_COUNT_PER_MODULE; keyId++) {
            key_action_t *keyAction = getEffectiveActionColor(slotId, keyId);
            const rgb_t* keyActionColor = PairingScreen_ActionColor(keyAction);

            setPerKeyColor(keyActionColor, colorMode, slotId, keyId);
        }
    }
#endif
}

static void updateLedsByPerKeyKeyStragegy() {
    for (uint8_t slotId=0; slotId<SLOT_COUNT; slotId++) {
        color_mode_t colorMode = determineMode(slotId);
        for (uint8_t keyId=0; keyId<MAX_KEY_COUNT_PER_MODULE; keyId++) {
            key_action_t *keyAction = &CurrentKeymap[ActiveLayer][slotId][keyId];
            setPerKeyColor(&keyAction->color, colorMode, slotId, keyId);
        }
    }
}

backlighting_mode_t Ledmap_GetEffectiveBacklightMode() {
    if (TemporaryBacklightingMode == BacklightingMode_Unspecified) {
        return Cfg.BacklightingMode;
    } else {
        return TemporaryBacklightingMode;
    }
}

void Ledmap_UpdateBacklightLeds(void) {
    switch (Ledmap_GetEffectiveBacklightMode()) {
        case BacklightingMode_PerKeyRgb:
            updateLedsByPerKeyKeyStragegy();
            break;
        case BacklightingMode_Functional:
            updateLedsByFunctionalStrategy();
            break;
        case BacklightingMode_ConstantRGB:
            updateLedsByConstantRgbStrategy();
            break;
        case BacklightingMode_Numpad:
            updateLedsByNumpadStrategy();
            break;
        case BacklightingMode_Unspecified:
            break;
    }
#if DEVICE_IS_UHK80_RIGHT || DEVICE_IS_UHK80_LEFT
    Uhk80_UpdateLeds();
#endif
}

void Ledmap_InitLedLayout(void) {
#if DEVICE_ID == DEVICE_ID_UHK60V2
    // clear the RGB first, since the default mapping will no longer be reachable
    setPerKeyColor(&black, ColorMode_Rgb, SlotId_LeftKeyboardHalf, LedMapIndex_LeftSlot_IsoKey);

    if (HardwareConfig->isIso) {
        LedMap[SlotId_LeftKeyboardHalf][LedMapIndex_LeftSlot_LeftShift].red = 0x6D;
        LedMap[SlotId_LeftKeyboardHalf][LedMapIndex_LeftSlot_LeftShift].green = 0x7D;
        LedMap[SlotId_LeftKeyboardHalf][LedMapIndex_LeftSlot_LeftShift].blue = 0x8D;
    } else {
        LedMap[SlotId_LeftKeyboardHalf][LedMapIndex_LeftSlot_IsoKey].red = 0;
        LedMap[SlotId_LeftKeyboardHalf][LedMapIndex_LeftSlot_IsoKey].green = 0;
        LedMap[SlotId_LeftKeyboardHalf][LedMapIndex_LeftSlot_IsoKey].blue = 0;
    }
#endif
}

void Ledmap_SetTemporaryLedBacklightingMode(backlighting_mode_t newMode) {
    TemporaryBacklightingMode = newMode;
#ifdef __ZEPHYR__
    StateSync_UpdateProperty(StateSyncPropertyId_Backlight, NULL);
#endif
}

void Ledmap_ResetTemporaryLedBacklightingMode() {
    TemporaryBacklightingMode = BacklightingMode_Unspecified;
#ifdef __ZEPHYR__
    StateSync_UpdateProperty(StateSyncPropertyId_Backlight, NULL);
#endif
}

void Ledmap_SetLedBacklightingMode(backlighting_mode_t newMode)
{
    Cfg.BacklightingMode = newMode;
#ifdef __ZEPHYR__
    StateSync_UpdateProperty(StateSyncPropertyId_Backlight, NULL);
#endif
}
