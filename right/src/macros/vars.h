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
    #define TRY_EXPAND_TEMPLATE(CTX) (*ctx->at == '&' && TryExpandMacroTemplateOnce(CTX))

    #define MACRO_ARGUMENT_POOL_SIZE 32

// Typedefs:

    typedef enum {
        MacroVariableType_Int,
        MacroVariableType_Float,
        MacroVariableType_Bool,
        MacroVariableType_String,
        MacroVariableType_None,
    } macro_variable_type_t;

    typedef struct {
        union {
            int32_t asInt;
            float asFloat;
            bool asBool;
            string_ref_t asStringRef;
        };
        string_ref_t name;
        macro_variable_type_t type;
    } ATTR_PACKED macro_variable_t;

    typedef enum {
        MacroArgType_Unused = 0,
        MacroArgType_Any,
        MacroArgType_Int,
        MacroArgType_Float,
        MacroArgType_Bool,
        MacroArgType_String,
        MacroArgType_KeyId,
        MacroArgType_ScanCode
    } macro_argument_type_t;

    typedef struct {
        uint8_t owner;          // MACRO_STATE_SLOT() of the macro that owns this argument
        macro_argument_type_t type;
        uint8_t idx;            // index of the argument in the macro's argument list (1-based)
                                // (we could always calculate idx by looping through the pool, 
                                //  but returning argument+index separately everywhere becomes 
                                //  a nightmare...)
        string_ref_t name;      // macro argument name (identifier)
    } macro_argument_t;

    typedef enum {
        MacroArgAllocResult_Success,
        MacroArgAllocResult_PoolLimitExceeded,
        MacroArgAllocResult_DuplicateArgumentName,
    } macro_argument_alloc_result_t;

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
    void Macros_SerializeVar(char* buffer, uint8_t len, macro_variable_t var);
    bool TryExpandMacroTemplateOnce(parser_context_t* ctx);

    macro_argument_alloc_result_t Macros_AllocateMacroArgument(uint8_t owner, const char *idStart, const char *idEnd, macro_argument_type_t type, uint8_t argNumber);
    void Macros_DeallocateMacroArgumentsByOwner(uint8_t owner);
    uint8_t Macros_CountMacroArgumentsByOwner(uint8_t owner);
    macro_argument_t *Macros_FindMacroArgumentByName(uint8_t owner, const char *nameStart, const char *nameEnd);
    uint8_t Macros_FindMacroArgumentIndexByName(uint8_t owner, const char *nameStart, const char *nameEnd);
    macro_argument_t *Macros_FindMacroArgumentByIndex(uint8_t owner, uint8_t argIndex);

#endif
