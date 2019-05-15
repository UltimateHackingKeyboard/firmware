#include "fsl_lptmr.h"
#include "key_scanner.h"
#include "i2c_watchdog.h"

void KEY_SCANNER_HANDLER(void)
{
    KeyMatrix_ScanRow(&keyMatrix);
    RunWatchdog();
    LPTMR_ClearStatusFlags(KEY_SCANNER_LPTMR_BASEADDR, kLPTMR_TimerCompareFlag);
}

void InitKeyScanner(void)
{
    lptmr_config_t lptmrConfig;
    LPTMR_GetDefaultConfig(&lptmrConfig);
    LPTMR_Init(KEY_SCANNER_LPTMR_BASEADDR, &lptmrConfig);

    LPTMR_SetTimerPeriod(KEY_SCANNER_LPTMR_BASEADDR, USEC_TO_COUNT(KEY_SCANNER_INTERVAL_USEC, LPTMR_SOURCE_CLOCK));
    LPTMR_EnableInterrupts(KEY_SCANNER_LPTMR_BASEADDR, kLPTMR_TimerInterruptEnable);
    EnableIRQ(KEY_SCANNER_LPTMR_IRQ_ID);
    LPTMR_StartTimer(KEY_SCANNER_LPTMR_BASEADDR);
}
