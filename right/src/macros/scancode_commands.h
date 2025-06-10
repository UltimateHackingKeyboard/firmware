#ifndef __MACROS_SCANCODE_COMMANDS_H__
#define __MACROS_SCANCODE_COMMANDS_H__

// Includes:

    #include <stdint.h>
    #include <stdbool.h>
    #include "attributes.h"
    #include "macros/typedefs.h"
    #include "str_utils.h"
    #include "macros/typedefs.h"

// Macros:

// Typedefs:

// Variables:

// Functions:

    macro_result_t Macros_DispatchText(const char* text, uint16_t textLen, bool rawString);
    macro_result_t Macros_ProcessKeyCommandAndConsume(parser_context_t* ctx, macro_sub_action_t type, macro_usb_keyboard_reports_t* reports);
    macro_result_t Macros_ProcessTapKeySeqCommand(parser_context_t* ctx);
    macro_result_t Macros_ProcessMouseButtonAction(void);
    macro_result_t Macros_ProcessTextAction(void);
    macro_result_t Macros_ProcessMoveMouseAction(void);
    macro_result_t Macros_ProcessScrollMouseAction(void);
    macro_result_t Macros_ProcessKeyAction();

#endif
