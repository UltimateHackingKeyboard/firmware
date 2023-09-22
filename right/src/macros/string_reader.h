#ifndef __MACROS_STRING_READER_H__
#define __MACROS_STRING_READER_H__


// Includes:

    #include <stdint.h>
    #include <stdbool.h>
    #include "attributes.h"
    #include "key_action.h"
    #include "usb_device_config.h"
    #include "key_states.h"
    #include "str_utils.h"
    #include "macros/typedefs.h"

// Macros:

    #define MACRO_VARIABLE_COUNT_MAX 32

// Typedefs:



// Variables:

// Functions:

    char Macros_ConsumeCharOfString(parser_context_t* ctx, uint16_t* stringOffset, uint16_t* index, uint16_t* subIndex);

#endif

