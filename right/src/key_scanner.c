#include "fsl_pit.h"
#include "key_scanner.h"

uint32_t KeyScannerCounter;

void PIT_KEY_SCANNER_HANDLER(void)
{
    KeyMatrix_ScanRow(&RightKeyMatrix);
    KeyScannerCounter++;
    PIT_ClearStatusFlags(PIT, PIT_KEY_SCANNER_CHANNEL, PIT_TFLG_TIF_MASK);
}

void InitKeyScanner(void)
{
    pit_config_t pitConfig;
    PIT_GetDefaultConfig(&pitConfig);
    PIT_Init(PIT, &pitConfig);
    PIT_SetTimerPeriod(PIT, PIT_KEY_SCANNER_CHANNEL, USEC_TO_COUNT(KEY_SCANNER_INTERVAL_USEC, PIT_SOURCE_CLOCK));
    PIT_EnableInterrupts(PIT, PIT_KEY_SCANNER_CHANNEL, kPIT_TimerInterruptEnable);
    EnableIRQ(PIT_KEY_SCANNER_IRQ_ID);
    PIT_StartTimer(PIT, PIT_KEY_SCANNER_CHANNEL);
}
