#ifndef __LED_PWM_H__
#define __LED_PWM_H__

// Includes:

    #include "fsl_tpm.h"

// Macros:

    #define LED_PWM_PORT  PORTB
    #define LED_PWM_CLOCK kCLOCK_PortB
    #define LED_PWM_PIN   5

    #define LED_PWM_TPM_BASEADDR TPM1
    #define LED_PWM_TPM_CHANNEL kTPM_Chnl_1

    #define TPM_SOURCE_CLOCK CLOCK_GetFreq(kCLOCK_BusClk)
    #define TPM_PWM_FREQUENCY 24000U

    #define INITIAL_DUTY_CYCLE_PERCENT 100U

// Functions:

    void LedPwm_Init(void);
    void LedPwm_SetBrightness(uint8_t brightnessPercent);

#endif
