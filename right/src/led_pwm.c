#include "led_pwm.h"
#include "fsl_port.h"

void LedPwm_Init() {
    ftm_config_t ftmInfo;
    ftm_chnl_pwm_signal_param_t ftmParam[] = {{
            .chnlNumber = LED_PWM_FTM_CHANNEL,
            .level = kFTM_LowTrue,
            .dutyCyclePercent = 00,
            .firstEdgeDelayPercent = 0
    }};

    CLOCK_EnableClock(LED_PWM_CLOCK);
    PORT_SetPinMux(LED_PWM_PORT, LED_PWM_PIN, kPORT_MuxAlt4);

    ftmParam[0].chnlNumber = (ftm_chnl_t)LED_PWM_FTM_CHANNEL;
    ftmParam[0].level = kFTM_LowTrue;
    ftmParam[0].dutyCyclePercent = 100 - INITIAL_DUTY_CYCLE_PERCENT;
    ftmParam[0].firstEdgeDelayPercent = 0;
    FTM_GetDefaultConfig(&ftmInfo);

    // Initializes the FTM module.
    FTM_Init(LED_PWM_FTM_BASEADDR, &ftmInfo);
    FTM_SetupPwm(LED_PWM_FTM_BASEADDR, ftmParam, sizeof(ftmParam),
                 kFTM_EdgeAlignedPwm, FTM_PWM_FREQUENCY, FTM_SOURCE_CLOCK);
    FTM_StartTimer(LED_PWM_FTM_BASEADDR, kFTM_SystemClock);
}

void LedPwm_SetBrightness(uint8_t brightnessPercent)
{
    FTM_UpdatePwmDutycycle(LED_PWM_FTM_BASEADDR, LED_PWM_FTM_CHANNEL, kFTM_EdgeAlignedPwm, 100 - brightnessPercent);
    FTM_SetSoftwareTrigger(LED_PWM_FTM_BASEADDR, true);  // Triggers register update.
}
