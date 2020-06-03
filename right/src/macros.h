#ifndef __MACROS_H__
#define __MACROS_H__

// Includes:

    #include <stdint.h>
    #include <stdbool.h>
    #include "key_action.h"
    #include "usb_device_config.h"
    #include "key_states.h"

// Macros:
    #define MACRO_CYCLES_TO_POSTPONE 4

    #define MAX_MACRO_NUM 255
    #define STATUS_BUFFER_MAX_LENGTH 1024
    #define LAYER_STACK_SIZE 10
    #define MACRO_STATE_POOL_SIZE 20
    #define MAX_REG_COUNT 32

    #define ALTMASK (HID_KEYBOARD_MODIFIER_LEFTALT | HID_KEYBOARD_MODIFIER_RIGHTALT)
    #define CTRLMASK (HID_KEYBOARD_MODIFIER_LEFTCTRL | HID_KEYBOARD_MODIFIER_RIGHTCTRL)
    #define SHIFTMASK (HID_KEYBOARD_MODIFIER_LEFTSHIFT | HID_KEYBOARD_MODIFIER_RIGHTSHIFT)
    #define GUIMASK (HID_KEYBOARD_MODIFIER_LEFTGUI | HID_KEYBOARD_MODIFIER_RIGHTGUI)

// Typedefs:

    typedef struct {
        uint16_t firstMacroActionOffset;
        uint8_t macroActionsCount; //official uses uint16_t, we think that 256 actions per macro should suffice
        uint8_t macroNameOffset; //negative w.r.t. firstMacroActionOffset, we think that 256 chars per name should suffice
    } macro_reference_t;

    typedef struct {
        uint8_t layer;
        uint8_t keymap;
        bool held;
        bool removed;
    } layerStackRecord;

    typedef enum {
        MacroSubAction_Tap,
        MacroSubAction_Press,
        MacroSubAction_Release,
        MacroSubAction_Hold
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
                uint16_t scancode;
                uint8_t modifierMask;
                bool sticky;
            } ATTR_PACKED key;
            struct {
                macro_sub_action_t action;
                uint8_t mouseButtonsMask;
            } ATTR_PACKED mouseButton;
            struct {
                int16_t x;
                int16_t y;
            } ATTR_PACKED moveMouse;
            struct {
                int16_t x;
                int16_t y;
            } ATTR_PACKED scrollMouse;
            struct {
                uint16_t delay;
            } ATTR_PACKED delay;
            struct {
                const char *text;
                uint16_t textLen;
            } ATTR_PACKED text;
        };
        macro_action_type_t type;
    } ATTR_PACKED macro_action_t;

    //TODO: break this down to a union once memory starts to be a problem
    typedef struct {
        bool macroInterrupted;
        bool macroBroken;
        bool macroPlaying;
        bool macroSleeping;

        uint8_t currentMacroIndex;
        uint16_t currentMacroActionIndex;
        macro_action_t currentMacroAction;
        key_state_t *currentMacroKey;
        uint8_t previousMacroIndex;
        uint32_t previousMacroEndTime;
        uint32_t previousMacroStartTime;
        uint32_t currentMacroStartTime;

        uint8_t pressPhase;
        bool mouseMoveInMotion;
        bool mouseScrollInMotion;
        uint16_t dispatchTextIndex;
        uint8_t dispatchReportIndex;

        bool currentConditionPassed;
        bool currentIfShortcutConditionPassed;
        bool currentIfSecondaryConditionPassed;
        uint8_t postponeNextNCommands;
        bool weInitiatedPostponing;

        bool delayActive;
        uint32_t delayStart;
        uint32_t resolveSecondaryPhase2StartTime;

        bool holdActive;
        uint8_t holdLayerIdx;

        uint16_t bufferOffset;

        uint8_t parentMacroSlot;


        bool reportsUsed;
        usb_mouse_report_t macroMouseReport;
        usb_basic_keyboard_report_t macroBasicKeyboardReport;
        usb_media_keyboard_report_t macroMediaKeyboardReport;
        usb_system_keyboard_report_t macroSystemKeyboardReport;
    } macro_state_t;

// Variables:

    extern macro_reference_t AllMacros[MAX_MACRO_NUM];
    extern uint8_t AllMacrosCount;
    extern macro_state_t MacroState[MACRO_STATE_POOL_SIZE];
    extern bool MacroPlaying;
    extern layer_id_t Macros_ActiveLayer;
    extern bool Macros_ActiveLayerHeld;

// Functions:

    void Macros_StartMacro(uint8_t index, key_state_t *keyState, uint8_t parentMacroSlot);
    void Macros_ContinueMacro(void);
    void Macros_SignalInterrupt(void);
    bool Macros_ClaimReports(void);
    void Macros_ReportError(const char* err, const char* arg, const char *argEnd);
    void Macros_ReportErrorNum(const char* err, uint32_t num);
    void Macros_SetStatusString(const char* text, const char *textEnd);
    void Macros_SetStatusStringInterpolated(const char* text, const char *textEnd);
    void Macros_SetStatusBool(bool b);
    void Macros_SetStatusNum(uint32_t n);
    void Macros_SetStatusNumSpaced(uint32_t n, bool space);
    void Macros_SetStatusChar(char n);
    void Macros_UpdateLayerStack();
    bool Macros_IsLayerHeld();

#endif
