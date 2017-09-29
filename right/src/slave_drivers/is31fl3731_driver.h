#ifndef __SLAVE_DRIVER_LED_DRIVER_H__
#define __SLAVE_DRIVER_LED_DRIVER_H__

// Includes:

    #include "fsl_common.h"
    #include "peripherals/led_driver.h"

// Macros:

    #define LED_DRIVER_MAX_COUNT 2
    #define LED_CONTROL_REGISTERS_COMMAND_LENGTH 19
    #define PMW_REGISTER_UPDATE_CHUNK_SIZE 8
    #define PWM_REGISTER_BUFFER_LENGTH (1 + PMW_REGISTER_UPDATE_CHUNK_SIZE)
    #define LED_BRIGHTNESS_LEVEL 0xff

    #define IS_ISO true
    #define ISO_KEY_LED_DRIVER_ID LedDriverId_Left
    #define ISO_KEY_CONTROL_REGISTER_POS 7
    #define ISO_KEY_CONTROL_REGISTER_BIT 1

// Typedefs:

    typedef enum {
        LedDriverId_Right,
        LedDriverId_Left,
    } led_driver_id_t;

    typedef enum {
        LedDriverPhase_SetFunctionFrame,
        LedDriverPhase_SetShutdownModeNormal,
        LedDriverPhase_SetFrame1,
        LedDriverPhase_InitLedControlRegisters,
        LedDriverPhase_InitLedValues,
        LedDriverPhase_UpdateChangedLedValues,
    } led_driver_phase_t;

    typedef struct {
        led_driver_phase_t phase;
        uint8_t targetLedValues[LED_DRIVER_LED_COUNT];
        uint8_t ledIndex;
        uint8_t i2cAddress;
        uint8_t setupLedControlRegistersCommand[LED_CONTROL_REGISTERS_COMMAND_LENGTH];
    } led_driver_state_t;

// Variables:

    extern uint8_t LedDriverValues[LED_DRIVER_MAX_COUNT][LED_DRIVER_LED_COUNT];

// Functions:

    void LedSlaveDriver_Init(uint8_t ledDriverId);
    status_t LedSlaveDriver_Update(uint8_t ledDriverId);

#endif
