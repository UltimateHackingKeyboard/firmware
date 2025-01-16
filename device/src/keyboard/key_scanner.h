#ifndef __KEY_SCANNER_H__
#define __KEY_SCANNER_H__

// Includes:

    #include "device.h"
    #include "stdint.h"
    #include "stdbool.h"

// Variables:

    extern volatile bool KeyPressed;
    extern volatile bool KeyScanner_ResendKeyStates;

// Functions:

    extern void InitKeyScanner(void);

#endif // KEY_SCANNER_H__
