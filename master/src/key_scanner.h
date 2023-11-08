#ifndef __KEY_SCANNER_H__
#define __KEY_SCANNER_H__

// Variables:

    extern uint8_t KeyStates[6][10];
    extern volatile char KeyPressed;

// Functions:

    extern void InitKeyScanner(void);

#endif
