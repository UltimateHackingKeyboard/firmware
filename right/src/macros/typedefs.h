#ifndef __MACROS_CORE_H__
#define __MACROS_CORE_H__

// Includes:


    #include <stdbool.h>
    #include <stdint.h>
    #include "usb_device_config.h"

// Macros:

    #define STATUS_BUFFER_MAX_LENGTH 3000

// Typedefs:


    typedef enum {
        MacroSubAction_Tap,
        MacroSubAction_Press,
        MacroSubAction_Release,
        MacroSubAction_Hold,
        MacroSubAction_Toggle,
    } macro_sub_action_t;

    typedef enum {
        MacroResult_InProgressFlag = 1,
        MacroResult_ActionFinishedFlag = 2,
        MacroResult_DoneFlag = 4,
        MacroResult_YieldFlag = 8,
        MacroResult_BlockingFlag = 16,
        MacroResult_ConditionFailedFlag = 32,
        MacroResult_OpeningBraceFlag = 64,
        MacroResult_ClosingBraceFlag = 128,
        MacroResult_Blocking = MacroResult_InProgressFlag | MacroResult_BlockingFlag,
        MacroResult_Waiting = MacroResult_InProgressFlag | MacroResult_YieldFlag,
        MacroResult_Sleeping = MacroResult_InProgressFlag | MacroResult_YieldFlag,
        MacroResult_Finished = MacroResult_ActionFinishedFlag,
        MacroResult_JumpedForward = MacroResult_DoneFlag,
        MacroResult_JumpedBackward = MacroResult_DoneFlag | MacroResult_YieldFlag,
    } macro_result_t;

    typedef struct {
        usb_mouse_report_t macroMouseReport;
        usb_basic_keyboard_report_t macroBasicKeyboardReport;
        usb_media_keyboard_report_t macroMediaKeyboardReport;
        usb_system_keyboard_report_t macroSystemKeyboardReport;
        uint8_t inputModifierMask;
    } macro_usb_keyboard_reports_t;

// Variables:

// Functions:

#endif
