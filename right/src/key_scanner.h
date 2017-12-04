#ifndef __KEY_SCANNER_H__
#define __KEY_SCANNER_H__

// Includes:

    #include "peripherals/pit.h"
    #include "right_key_matrix.h"

// Macros:

    #define KEY_SCANNER_INTERVAL_USEC (1000 / RIGHT_KEY_MATRIX_ROWS_NUM)

// Variables:

    extern uint32_t KeyScannerCounter;

// Functions:

    void InitKeyScanner(void);

#endif
