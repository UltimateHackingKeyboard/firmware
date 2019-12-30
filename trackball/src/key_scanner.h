#ifndef __KEY_SCANNER_H__
#define __KEY_SCANNER_H__

// Includes:

    #include "module.h"

// Macros:

    #define LPTMR_SOURCE_CLOCK CLOCK_GetFreq(kCLOCK_LpoClk)

    #define KEY_SCANNER_LPTMR_BASEADDR LPTMR0
    #define KEY_SCANNER_LPTMR_IRQ_ID   LPTMR0_IRQn
    #define KEY_SCANNER_HANDLER        LPTMR0_IRQHandler

    #define KEY_SCANNER_INTERVAL_USEC (1000 / KEYBOARD_VECTOR_ITEMS_NUM)

// Functions:

    void InitKeyScanner(void);

#endif
