#ifndef __SLAVE_DRIVER_UHK_MODULE_H__
#define __SLAVE_DRIVER_UHK_MODULE_H__

// Includes:

    #include "fsl_common.h"
    #include "crc16.h"

// Macros:

    #define UHK_MODULE_MAX_COUNT 3
    #define MAX_PWM_BRIGHTNESS 0x64

// Typedefs:

    typedef enum {
        UhkModuleDriverId_LeftKeyboardHalf,
        UhkModuleDriverId_LeftAddon,
        UhkModuleDriverId_RightAddon,
    } uhk_module_id_t;

    typedef enum {

        // Sync communication
        UhkModulePhase_RequestSync,
        UhkModulePhase_ReceiveSync,
        UhkModulePhase_ProcessSync,

        // Get protocol version
        UhkModulePhase_RequestProtocolVersion,
        UhkModulePhase_ReceiveProtocolVersion,
        UhkModulePhase_ProcessProtocolVersion,

        // Get module id
        UhkModulePhase_RequestModuleId,
        UhkModulePhase_ReceiveModuleId,
        UhkModulePhase_ProcessModuleId,

        // Get module features
        UhkModulePhase_RequestModuleFeatures,
        UhkModulePhase_ReceiveModuleFeatures,
        UhkModulePhase_ProcessModuleFeatures,

        // Get key states
        UhkModulePhase_RequestKeyStates,
        UhkModulePhase_ReceiveKeystates,
        UhkModulePhase_ProcessKeystates,

        // Misc phases
        UhkModulePhase_JumpToBootloader,
        UhkModulePhase_SetTestLed,
        UhkModulePhase_SetLedPwmBrightness,

    } uhk_module_phase_t;

    typedef struct {
        uint8_t ledPwmBrightness;
        bool isTestLedOn;
    } uhk_module_vars_t;

    typedef struct {
        uint8_t moduleId;
        uint8_t protocolVersion;
        uhk_module_phase_t phase;
        uhk_module_vars_t sourceVars;
        uhk_module_vars_t targetVars;
        i2c_message_t rxMessage;
        uint8_t firmwareI2cAddress;
        uint8_t bootloaderI2cAddress;
        uhk_module_features_t features;
        bool jumpToBootloader;
    } uhk_module_state_t;

    typedef struct {
        uint8_t firmwareI2cAddress;
        uint8_t bootloaderI2cAddress;
    } uhk_module_i2c_addresses_t;

// Variables:

    extern uhk_module_state_t UhkModuleStates[UHK_MODULE_MAX_COUNT];

// Functions:

    void UhkModuleSlaveDriver_Init(uint8_t uhkModuleDriverId);
    status_t UhkModuleSlaveDriver_Update(uint8_t uhkModuleDriverId);
    void UhkModuleSlaveDriver_Disconnect(uint8_t uhkModuleDriverId);

#endif
