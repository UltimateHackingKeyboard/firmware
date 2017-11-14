#include "key_debouncer.h"
#include "fsl_pit.h"
#include "slot.h"
#include "module.h"
#include "key_states.h"
#include "peripherals/test_led.h"

void PIT_KEY_DEBOUNCER_HANDLER(void)
{
    TEST_LED_TOGGLE();
    for (uint8_t slotId=0; slotId<SLOT_COUNT; slotId++) {
        for (uint8_t keyId=0; keyId<MAX_KEY_COUNT_PER_MODULE; keyId++) {
            uint8_t *debounceCounter = &KeyStates[slotId][keyId].debounceCounter;
            if (*debounceCounter < 0xff) {
                (*debounceCounter)++;
            }
        }
    }

    PIT_ClearStatusFlags(PIT, PIT_KEY_DEBOUNCER_CHANNEL, PIT_TFLG_TIF_MASK);
}

void InitKeyDebouncer(void)
{
    pit_config_t pitConfig;
    PIT_GetDefaultConfig(&pitConfig);
    PIT_Init(PIT, &pitConfig);
    PIT_SetTimerPeriod(PIT, PIT_KEY_DEBOUNCER_CHANNEL, MSEC_TO_COUNT(KEY_DEBOUNCER_INTERVAL_MSEC, PIT_SOURCE_CLOCK));
    PIT_EnableInterrupts(PIT, PIT_KEY_DEBOUNCER_CHANNEL, kPIT_TimerInterruptEnable);
    EnableIRQ(PIT_KEY_DEBOUNCER_IRQ_ID);
    PIT_StartTimer(PIT, PIT_KEY_DEBOUNCER_CHANNEL);
}
