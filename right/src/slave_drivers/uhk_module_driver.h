#ifndef __SLAVE_DRIVER_UHK_MODULE_H__
#define __SLAVE_DRIVER_UHK_MODULE_H__

// Includes:

    #include "fsl_common.h"
    #include "crc16.h"
    #include "versions.h"
    #include "slot.h"
    #include "usb_interfaces/usb_interface_mouse.h"

// Macros:

    #define UHK_MODULE_MAX_SLOT_COUNT (SLOT_COUNT-1)
    #define MAX_PWM_BRIGHTNESS 0x64

    #define MAX_STRING_PROPERTY_LENGTH 63

// Typedefs:

    typedef enum {
        UhkModuleDriverId_LeftKeyboardHalf,
        UhkModuleDriverId_LeftModule,
        UhkModuleDriverId_RightModule,
    } uhk_module_id_t;

    typedef enum {

        // Sync communication
        UhkModulePhase_RequestSync,
        UhkModulePhase_ReceiveSync,
        UhkModulePhase_ProcessSync,

        // Get protocol version
        UhkModulePhase_RequestModuleProtocolVersion,
        UhkModulePhase_ReceiveModuleProtocolVersion,
        UhkModulePhase_ProcessModuleProtocolVersion,

        // Get firmware version
        UhkModulePhase_RequestFirmwareVersion,
        UhkModulePhase_ReceiveFirmwareVersion,
        UhkModulePhase_ProcessFirmwareVersion,

        // Get module id
        UhkModulePhase_RequestModuleId,
        UhkModulePhase_ReceiveModuleId,
        UhkModulePhase_ProcessModuleId,

        // Get module key count
        UhkModulePhase_RequestModuleKeyCount,
        UhkModulePhase_ReceiveModuleKeyCount,
        UhkModulePhase_ProcessModuleKeyCount,

        // Get module key count
        UhkModulePhase_RequestModulePointerCount,
        UhkModulePhase_ReceiveModulePointerCount,
        UhkModulePhase_ProcessModulePointerCount,

        // Get key states
        UhkModulePhase_RequestKeyStates,
        UhkModulePhase_ReceiveKeystates,
        UhkModulePhase_ProcessKeystates,

        // Get git tag
        UhkModulePhase_RequestGitTag,
        UhkModulePhase_ReceiveGitTag,
        UhkModulePhase_ProcessGitTag,

        // Get git repo
        UhkModulePhase_RequestGitRepo,
        UhkModulePhase_ReceiveGitRepo,
        UhkModulePhase_ProcessGitRepo,

        // Misc phases
        UhkModulePhase_SetTestLed,
        UhkModulePhase_SetLedPwmBrightness,
        UhkModulePhase_JumpToBootloader,

    } uhk_module_phase_t;

    typedef struct {
        uint8_t ledPwmBrightness;
        bool isTestLedOn;
    } uhk_module_vars_t;

    typedef struct {
        uint8_t moduleId;
        version_t moduleProtocolVersion;
        version_t firmwareVersion;
        uhk_module_phase_t phase;
        uhk_module_vars_t sourceVars;
        uhk_module_vars_t targetVars;
        i2c_message_t rxMessage;
        uint8_t firmwareI2cAddress;
        uint8_t bootloaderI2cAddress;
        uint8_t keyCount;
        uint8_t pointerCount;
        pointer_delta_t pointerDelta;
        char gitRepo[MAX_STRING_PROPERTY_LENGTH];
        char gitTag[MAX_STRING_PROPERTY_LENGTH];
    } uhk_module_state_t;

    typedef struct {
        uint8_t firmwareI2cAddress;
        uint8_t bootloaderI2cAddress;
    } uhk_module_i2c_addresses_t;

// Variables:

    extern uhk_module_state_t UhkModuleStates[UHK_MODULE_MAX_SLOT_COUNT];

// Functions:

    uint8_t UhkModuleSlaveDriver_SlotIdToDriverId(uint8_t slotId);
    uint8_t UhkModuleSlaveDriver_DriverIdToSlotId(uint8_t uhkModuleDriverId);

    void UhkModuleSlaveDriver_Init(uint8_t uhkModuleDriverId);
    status_t UhkModuleSlaveDriver_Update(uint8_t uhkModuleDriverId);
    void UhkModuleSlaveDriver_Disconnect(uint8_t uhkModuleDriverId);

#endif
