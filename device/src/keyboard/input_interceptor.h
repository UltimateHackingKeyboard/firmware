#ifndef __INPUT_INTERCEPTOR_H__
#define __INPUT_INTERCEPTOR_H__

// Includes:

    #include <inttypes.h>
    #include <stdbool.h>
    #include "legacy/usb_interfaces/usb_interface_basic_keyboard.h"

// Macros:

// Typedefs:

// Variables:


// Functions:

    bool InputInterceptor_RegisterReport(usb_basic_keyboard_report_t* activeReport);

#endif
