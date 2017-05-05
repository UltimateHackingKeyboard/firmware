#ifndef __SLAVE_DRIVER_UHK_MODULE_H__
#define __SLAVE_DRIVER_UHK_MODULE_H__

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
        UhkModuleField_SendDisableKeyMatrixScanState,
        UhkModuleField_SendLedPwmBrightness,
        UhkModuleField_DisableLedSdb,
    } uhk_module_field_t;

    typedef struct {
        uint8_t ledPwmBrightness;
        bool isTestLedOn;
    } uhk_module_state_t;

// Variables:

    extern uhk_module_state_t UhkModuleStates[UHK_MODULE_MAX_COUNT];

// Functions:

    extern void UhkModuleSlaveDriver_Init();
    extern void UhkModuleSlaveDriver_Update(uint8_t uhkModuleId);

#endif
