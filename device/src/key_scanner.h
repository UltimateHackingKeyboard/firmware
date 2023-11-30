#ifndef __KEY_SCANNER_H__
#define __KEY_SCANNER_H__

// Includes:

    #include "device.h"
    #include "stdint.h"

// Variables:

    extern uint8_t KeyStates[KEY_MATRIX_ROWS][KEY_MATRIX_COLS];
    extern volatile char KeyPressed;

// Functions:

    extern void InitKeyScanner(void);

#endif // KEY_SCANNER_H__
