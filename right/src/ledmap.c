#include "key_action.h"
#include "keymap.h"
#include "layer_switcher.h"
#include "ledmap.h"
#include "slave_drivers/is31fl3xxx_driver.h"
#include "device/device.h"
#include "config_parser/config_globals.h"
#include "debug.h"
#include "slot.h"
#include "config_manager.h"

#define RGB(R, G, B) { .red = (R), .green = (G), .blue = (B)}
#define MONO(M) { .red = (M) }

static const rgb_t black = RGB(0x00, 0x00, 0x00);
static const rgb_t white = RGB(0xff, 0xff, 0xff);

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


#endif

static void setPerKeyRgb(const rgb_t* color, uint8_t slotId, uint8_t keyId)
{
    const rgb_t *ledMapItem = &LedMap[slotId][keyId];
    if (ledMapItem->red == 0 && ledMapItem->green == 0 && ledMapItem->blue == 0) {
        return;
    }
    LedDriverValues[slotId][ledMapItem->red] = color->red * KeyBacklightBrightness / 255;
    float brightnessDivisor = slotId == SlotId_LeftModule ? 2 : 1;
    LedDriverValues[slotId][ledMapItem->green] = color->green * KeyBacklightBrightness / brightnessDivisor / 255;
    LedDriverValues[slotId][ledMapItem->blue] = color->blue * KeyBacklightBrightness / 255;
}

static void setPerKeyMonochromatic(const rgb_t* color, uint8_t slotId, uint8_t keyId)
{
    const rgb_t *ledMapItem = &LedMap[slotId][keyId];
    if (ledMapItem->red == 0 && keyId != 0) {
        return;
    }
    uint8_t value = ((uint16_t)color->red + color->green + color->blue) / 3;
    LedDriverValues[slotId][ledMapItem->red] = value * KeyBacklightBrightness / 255;
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
#elif DEVICE_ID == DEVICE_ID_UHK60V2
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

static void updateLedsByFunctionalStrategy() {
    for (uint8_t slotId=0; slotId<SLOT_COUNT; slotId++) {
        color_mode_t colorMode = determineMode(slotId);
        for (uint8_t keyId=0; keyId<MAX_KEY_COUNT_PER_MODULE; keyId++) {
            key_action_t *keyAction = &CurrentKeymap[ActiveLayer][slotId][keyId];

            if (keyAction->type == KeyActionType_None && IS_MODIFIER_LAYER(ActiveLayer)) {
                keyAction = &CurrentKeymap[LayerId_Base][slotId][keyId];
            }

            const rgb_t* keyActionColor = determineFunctionalColor(keyAction, colorMode);

            if (keyAction->colorOverridden) {
                setPerKeyColor(&keyAction->color, colorMode, slotId, keyId);
            } else {
                setPerKeyColor(keyActionColor, colorMode, slotId, keyId);
            }
        }
    }
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

void Ledmap_UpdateBacklightLeds(void) {
    switch (Cfg.BacklightingMode) {
        case BacklightingMode_PerKeyRgb:
            updateLedsByPerKeyKeyStragegy();
            break;
        case BacklightingMode_Functional:
            updateLedsByFunctionalStrategy();
            break;
        case BacklightingMode_ConstantRGB:
            updateLedsByConstantRgbStrategy();
            break;
    }
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

void Ledmap_SetLedBacklightingMode(backlighting_mode_t newMode)
{
    Cfg.BacklightingMode = newMode;
}

