#include "ledmap.h"

rgb_key_ids_t LedMap[SLOT_COUNT][MAX_KEY_COUNT_PER_MODULE] = {
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
