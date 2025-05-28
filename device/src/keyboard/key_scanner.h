#ifndef __KEY_SCANNER_H__
#define __KEY_SCANNER_H__

// Includes:

    #include "device.h"
    #include "stdint.h"
    #include "stdbool.h"

// Typedefs:

    typedef enum {
        SfjlScanResult_NonePressed,
        SfjlScanResult_SomethingPressed,
        SfjlScanResult_FullMatch
    } sfjl_scan_result_t;

// Variables:


    extern volatile bool KeyPressed;
    extern volatile bool KeyScanner_ResendKeyStates;

// Functions:

    extern void InitKeyScanner(void);
    extern void InitKeyScanner_Min(void);
    extern bool KeyScanner_ScanAndWakeOnSfjl(bool fullScan, bool wake);

#endif // KEY_SCANNER_H__
