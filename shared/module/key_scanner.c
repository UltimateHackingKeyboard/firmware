#include "fsl_lptmr.h"
#include "key_scanner.h"
#include "module/i2c_watchdog.h"

void KEY_SCANNER_HANDLER(void)
{
    #if KEY_ARRAY_TYPE == KEY_ARRAY_TYPE_VECTOR
        KeyVector_Scan(&keyVector);
    #elif KEY_ARRAY_TYPE == KEY_ARRAY_TYPE_MATRIX
        KeyMatrix_ScanRow(&keyMatrix);
    #endif
    RunWatchdog();
    LPTMR_ClearStatusFlags(KEY_SCANNER_LPTMR_BASEADDR, kLPTMR_TimerCompareFlag);
}

void InitKeyScanner(void)
{
    lptmr_config_t lptmrConfig;
    LPTMR_GetDefaultConfig(&lptmrConfig);
    LPTMR_Init(KEY_SCANNER_LPTMR_BASEADDR, &lptmrConfig);

    #if KEY_ARRAY_TYPE == KEY_ARRAY_TYPE_MATRIX
        uint8_t scanCountPerUsec = KEYBOARD_MATRIX_ROWS_NUM;
    #else
        uint8_t scanCountPerUsec = 1;
    #endif
    LPTMR_SetTimerPeriod(KEY_SCANNER_LPTMR_BASEADDR, USEC_TO_COUNT(1000 / scanCountPerUsec, LPTMR_SOURCE_CLOCK));
    LPTMR_EnableInterrupts(KEY_SCANNER_LPTMR_BASEADDR, kLPTMR_TimerInterruptEnable);
    EnableIRQ(KEY_SCANNER_LPTMR_IRQ_ID);
    LPTMR_StartTimer(KEY_SCANNER_LPTMR_BASEADDR);
}
