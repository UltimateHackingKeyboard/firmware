#ifndef __KEY_SCANNER_H__
#define __KEY_SCANNER_H__

// Includes:

    #include "key_matrix_instance.h"

// Macros:

    #define KEY_SCANNER_INTERVAL_USEC (1000 / KEYBOARD_MATRIX_ROWS_NUM)

    #define PIT_KEY_SCANNER_HANDLER PIT1_IRQHandler
    #define PIT_KEY_SCANNER_IRQ_ID PIT1_IRQn
    #define PIT_KEY_SCANNER_CHANNEL kPIT_Chnl_1
    #define PIT_SOURCE_CLOCK CLOCK_GetFreq(kCLOCK_BusClk)

// Functions:

    void InitKeyScanner(void);

#endif
