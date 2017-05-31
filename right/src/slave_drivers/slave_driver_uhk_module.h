#ifndef __SLAVE_DRIVER_UHK_MODULE_H__
#define __SLAVE_DRIVER_UHK_MODULE_H__

// Includes:

    #include "fsl_common.h"
    #include "crc16.h"

// Macros:

    #define UHK_MODULE_MAX_COUNT 1
    #define KEY_STATE_SIZE (LEFT_KEYBOARD_HALF_KEY_COUNT/8 + 1)
    #define KEY_STATE_BUFFER_SIZE (KEY_STATE_SIZE + CRC16_HASH_LENGTH)

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
