#include "key_layout.h"

const uint8_t KeyLayout_Uhk80_to_Uhk60[SLOT_COUNT][MAX_KEY_COUNT_PER_MODULE] = {
    {
        // UHK60 keys
        [10] = 0, // 7
        [11] = 1, // 8
        [12] = 2, // 9
        [13] = 3, // 0
        [14] = 4, // -
        [15] = 5, // =
        [16] = 6, // backspace

        [30] = 14, // y
        [20] = 7, // u
        [21] = 8, // i
        [22] = 9, // o
        [23] = 10, // p
        [24] = 11, // [
        [25] = 12, // ]
        [26] = 13, // |

        [40] = 21, // h
        [31] = 15, // j
        [32] = 16, // k
        [33] = 17, // l
        [34] = 18, // ;
        [35] = 19, // '
        [36] = 20, // enter

        [50] = 22, // n
        [41] = 23, // m
        [43] = 24, // ,
        [44] = 25, // .
        [45] = 26, // /
        [46] = 27, // shift

        [51] = 29, // mod
        [42] = 31, // fn
        [54] = 32, // alt
        [55] = 33, // super
        [56] = 34, // ctrl

        [52] = 30, // case button

        // new keys

        [0] = 35, // f7
        [1] = 36, // f8
        [2] = 37, // f9
        [3] = 38, // f10
        [4] = 39, // f11
        [5] = 40, // f12
        [6] = 41, // print
        [8] = 42, // del
        [9] = 43, // ins

        [18] = 44, // scrl lck
        [19] = 45, // pause

        [28] = 46, // home
        [29] = 47, // pg up

        [38] = 48, // end
        [39] = 49, // pg down

        [47] = 50, // dbl arrow left
        [48] = 51, // arrow up
        [49] = 52, // dbl arrow right

        [57] = 53, // arrow right
        [58] = 54, // arrow down
        [59] = 55, // arrow right

        [53] = 56, // right case button
    },
    {
        // TODO: this,
    },
    {},
    {},
};
