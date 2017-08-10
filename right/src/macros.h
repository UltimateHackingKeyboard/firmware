#ifndef __MACROS_H__
#define __MACROS_H__

// Includes:

    #include <stdint.h>
    #include <stdbool.h>
    #include "key_action.h"
    #include "usb_device_config.h"

// Macros:

    #define MAX_MACRO_NUM 255

// Typedefs:

    typedef struct {
        uint16_t firstMacroActionOffset;
        uint16_t macroActionsCount;
    } macro_reference_t;

    typedef enum {
        MacroSubAction_Press,
        MacroSubAction_Hold,
        MacroSubAction_Release,
    } macro_sub_action_t;

    typedef enum {
        MacroActionType_Key,
        MacroActionType_MouseButton,
        MacroActionType_MoveMouse,
        MacroActionType_ScrollMouse,
        MacroActionType_Delay,
        MacroActionType_Text,
    } macro_action_type_t;

    typedef struct {
        union {
            struct {
                macro_sub_action_t action;
                keystroke_type_t type;
                uint8_t scancode;
                uint8_t modifierMask;
            } __attribute__ ((packed)) key;
            struct {
                macro_sub_action_t action;
                uint8_t mouseButtonsMask;
            } __attribute__ ((packed)) mouseButton;
            struct {
                int16_t x;
                int16_t y;
            } __attribute__ ((packed)) moveMouse;
            struct {
                int16_t x;
                int16_t y;
            } __attribute__ ((packed)) scrollMouse;
            struct {
                int16_t delay;
            } __attribute__ ((packed)) delay;
            struct {
                const char *text;
                uint16_t textLen;
            } __attribute__ ((packed)) text;
        };
        macro_action_type_t type;
    } __attribute__ ((packed)) macro_action_t;

// Variables:

    extern macro_reference_t AllMacros[MAX_MACRO_NUM];
    extern uint8_t AllMacrosCount;
    extern bool MacroPlaying;
    extern usb_mouse_report_t MacroMouseReport;
    extern usb_basic_keyboard_report_t MacroBasicKeyboardReport;
    extern usb_media_keyboard_report_t MacroMediaKeyboardReport;
    extern usb_system_keyboard_report_t MacroSystemKeyboardReport;

// Functions:

    void Macros_StartMacro(uint8_t index);
    void Macros_ContinueMacro(void);

#endif
