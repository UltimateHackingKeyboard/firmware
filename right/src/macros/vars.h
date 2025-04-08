#ifndef __MACROS_VARS_H__
#define __MACROS_VARS_H__


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
        MacroVariableType_Int,
        MacroVariableType_Float,
        MacroVariableType_Bool,
        MacroVariableType_None,
    } macro_variable_type_t;

    typedef struct {
        union {
            int32_t asInt;
            float asFloat;
            bool asBool;
        };
        string_ref_t name;
        macro_variable_type_t type;
    } ATTR_PACKED macro_variable_t;


// Variables:

// Functions:

    void MacroVariables_Reset(void);
    macro_result_t Macros_ProcessStatsVariablesCommand(void);
    macro_result_t Macros_ProcessSetVarCommand(parser_context_t* ctx);
    macro_variable_t* Macros_ConsumeExistingWritableVariable(parser_context_t* ctx);
    int32_t Macros_ConsumeInt(parser_context_t* ctx);
    float Macros_ConsumeFloat(parser_context_t* ctx);
    bool Macros_ConsumeBool(parser_context_t* ctx);
    macro_variable_t Macros_ConsumeAnyValue(parser_context_t* ctx);
    void MacroVariables_RunTests(void);

#endif

