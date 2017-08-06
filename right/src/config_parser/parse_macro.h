#ifndef __PARSE_MACRO_H__
#define __PARSE_MACRO_H__

// Includes:

    #include "parse_config.h"

// Typedefs:

    typedef enum {
        SerializedMacroActionType_KeyMacroAction = 0,
        SerializedMacroActionType_LastKeyMacroAction = 63,
        SerializedMacroActionType_MouseButtonMacroAction,
        SerializedMacroActionType_LastMouseButtonMacroAction = 66,
        SerializedMacroActionType_MoveMouseMacroAction,
        SerializedMacroActionType_ScrollMouseMacroAction,
        SerializedMacroActionType_DelayMacroAction,
        SerializedMacroActionType_TextMacroAction
    } serialized_macro_action_type_t;

// Functions:

    parser_error_t ParseMacro(config_buffer_t *buffer);

#endif
