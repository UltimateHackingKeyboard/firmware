#ifndef __SLAVE_DRIVER_UHK_MODULE_H__
#define __SLAVE_DRIVER_UHK_MODULE_H__

// Includes:

#ifndef __ZEPHYR__
    #include "fsl_common.h"
#endif
    #include "crc16.h"
    #include "slave_scheduler.h"
    #include "slave_protocol.h"
    #include "versioning.h"
    #include "slot.h"

// Macros:

    #define UHK_MODULE_MAX_SLOT_COUNT (SLOT_COUNT-1)
    #define MAX_PWM_BRIGHTNESS 0x64

    #define MAX_STRING_PROPERTY_LENGTH 63

    #define MODULE_CONNECTION_TIMEOUT 350

// Typedefs:

    typedef enum {
        UhkModuleDriverId_LeftKeyboardHalf,
        UhkModuleDriverId_LeftModule,
        UhkModuleDriverId_RightModule,
    } uhk_module_id_t;

    typedef enum {

        // Sync communication
        UhkModulePhase_RequestSync = 0,
        UhkModulePhase_ReceiveSync = 1,
        UhkModulePhase_ProcessSync = 2,

        // Get protocol version
        UhkModulePhase_RequestModuleProtocolVersion = 3,
        UhkModulePhase_ReceiveModuleProtocolVersion = 4,
        UhkModulePhase_ProcessModuleProtocolVersion = 5,

        // Get firmware version
        UhkModulePhase_RequestFirmwareVersion = 6,
        UhkModulePhase_ReceiveFirmwareVersion = 7,
        UhkModulePhase_ProcessFirmwareVersion = 8,

        // Get module id
        UhkModulePhase_RequestModuleId = 9,
        UhkModulePhase_ReceiveModuleId = 10,
        UhkModulePhase_ProcessModuleId = 11,

        // Get module key count
        UhkModulePhase_RequestModuleKeyCount = 12,
        UhkModulePhase_ReceiveModuleKeyCount = 13,
        UhkModulePhase_ProcessModuleKeyCount = 14,

        // Get module key count
        UhkModulePhase_RequestModulePointerCount = 15,
        UhkModulePhase_ReceiveModulePointerCount = 16,
        UhkModulePhase_ProcessModulePointerCount = 17,

        // Get key states
        UhkModulePhase_RequestKeyStates = 18,
        UhkModulePhase_ReceiveKeystates = 19,
        UhkModulePhase_ProcessKeystates = 20,

        // Get git tag
        UhkModulePhase_RequestGitTag = 21,
        UhkModulePhase_ReceiveGitTag = 22,
        UhkModulePhase_ProcessGitTag = 23,

        // Get git repo
        UhkModulePhase_RequestGitRepo = 24,
        UhkModulePhase_ReceiveGitRepo = 25,
        UhkModulePhase_ProcessGitRepo = 26,

        // Get firmware checksum
        UhkModulePhase_RequestFirmwareChecksum = 27,
        UhkModulePhase_ReceiveFirmwareChecksum = 28,
        UhkModulePhase_ProcessFirmwareChecksum = 29,

        // Misc phases
        UhkModulePhase_SetTestLed = 30,
        UhkModulePhase_SetLedPwmBrightness = 31,
        UhkModulePhase_JumpToBootloader = 32,
        UhkModulePhase_ResetTrackpoint = 33,

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
        char firmwareChecksum[MD5_CHECKSUM_LENGTH];
    } uhk_module_state_t;

    typedef struct {
        uint8_t firmwareI2cAddress;
        uint8_t bootloaderI2cAddress;
    } uhk_module_i2c_addresses_t;

    typedef struct {
        uint8_t moduleId;
        uint32_t lastTimeConnected;
    } module_connection_state_t;

// Variables:

    extern bool UhkModuleDriver_ResendKeyStates;
    extern uhk_module_state_t UhkModuleStates[UHK_MODULE_MAX_SLOT_COUNT];
    extern module_connection_state_t ModuleConnectionStates[UHK_MODULE_MAX_SLOT_COUNT];

// Functions:
    uint8_t UhkModuleSlaveDriver_SlotIdToDriverId(uint8_t slotId);
    uint8_t UhkModuleSlaveDriver_DriverIdToSlotId(uint8_t uhkModuleDriverId);
    void UhkModuleSlaveDriver_SendTrackpointCommand(module_specific_command_t command);

    void UhkModuleSlaveDriver_Init(uint8_t uhkModuleDriverId);
    slave_result_t UhkModuleSlaveDriver_Update(uint8_t uhkModuleDriverId);
    void UhkModuleSlaveDriver_Disconnect(uint8_t uhkModuleDriverId);

    void UhkModuleSlaveDriver_ProcessKeystates(uint8_t uhkModuleDriverId, uhk_module_state_t* uhkModuleState, const uint8_t* rxMessageData);
    void UhkModuleSlaveDriver_UpdateConnectionStatus();

    i2c_message_t* UhkModuleDriver_GetTxMessage();
#endif
