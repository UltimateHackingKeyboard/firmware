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

typedef enum {
    StrReadMode_Verbatim, // do not expand anything in the string. Reads until end of context.
    StrReadMode_Literal,  // read a string literal, with support for quotes, escapes and $-expansions.
} string_reader_mode_t;

// Variables:

// Functions:

    char Macros_ConsumeCharOfString(parser_context_t* ctx, uint16_t* stringOffset, uint16_t* index, uint16_t* subIndex);
    bool Macros_CompareStringRefs(string_ref_t ref1, string_ref_t ref2);
    bool Macros_CompareStringToken(parser_context_t* ctx, string_segment_t str);
    void Macros_ConsumeStringToken(parser_context_t* ctx);

#endif

