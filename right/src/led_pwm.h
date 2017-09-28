#ifndef __LED_PWM_H__
#define __LED_PWM_H__

// Includes:

    #include "fsl_ftm.h"

// Macros:

    #define LED_PWM_PORT  PORTD
    #define LED_PWM_CLOCK kCLOCK_PortD
    #define LED_PWM_PIN   6

    #define LED_PWM_FTM_BASEADDR FTM0
    #define LED_PWM_FTM_CHANNEL kFTM_Chnl_6

    #define FTM_SOURCE_CLOCK CLOCK_GetFreq(kCLOCK_BusClk)
    #define FTM_PWM_FREQUENCY 24000

    #define INITIAL_DUTY_CYCLE_PERCENT 100

// Functions:

    void LedPwm_Init(void);
    void LedPwm_SetBrightness(uint8_t brightnessPercent);

#endif
