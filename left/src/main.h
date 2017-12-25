#ifndef __MAIN_H__
#define __MAIN_H__

// Includes:

    #include "key_matrix.h"

// Macros:

    #define KEYBOARD_MATRIX_COLS_NUM 7
    #define KEYBOARD_MATRIX_ROWS_NUM 5

// Variables:

    extern key_matrix_t keyMatrix;

   #define KEY_USE_I2C_WATCHDOG_TIMER   (1) /* if set to 1, every 100 ms the I2C communication is checked */

#endif
