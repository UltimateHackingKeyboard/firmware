#ifndef __SLAVE_DRIVER_LED_DRIVER_H__
#define __SLAVE_DRIVER_LED_DRIVER_H__

// Includes:

    #include "fsl_common.h"
    #include "led_driver.h"

// Macros:

    #define LED_DRIVER_MAX_COUNT 2
    #define BUFFER_SIZE (LED_DRIVER_LED_COUNT + 1)
    #define LED_CONTROL_REGISTERS_COMMAND_LENGTH 19
    #define PMW_REGISTER_UPDATE_CHUNK_SIZE 8
    #define PWM_REGISTER_BUFFER_LENGTH (1 + PMW_REGISTER_UPDATE_CHUNK_SIZE)

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
        LedDriverPhase_Initialized,
    } led_driver_phase_t;

    typedef struct {
        led_driver_phase_t phase;
        uint8_t ledValues[LED_DRIVER_LED_COUNT];
        uint8_t ledIndex;
        uint8_t i2cAddress;
        uint8_t setupLedControlRegistersCommand[LED_CONTROL_REGISTERS_COMMAND_LENGTH];
    } led_driver_state_t;

// Functions:

    extern void LedSlaveDriver_Init(uint8_t ledDriverId);
    extern void LedSlaveDriver_Update(uint8_t ledDriverId);
    extern void SetLeds(uint8_t ledBrightness);

#endif
