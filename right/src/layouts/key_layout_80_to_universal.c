#include "key_layout_80_to_universal.h"

#ifdef __ZEPHYR__

const uint8_t KeyLayout_Uhk80_to_Universal[SLOT_COUNT][MAX_KEY_COUNT_PER_MODULE] = {
    {
        [0] = 35, // f7
        [1] = 36, // f8
        [2] = 37, // f9
        [3] = 38, // f10
        [4] = 39, // f11
        [5] = 40, // f12
        [6] = 41, // print
        [8] = 42, // del
        [9] = 43, // ins

        [10] = 0, // 7
        [11] = 1, // 8
        [12] = 2, // 9
        [13] = 3, // 0
        [14] = 4, // -
        [15] = 5, // =
        [16] = 6, // backspace
        [18] = 44, // scrl lck
        [19] = 45, // pause

        [30] = 7, // y
        [20] = 8, // u
        [21] = 9, // i
        [22] = 10, // o
        [23] = 11, // p
        [24] = 12, // [
        [25] = 13, // ]
        [26] = 14, // |
        [28] = 46, // home
        [29] = 47, // pg up

        [40] = 15, // h
        [31] = 16, // j
        [32] = 17, // k
        [33] = 18, // l
        [34] = 19, // ;
        [35] = 20, // '
        [36] = 21, // enter
        [38] = 48, // end
        [39] = 49, // pg down

        [50] = 22, // n
        [41] = 23, // m
        [43] = 24, // ,
        [44] = 25, // .
        [45] = 26, // /
        [46] = 27, // shift
        [47] = 50, // dbl arrow left
        [48] = 51, // arrow up
        [49] = 52, // dbl arrow right

        [51] = 28, // space
        [42] = 29, // fn
        [54] = 30, // alt
        [55] = 31, // super
        [56] = 32, // ctrl
        [57] = 53, // arrow left
        [58] = 54, // arrow down
        [59] = 55, // arrow right

        [52] = 33, // inner case button
        [53] = 34, // right case button

        // empty
        [7] = 255,
        [17] = 255,
        [27] = 255,
        [37] = 255,
    },
    {
        // UHK60 keys

        [0] = 33, // esc
        [1] = 34, // f1
        [2] = 35, // f2
        [3] = 36, // f3
        [4] = 37, // f4
        [5] = 38, // f5
        [6] = 39, // f6

        [7] = 0, // tilde
        [8] = 1, // 1
        [9] = 2, // 2
        [10] = 3, // 3
        [11] = 4, // 4
        [12] = 5, // 5
        [13] = 6, // 6


        [14] = 7, // tab
        [15] = 8, // q
        [16] = 9, // w
        [17] = 10, // e
        [18] = 11, // r
        [19] = 12, // t

        [21] = 13, // mouse
        [22] = 14, // a
        [23] = 15, // s
        [24] = 16, // d
        [25] = 17, // f
        [26] = 18, // g

        [28] = 19, // shift
        [29] = 20, // iso key
        [30] = 21, // z
        [31] = 22, // x
        [32] = 23, // c
        [33] = 24, // v
        [34] = 25, // b

        [35] = 26, // ctrl
        [36] = 27, // super
        [37] = 28, // alt
        [40] = 29, // fn
        [41] = 30, // mod

        [39] = 31, // inner case button
        [38] = 32, // left case button

        // unused
        [20] = 255,
        [27] = 255,
        [42] = 255,
        [43] = 255,
        [44] = 255,
        [45] = 255,
        [46] = 255,
        [47] = 255,
        [48] = 255,
        [49] = 255,
        [50] = 255,
        [51] = 255,
        [52] = 255,
        [53] = 255,
        [54] = 255,
        [55] = 255,
        [56] = 255,
        [57] = 255,
        [58] = 255,
        [59] = 255,
    },
    {},
    {},
};

#endif
