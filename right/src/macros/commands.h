#ifndef __MACROS_COMMANDS_H__
#define __MACROS_COMMANDS_H__

// Includes:

    #include <stdint.h>
    #include <stdbool.h>
    #include "attributes.h"
    #include "macros/typedefs.h"
    #include "str_utils.h"

// Macros:

// Typedefs:

// Variables:

// Functions:

    macro_result_t Macros_ProcessCommandAction(void);
    macro_result_t Macros_ProcessDelayAction();

#endif
