#ifndef SRC_MACRO_KEYID_PARSER_H_
#define SRC_MACRO_KEYID_PARSER_H_


// Includes:

    #include <stdint.h>
    #include <stdbool.h>
    #include "macros/core.h"
#include "str_utils.h"

// Typedefs:

    // Functions:

    void KeyIdParser_initialize();
    uint8_t MacroKeyIdParser_TryConsumeKeyId(parser_context_t* ctx);
    const char* MacroKeyIdParser_KeyIdToAbbreviation(uint8_t keyId) ;


#endif
