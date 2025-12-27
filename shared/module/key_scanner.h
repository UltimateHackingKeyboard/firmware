#ifndef __KEY_SCANNER_H__
#define __KEY_SCANNER_H__

// Includes:

    #include "module.h"
    #include <stdint.h>

// Macros:

    #define LPTMR_SOURCE_CLOCK CLOCK_GetFreq(kCLOCK_LpoClk)

    #define KEY_SCANNER_LPTMR_BASEADDR LPTMR0
    #define KEY_SCANNER_LPTMR_IRQ_ID   LPTMR0_IRQn
    #define KEY_SCANNER_HANDLER        LPTMR0_IRQHandler

// Variables:

    extern uint32_t CurrentTime;

// Functions:

    void InitKeyScanner(void);

#endif
