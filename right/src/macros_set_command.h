#ifndef __MACRO_SET_COMMAND_H__
#define __MACRO_SET_COMMAND_H__

// Includes:

    #include <stdint.h>
    #include <stdbool.h>
    #include "key_action.h"
    #include "usb_device_config.h"
    #include "key_states.h"
    #include "macros_typedefs.h"
    #include "macros_vars.h"
    #include "macros.h"

// Macros:

// Typedefs:

// Variables:

// Functions:
    macro_variable_t Macro_TryReadConfigVal(parser_context_t* ctx);
    macro_result_t Macro_ProcessSetCommand(parser_context_t* ctx);

#endif /* __MACRO_SET_COMMAND_H__ */
