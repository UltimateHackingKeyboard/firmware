#ifndef __MAIN_H__
#define __MAIN_H__

// Includes:

    #include "key_matrix.h"
    #include "slot.h"
    #include "module.h"

// Macros:

    #define KEYBOARD_MATRIX_COLS_NUM 7
    #define KEYBOARD_MATRIX_ROWS_NUM 5

// Variables:

    extern key_matrix_t KeyMatrix;
    extern uint8_t PreviousKeyStates[SLOT_COUNT][MAX_KEY_COUNT_PER_MODULE];
    extern uint8_t CurrentKeyStates[SLOT_COUNT][MAX_KEY_COUNT_PER_MODULE];
    extern void UpdateUsbReports(void);

#endif
