#ifndef __INPUT_INTERCEPTOR_H__
#define __INPUT_INTERCEPTOR_H__

// Includes:

    #include <inttypes.h>
    #include <stdbool.h>
    #include "hid/keyboard_report.h"

// Macros:

// Typedefs:

// Variables:


// Functions:

    bool InputInterceptor_RegisterReport(hid_keyboard_report_t* activeReport);

#endif
