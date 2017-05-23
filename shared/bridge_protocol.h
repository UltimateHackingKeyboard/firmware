#ifndef __BRIDGE_PROTOCOL_H__
#define __BRIDGE_PROTOCOL_H__

// Typedefs:

    typedef enum {
        BridgeCommand_GetKeyStates,
        BridgeCommand_SetTestLed,
        BridgeCommand_SetLedPwmBrightness,
        BridgeCommand_SetDisableKeyMatrixScanState,
        BridgeCommand_SetDisableLedSdb,
    } bridge_command_t;

#endif
