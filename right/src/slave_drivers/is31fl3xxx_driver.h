#ifndef __SLAVE_DRIVER_IS31FL37XX_DRIVER_H__
#define __SLAVE_DRIVER_IS31FL37XX_DRIVER_H__

// Includes:

    #include "fsl_common.h"
    #include "peripherals/led_driver.h"

// Macros:

    #define LED_DRIVER_MAX_COUNT 3

    #define LED_CONTROL_REGISTERS_COMMAND_LENGTH_IS31FL3731 19
    #define LED_CONTROL_REGISTERS_COMMAND_LENGTH_IS31FL3737 25
    #define LED_CONTROL_REGISTERS_COMMAND_LENGTH_MAX MAX(LED_CONTROL_REGISTERS_COMMAND_LENGTH_IS31FL3731, LED_CONTROL_REGISTERS_COMMAND_LENGTH_IS31FL3737)

    #define PMW_REGISTER_UPDATE_CHUNK_SIZE LED_DRIVER_LED_COUNT_IS31FL3737
    #define PWM_REGISTER_BUFFER_LENGTH (1 + PMW_REGISTER_UPDATE_CHUNK_SIZE)

    #define IS_ISO true
    #define ISO_KEY_LED_DRIVER_ID LedDriverId_Left
    #define ISO_KEY_CONTROL_REGISTER_POS 7
    #define ISO_KEY_CONTROL_REGISTER_BIT 1

// Typedefs:

    typedef enum {
        LedDriverIc_IS31FL3199,
        LedDriverIc_IS31FL3731,
        LedDriverIc_IS31FL3737,
    } led_driver_ic_t;

    typedef enum {
        LedDriverId_Right,
        LedDriverId_Left,
        LedDriverId_ModuleLeft,
        LedDriverId_Last = LedDriverId_ModuleLeft,
    } led_driver_id_t;

    typedef enum {
        LedDriverPhase_UnlockCommandRegister1,
        LedDriverPhase_SetFunctionFrame,
        LedDriverPhase_SetShutdownModeNormal,
        LedDriverPhase_SetGlobalCurrent,
        LedDriverPhase_UnlockCommandRegister2,
        LedDriverPhase_SetFrame1,
        LedDriverPhase_InitLedControlRegisters,
        LedDriverPhase_UnlockCommandRegister3,
        LedDriverPhase_SetFrame2,
        LedDriverPhase_InitLedValues,
        LedDriverPhase_UpdateData,
        LedDriverPhase_SetLedBrightness,
        LedDriverPhase_UpdateChangedLedValues,
    } led_driver_phase_t;

    typedef struct {
        led_driver_phase_t phase;
        uint8_t ledCount;
        uint8_t targetLedValues[LED_DRIVER_LED_COUNT_MAX];
        uint8_t ledIndex;
        uint8_t i2cAddress;
        led_driver_ic_t ledDriverIc;
        uint8_t frameRegisterPwmFirst;
        uint8_t setShutdownModeNormalBufferLength;
        uint8_t *setShutdownModeNormalBuffer;
        uint8_t setupLedControlRegistersCommandLength;
        uint8_t setupLedControlRegistersCommand[LED_CONTROL_REGISTERS_COMMAND_LENGTH_MAX];
    } led_driver_state_t;

// Variables:

    extern bool LedsEnabled;
    extern bool LedSleepModeActive;
    extern float LedBrightnessMultiplier;
    extern uint8_t KeyBacklightBrightness;
    extern uint8_t KeyBacklightBrightnessDefault;
    extern uint8_t LedDriverValues[LED_DRIVER_MAX_COUNT][LED_DRIVER_LED_COUNT_MAX];

// Functions:

    void LedSlaveDriver_DisableLeds(void);
    void LedSlaveDriver_UpdateLeds(void);
    void LedSlaveDriver_Init(uint8_t ledDriverId);
    status_t LedSlaveDriver_Update(uint8_t ledDriverId);

#endif
