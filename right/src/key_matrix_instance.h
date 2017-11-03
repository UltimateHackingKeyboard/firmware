#ifndef __KEY_MATRIX_INSTANCE_H__
#define __KEY_MATRIX_INSTANCE_H__

// Includes:

    #include "key_matrix.h"

// Macros:

    #define KEYBOARD_MATRIX_COLS_NUM 7
    #define KEYBOARD_MATRIX_ROWS_NUM 5
    #define KEYBOARD_MATRIX_KEY_COUNT (KEYBOARD_MATRIX_COLS_NUM * KEYBOARD_MATRIX_ROWS_NUM)

// Variables:

    extern key_matrix_t KeyMatrix;

#endif
