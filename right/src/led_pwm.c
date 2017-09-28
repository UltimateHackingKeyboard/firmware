#include "led_pwm.h"
#include "fsl_port.h"

void LedPwm_Init(void) {
    CLOCK_EnableClock(LED_PWM_CLOCK);
    PORT_SetPinMux(LED_PWM_PORT, LED_PWM_PIN, kPORT_MuxAlt4);

    ftm_config_t ftmInfo;
    FTM_GetDefaultConfig(&ftmInfo);
    FTM_Init(LED_PWM_FTM_BASEADDR, &ftmInfo);

    ftm_chnl_pwm_signal_param_t ftmParam[1];
    ftmParam[0].chnlNumber = LED_PWM_FTM_CHANNEL;
    ftmParam[0].level = kFTM_LowTrue;
    ftmParam[0].dutyCyclePercent = 100 - INITIAL_DUTY_CYCLE_PERCENT;
    ftmParam[0].firstEdgeDelayPercent = 0;
    FTM_SetupPwm(LED_PWM_FTM_BASEADDR, ftmParam, 1,
                 kFTM_EdgeAlignedPwm, FTM_PWM_FREQUENCY, FTM_SOURCE_CLOCK);

    FTM_StartTimer(LED_PWM_FTM_BASEADDR, kFTM_SystemClock);
}

void LedPwm_SetBrightness(uint8_t brightnessPercent)
{
    FTM_UpdatePwmDutycycle(LED_PWM_FTM_BASEADDR, LED_PWM_FTM_CHANNEL,
                           kFTM_EdgeAlignedPwm, 100 - brightnessPercent);
    FTM_SetSoftwareTrigger(LED_PWM_FTM_BASEADDR, true);
}
