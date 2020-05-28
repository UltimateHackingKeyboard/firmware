#include "ledmap.h"

rgb_key_ids_t LedMap[SLOT_COUNT][MAX_KEY_COUNT_PER_MODULE] = {
    // Right keyboard half
    {
        // Row 1
        { .red=0, .green=0, .blue=0 }, // 7
        { .red=0, .green=0, .blue=0 }, // 8
        { .red=0, .green=0, .blue=0 }, // 9
        { .red=0, .green=0, .blue=0 }, // 0
        { .red=0, .green=0, .blue=0 }, // -
        { .red=0, .green=0, .blue=0 }, // =
        { .red=0, .green=0, .blue=0 }, // Backspace

        // Row 2
        { .red=0, .green=0, .blue=0 }, // U
        { .red=0, .green=0, .blue=0 }, // I
        { .red=0, .green=0, .blue=0 }, // O
        { .red=0, .green=0, .blue=0 }, // P
        { .red=0, .green=0, .blue=0 }, // [
        { .red=0, .green=0, .blue=0 }, // ]
        { .red=0, .green=0, .blue=0 }, // Backslash
        { .red=0, .green=0, .blue=0 }, // Y

        // Row 3
        { .red=0, .green=0, .blue=0 }, // J
        { .red=0, .green=0, .blue=0 }, // K
        { .red=0, .green=0, .blue=0 }, // L
        { .red=0, .green=0, .blue=0 }, // ;
        { .red=0, .green=0, .blue=0 }, // '
        { .red=0, .green=0, .blue=0 }, // Enter
        { .red=0, .green=0, .blue=0 }, // H

        // Row 4
        { .red=0, .green=0, .blue=0 }, // N
        { .red=0, .green=0, .blue=0 }, // M
        { .red=0, .green=0, .blue=0 }, // ,
        { .red=0, .green=0, .blue=0 }, // .
        { .red=0, .green=0, .blue=0 }, // /
        { .red=0, .green=0, .blue=0 }, // Right Shift
        { .red=0, .green=0, .blue=0 }, // Unused

        // Row 5
        { .red=0, .green=0, .blue=0 }, // Right Space
        { .red=0, .green=0, .blue=0 }, // Right Mod
        { .red=0, .green=0, .blue=0 }, // Right Fn
        { .red=0, .green=0, .blue=0 }, // Right Alt
        { .red=0, .green=0, .blue=0 }, // Right Super
        { .red=0, .green=0, .blue=0 }, // Right Control
    },

    // Left keyboard half
    {
        // Row 1
        { .red=0, .green=0, .blue=0 }, // `
        { .red=0, .green=0, .blue=0 }, // 1
        { .red=0, .green=0, .blue=0 }, // 2
        { .red=0, .green=0, .blue=0 }, // 3
        { .red=0, .green=0, .blue=0 }, // 4
        { .red=0, .green=0, .blue=0 }, // 5
        { .red=0, .green=0, .blue=0 }, // 6

        // Row 2
        { .red=0, .green=0, .blue=0 }, // Tab
        { .red=0, .green=0, .blue=0 }, // Q
        { .red=0, .green=0, .blue=0 }, // W
        { .red=0, .green=0, .blue=0 }, // E
        { .red=0, .green=0, .blue=0 }, // R
        { .red=0, .green=0, .blue=0 }, // Unused
        { .red=0, .green=0, .blue=0 }, // T

        // Row 3
        { .red=0, .green=0, .blue=0 }, // Mouse
        { .red=0, .green=0, .blue=0 }, // A
        { .red=0, .green=0, .blue=0 }, // S
        { .red=0, .green=0, .blue=0 }, // D
        { .red=0, .green=0, .blue=0 }, // F
        { .red=0, .green=0, .blue=0 }, // Unused
        { .red=0, .green=0, .blue=0 }, // G

        // Row 4
        { .red=0, .green=0, .blue=0 }, // Left Shit
        { .red=0, .green=0, .blue=0 }, // ISO Key
        { .red=0, .green=0, .blue=0 }, // Z
        { .red=0, .green=0, .blue=0 }, // X
        { .red=0, .green=0, .blue=0 }, // C
        { .red=0, .green=0, .blue=0 }, // V
        { .red=0, .green=0, .blue=0 }, // B

        // Row 5
        { .red=0, .green=0, .blue=0 }, // Left Control
        { .red=0, .green=0, .blue=0 }, // Left Super
        { .red=0, .green=0, .blue=0 }, // Left Alt
        { .red=0, .green=0, .blue=0 }, // Left Fn
        { .red=0, .green=0, .blue=0 }, // Left Space
        { .red=0, .green=0, .blue=0 }, // Left Mod
        { .red=0, .green=0, .blue=0 } // Unused
    },
};
