#ifndef __BRIDGE_PROTOCOL__
#define __BRIDGE_PROTOCOL__

// Macros:

    typedef enum {
        BridgeCommand_GetKeyStates,
        BridgeCommand_SetTestLed,
        BridgeCommand_SetLedPwmBrightness,
    } bridge_command_t;

#endif
