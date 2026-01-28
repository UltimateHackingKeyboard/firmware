#ifndef __MACROS_DISPLAY_H__
#define __MACROS_DISPLAY_H__


// Includes:

    #include <stdint.h>
    #include <stdbool.h>
    #include "attributes.h"
    #include "str_utils.h"
    #include "macros/typedefs.h"
    #include "device.h"

// Macros:

#if DEVICE_IS_UHK80
#define BY_DEVICE(A, B) (B)
#else
#define BY_DEVICE(A, B) (A)
#endif


// Typedefs:

    typedef struct {
        char notify[BY_DEVICE(1, 32)];
        char leftStatus[BY_DEVICE(1, 16)];
        char rightStatus[BY_DEVICE(1, 16)];
        char keymap[BY_DEVICE(1, 16)];
        char layer[BY_DEVICE(1, 16)];
        char host[BY_DEVICE(1, 16)];
        char abbrev[BY_DEVICE(4, 4)];
    } display_strings_buffs_t;

// Variables:

    extern display_strings_buffs_t Macros_DisplayStringsBuffs;

// Functions:

    macro_result_t Macros_ProcessSetLedTxtCommand(parser_context_t* ctx);
    macro_result_t Macros_ProcessNotifyCommand(parser_context_t* ctx);

#endif

