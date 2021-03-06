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
    // Right keyboard half
    {
        // Row 1
        { .red=144, .green=160, .blue=32 }, // 7
        { .red=145, .green=161, .blue=33 }, // 8
        { .red=146, .green=162, .blue=34 }, // 9
        { .red=147, .green=163, .blue=35 }, // 0
        { .red=148, .green=164, .blue=36 }, // -
        { .red=149, .green=165, .blue=37 }, // =
        { .red=152, .green=168, .blue=0 }, // Backspace

        // Row 2
        { .red=153, .green=169, .blue=41 }, // U
        { .red=154, .green=170, .blue=42 }, // I
        { .red=155, .green=171, .blue=43 }, // O
        { .red=156, .green=172, .blue=44 }, // P
        { .red=96, .green=112, .blue=128 }, // [
        { .red=97, .green=113, .blue=129 }, // ]
        { .red=98, .green=114, .blue=130 }, // Backslash
        { .red=157, .green=173, .blue=45 }, // Y

        // Row 3
        { .red=48, .green=64, .blue=80 }, // J
        { .red=49, .green=65, .blue=81 }, // K
        { .red=50, .green=66, .blue=82 }, // L
        { .red=51, .green=67, .blue=83 }, // ;
        { .red=52, .green=68, .blue=84 }, // '
        { .red=53, .green=69, .blue=85 }, // Enter
        { .red=61, .green=77, .blue=93 }, // H

        // Row 4
        { .red=57, .green=73, .blue=89 }, // N
        { .red=58, .green=74, .blue=90 }, // M
        { .red=59, .green=75, .blue=91 }, // ,
        { .red=60, .green=76, .blue=92 }, // .
        { .red=56, .green=72, .blue=88 }, // /
        { .red=99, .green=115, .blue=131 }, // Right Shift
        { .red=0, .green=0, .blue=0 }, // Unused

        // Row 5
        { .red=105, .green=121, .blue=137 }, // Right Space
        { .red=0, .green=0, .blue=0 }, // Right Mod (no backlight)
        { .red=106, .green=122, .blue=138 }, // Right Fn
        { .red=104, .green=120, .blue=136 }, // Right Alt
        { .red=100, .green=116, .blue=132 }, // Right Super
        { .red=101, .green=117, .blue=133 }, // Right Control
    },

    // Left keyboard half
    {
        // Row 1
        { .red=106, .green=122, .blue=138 }, // `
        { .red=107, .green=123, .blue=139 }, // 1
        { .red=108, .green=124, .blue=140 }, // 2
        { .red=154, .green=170, .blue=186 }, // 3
        { .red=155, .green=171, .blue=187 }, // 4
        { .red=156, .green=172, .blue=188 }, // 5
        { .red=157, .green=173, .blue=189 }, // 6

        // Row 2
        { .red=0, .green=16, .blue=29 }, // Tab
        { .red=2, .green=18, .blue=34 }, // Q
        { .red=4, .green=20, .blue=36 }, // W
        { .red=8, .green=24, .blue=40 }, // E
        { .red=10, .green=26, .blue=42 }, // R
        { .red=0, .green=0, .blue=0 }, // Unused
        { .red=12, .green=28, .blue=44 }, // T

        // Row 3
        { .red=1, .green=17, .blue=33 }, // Mouse
        { .red=3, .green=19, .blue=35 }, // A
        { .red=5, .green=21, .blue=37 }, // S
        { .red=9, .green=25, .blue=41 }, // D
        { .red=11, .green=27, .blue=43 }, // F
        { .red=0, .green=0, .blue=0 }, // Unused
        { .red=13, .green=29, .blue=45 }, // G

        // Row 4
        { .red=48, .green=64, .blue=80 }, // ANSI Left Shift
//        { .red=109, .green=125, .blue=141 }, // ISO Left Shift
        { .red=50, .green=66, .blue=82 }, // ISO Key
        { .red=52, .green=68, .blue=84 }, // Z
        { .red=56, .green=72, .blue=88 }, // X
        { .red=58, .green=74, .blue=90 }, // C
        { .red=59, .green=75, .blue=91 }, // V
        { .red=61, .green=77, .blue=93 }, // B

        // Row 5
        { .red=49, .green=65, .blue=81 }, // Left Control
        { .red=51, .green=67, .blue=83 }, // Left Super
        { .red=53, .green=69, .blue=85 }, // Left Alt
        { .red=57, .green=73, .blue=89 }, // Left Fn
        { .red=0, .green=0, .blue=0 }, // Left Space (no backlight)
        { .red=60, .green=76, .blue=92 }, // Left Mod
        { .red=0, .green=0, .blue=0 } // Unused
    },

    // Left module
    {
        { .red=5, .green=4, .blue=3 },
        { .red=2, .green=1, .blue=0 },
        { .red=7, .green=6, .blue=8 },
    },

    // Right module
    {
    },
};

uint8_t validZeroKeyIds[SLOT_COUNT] = {
    6, // Right keyboard half
    7, // Left keyboard half
    1, // Left module
    0, // Right module
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
            if (ledMapItem->red != 0 || (ledMapItem->red == 0 && validZeroKeyIds[slotId] == keyId)) {
                LedDriverValues[slotId][ledMapItem->red] = keyActionColorValues->red * KeyBacklightBrightness / 255;
            }
            if (ledMapItem->green != 0 || (ledMapItem->green== 0 && validZeroKeyIds[slotId] == keyId)) {
                float brightnessDivisor = slotId == SlotId_LeftModule ? 2 : 1;
                LedDriverValues[slotId][ledMapItem->green] = keyActionColorValues->green * KeyBacklightBrightness / brightnessDivisor / 255;
            }
            if (ledMapItem->blue != 0 || (ledMapItem->blue == 0 && validZeroKeyIds[slotId] == keyId)) {
                LedDriverValues[slotId][ledMapItem->blue] = keyActionColorValues->blue * KeyBacklightBrightness / 255;
            }
        }
    }
#endif
}
