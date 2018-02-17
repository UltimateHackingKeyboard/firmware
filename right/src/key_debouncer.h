#ifndef __KEY_DEBOUNCER_H__
#define __KEY_DEBOUNCER_H__

// Includes:

    #include "peripherals/pit.h"
    #include "fsl_common.h"

// Macros:

    #define KEY_DEBOUNCER_INTERVAL_MSEC 1
    #define KEY_DEBOUNCER_TIMEOUT_MSEC 20

// Functions:

    void InitKeyDebouncer(void);

#endif
