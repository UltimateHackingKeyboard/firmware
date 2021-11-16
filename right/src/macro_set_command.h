#ifndef __MACRO_SET_COMMAND_H__
#define __MACRO_SET_COMMAND_H__

// Includes:

    #include <stdint.h>
    #include <stdbool.h>
    #include "key_action.h"
    #include "usb_device_config.h"
    #include "key_states.h"
    #include "macros.h"

// Macros:

// Typedefs:

// Variables:

// Functions:
    macro_result_t MacroSetCommand(const char* text, const char *textEnd);

#endif /* __MACRO_SET_COMMAND_H__ */
