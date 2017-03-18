#ifndef __BRIDGE_SLAVE_UHK_MODULE_H__
#define __BRIDGE_SLAVE_UHK_MODULE_H__

// Includes:

    #include "fsl_common.h"

// Macros:

    #define UHK_MODULE_MAX_COUNT 1

// Typedefs:

    typedef enum {
        UhkModuleField_SendKeystatesRequestCommand,
        UhkModuleField_ReceiveKeystates,
        UhkModuleField_SendPwmBrightnessCommand,
        UhkModuleField_SendTestLedCommand,
    } uhk_module_field_t;

    typedef struct {
        uint8_t ledPwmBrightness;
        bool isTestLedOn;
    } uhk_module_state_t;

// Variables:

    extern uhk_module_state_t UhkModuleStates[UHK_MODULE_MAX_COUNT];

// Functions:

    extern bool BridgeSlaveUhkModuleHandler(uint8_t uhkModuleId);

#endif
