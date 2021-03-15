#include "keymap.h"
#include "layer_switcher.h"
#include "ledmap.h"
#include "slave_drivers/is31fl3xxx_driver.h"
#include "device.h"

rgb_t KeyActionColors[] = {
    {.red=0, .green=0, .blue=0}, // KeyActionColor_None
    {.red=255, .green=255, .blue=255}, // KeyActionColor_Scancode
    {.red=0, .green=255, .blue=255}, // KeyActionColor_Modifier
    {.red=0, .green=0, .blue=255}, // KeyActionColor_Shortcut
    {.red=255, .green=255, .blue=0}, // KeyActionColor_SwitchLayer
    {.red=255, .green=0, .blue=0}, // KeyActionColor_SwitchKeymap
    {.red=0, .green=255, .blue=0}, // KeyActionColor_Mouse
    {.red=255, .green=0, .blue=255}, // KeyActionColor_Macro
};

rgb_t LedMap[SLOT_COUNT][MAX_KEY_COUNT_PER_MODULE] = {
    // All three values must be set to 0 for unused
    
    // Right keyboard half
    {
        // IS31FL3737 has a weird memory layout for PWM values:
        // 0x?0 - 0x?5 and  0x?8 - 0x?D
        //   where ? is  0-B
        // Row 1
        { .red=0x90, .green=0xA0, .blue=0x20 }, // 7
        { .red=0x91, .green=0xA1, .blue=0x21 }, // 8
        { .red=0x92, .green=0xA2, .blue=0x22 }, // 9
        { .red=0x93, .green=0xA3, .blue=0x23 }, // 0
        { .red=0x94, .green=0xA4, .blue=0x24 }, // -
        { .red=0x95, .green=0xA5, .blue=0x25 }, // =
        { .red=0x98, .green=0xA8, .blue=0x28 }, // Backspace
        // Row 2
        { .red=0x99, .green=0xA9, .blue=0x29 }, // U
        { .red=0x9A, .green=0xAA, .blue=0x2A }, // I
        { .red=0x9B, .green=0xAB, .blue=0x2B }, // O
        { .red=0x9C, .green=0xAC, .blue=0x2C }, // P
        { .red=0x60, .green=0x70, .blue=0x80 }, // [
        { .red=0x61, .green=0x71, .blue=0x81 }, // ]
        { .red=0x62, .green=0x72, .blue=0x82 }, // Backslash
        { .red=0x9D, .green=0xAD, .blue=0x2D }, // Y

        // Row 3
        { .red=0x30, .green=0x40, .blue=0x50 }, // J
        { .red=0x31, .green=0x41, .blue=0x51 }, // K
        { .red=0x32, .green=0x42, .blue=0x52 }, // L
        { .red=0x33, .green=0x43, .blue=0x53 }, // ;
        { .red=0x34, .green=0x44, .blue=0x54 }, // '
        { .red=0x35, .green=0x45, .blue=0x55 }, // Enter
        { .red=0x3D, .green=0x4D, .blue=0x5D }, // H

        // Row 4
        { .red=0x39, .green=0x49, .blue=0x59 }, // N
        { .red=0x3A, .green=0x4A, .blue=0x5A }, // M
        { .red=0x3B, .green=0x4B, .blue=0x5B }, // ,
        { .red=0x3C, .green=0x4C, .blue=0x5C }, // .
        { .red=0x38, .green=0x48, .blue=0x58 }, // /
        { .red=0x63, .green=0x73, .blue=0x83}, // Right Shift
        { .red=0,    .green=0,    .blue=0   }, // Unused

        // Row 5
        { .red=0x69, .green=0x79, .blue=0x89 }, // Right Space
        { .red=0,    .green=0,    .blue=0    }, // Right Mod (no backlight)
        { .red=0x6A, .green=0x7A, .blue=0x8A }, // Right Fn
        { .red=0x68, .green=0x78, .blue=0x88 }, // Right Alt
        { .red=0x64, .green=0x74, .blue=0x84 }, // Right Super
        { .red=0x65, .green=0x75, .blue=0x85 }, // Right Control
    },

    // Left keyboard half
    {
        // IS31FL3737 has a weird memory layout for PWM values:
        // 0x?0 - 0x?5 and  0x?8 - 0x?D
        //   where ? is  0-B        
        // Row 1
        { .red=0x6A, .green=0x7A, .blue=0x8A }, // `
        { .red=0x6B, .green=0x7B, .blue=0x8B }, // 1
        { .red=0x6C, .green=0x7C, .blue=0x8C }, // 2
        { .red=0x9A, .green=0xAA, .blue=0xBA }, // 3
        { .red=0x9B, .green=0xAB, .blue=0xBB }, // 4
        { .red=0x9C, .green=0xAC, .blue=0xBC }, // 5
        { .red=0x9D, .green=0xAD, .blue=0xBD }, // 6

        // Row 2
        { .red=0x00, .green=0x10, .blue=0x20 }, // Tab
        { .red=0x02, .green=0x12, .blue=0x22 }, // Q
        { .red=0x04, .green=0x14, .blue=0x24 }, // W
        { .red=0x08, .green=0x18, .blue=0x28 }, // E
        { .red=0x0A, .green=0x1A, .blue=0x2A }, // R
        { .red=0,    .green=0,    .blue=0    }, // Unused
        { .red=0x0C, .green=0x1C, .blue=0x2c }, // T

        // Row 3
        { .red=0x01, .green=0x11, .blue=0x21 }, // Mouse
        { .red=0x03, .green=0x13, .blue=0x23 }, // A
        { .red=0x05, .green=0x15, .blue=0x25 }, // S
        { .red=0x09, .green=0x19, .blue=0x29 }, // D
        { .red=0x0B, .green=0x1B, .blue=0x2B }, // F
        { .red=0,    .green=0,    .blue=0    }, // Unused
        { .red=0x0D, .green=0x1D, .blue=0x2D }, // G

        // Row 4
        { .red=0x30, .green=0x40, .blue=0x50 }, // ANSI Left Shift
//        { .red=0x6D, .green=0x7D, .blue=0x8D }, // ISO Left Shift
        { .red=0x32, .green=0x42, .blue=0x52 }, // ISO Key
        { .red=0x34, .green=0x44, .blue=0x54 }, // Z
        { .red=0x38, .green=0x48, .blue=0x58 }, // X
        { .red=0x3A, .green=0x4A, .blue=0x5A }, // C
        { .red=0x3B, .green=0x4B, .blue=0x5B }, // V
        { .red=0x3D, .green=0x4D, .blue=0x5D }, // B

        // Row 5
        { .red=0x31, .green=0x41, .blue=0x51 }, // Left Control
        { .red=0x33, .green=0x43, .blue=0x53 }, // Left Super
        { .red=0x35, .green=0x45, .blue=0x55 }, // Left Alt
        { .red=0x39, .green=0x49, .blue=0x59 }, // Left Fn
        { .red=0,    .green=0,    .blue=0    }, // Left Space (no backlight)
        { .red=0x3C, .green=0x4C, .blue=0x5C }, // Left Mod
        { .red=0,    .green=0,    .blue=0 } // Unused
    },

    // Left module
    {
        { .red=0x05, .green=0x04, .blue=0x03 },
        { .red=0x02, .green=0x01, .blue=0x00 },
        { .red=0x07, .green=0x06, .blue=0x08 },
    },

    // Right module
    {
    },
};

void UpdateLayerLeds(void) {
#if DEVICE_ID == DEVICE_ID_UHK60V2
    for (uint8_t slotId=0; slotId<SLOT_COUNT; slotId++) {
        for (uint8_t keyId=0; keyId<MAX_KEY_COUNT_PER_MODULE; keyId++) {
            key_action_color_t keyActionColor;
//            key_action_t (*layerKeyActions)[SLOT_COUNT][MAX_KEY_COUNT_PER_MODULE] = &CurrentKeymap[ActiveLayer];
//            key_action_t *keyAction = layerKeyActions[slotId][keyId];
            key_action_t *keyAction = &CurrentKeymap[ActiveLayer][slotId][keyId];
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

            rgb_t *keyActionColorValues = &KeyActionColors[keyActionColor];
            rgb_t *ledMapItem = &LedMap[slotId][keyId];
            if (ledMapItem->red == 0 && ledMapItem->green == 0 && ledMapItem->blue == 0) {
                continue;
            }
            LedDriverValues[slotId][ledMapItem->red] = keyActionColorValues->red * KeyBacklightBrightness / 255;
            float brightnessDivisor = slotId == SlotId_LeftModule ? 2 : 1;
            LedDriverValues[slotId][ledMapItem->green] = keyActionColorValues->green * KeyBacklightBrightness / brightnessDivisor / 255;
            LedDriverValues[slotId][ledMapItem->blue] = keyActionColorValues->blue * KeyBacklightBrightness / 255;
        }
    }
#endif
}
