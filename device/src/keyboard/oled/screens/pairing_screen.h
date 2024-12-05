#ifndef __PAIRING_SCREEN_H__
#define __PAIRING_SCREEN_H__

// Includes:

    #include <inttypes.h>
    #include <stdbool.h>
    #include "../widgets/widget.h"
    #include "legacy/key_action.h"

// Macros:

    #define PASSWORD_LENGTH 6

// Typedefs:

// Variables:

    extern widget_t* PairingScreen;
    extern widget_t* PairingSucceededScreen;
    extern widget_t* PairingFailedScreen;

// Functions:

    void PairingScreen_Init();
    void PairingScreen_RegisterScancode(uint8_t scancode);
    void PairingScreen_AskForPassword(unsigned int pass);
    const rgb_t* PairingScreen_ActionColor(key_action_t* action);

#endif
