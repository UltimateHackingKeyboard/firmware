#ifndef __PAIRING_SCREEN_H__
#define __PAIRING_SCREEN_H__

// Includes:

    #include <inttypes.h>
    #include <stdbool.h>
    #include "../widgets/widget.h"
    #include "key_action.h"

// Macros:

    #define PASSWORD_LENGTH 6

// Typedefs:

// Variables:

    extern widget_t* PairingScreen;

// Functions:

    void PairingScreen_Init(void);
    void PairingScreen_RegisterScancode(uint8_t scancode);
    void PairingScreen_AskForPassword(void);
    void PairingScreen_Feedback(bool success);
    void PairingScreen_Cancel(void);
    const rgb_t* PairingScreen_ActionColor(key_action_t* action);

#endif
