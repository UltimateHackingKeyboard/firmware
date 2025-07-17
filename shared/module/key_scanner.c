#include "fsl_lptmr.h"
#include "key_scanner.h"
#include "module/i2c_watchdog.h"
#include "module.h"

#if defined(DEVICE_ID)
#include "trace.h"
#else
#define Trace_Printc(...)
#endif

void KEY_SCANNER_HANDLER(void)
{

    Trace_Printc("<i2");
    #if KEY_ARRAY_TYPE == KEY_ARRAY_TYPE_VECTOR
        KeyVector_Scan(&KeyVector);
    #elif KEY_ARRAY_TYPE == KEY_ARRAY_TYPE_MATRIX
        KeyMatrix_ScanRow(&KeyMatrix);
    #endif
    RunWatchdog();
    Module_OnScan();
    LPTMR_ClearStatusFlags(KEY_SCANNER_LPTMR_BASEADDR, kLPTMR_TimerCompareFlag);
    Trace_Printc(">");
}

void InitKeyScanner(void)
{
    lptmr_config_t lptmrConfig;
    LPTMR_GetDefaultConfig(&lptmrConfig);
    LPTMR_Init(KEY_SCANNER_LPTMR_BASEADDR, &lptmrConfig);

    // LPTMR is clocked by 1ms fixed clock
    LPTMR_SetTimerPeriod(KEY_SCANNER_LPTMR_BASEADDR, 1);
    LPTMR_EnableInterrupts(KEY_SCANNER_LPTMR_BASEADDR, kLPTMR_TimerInterruptEnable);
    EnableIRQ(KEY_SCANNER_LPTMR_IRQ_ID);
    LPTMR_StartTimer(KEY_SCANNER_LPTMR_BASEADDR);
}
