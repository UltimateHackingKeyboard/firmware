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
        MacroActionType_Command,
    } macro_action_type_t;

    typedef enum {
        MacroResult_InProgressFlag = 1,
        MacroResult_ActionFinishedFlag = 2,
        MacroResult_DoneFlag = 4,
        MacroResult_YieldFlag = 8,
        MacroResult_Blocking = MacroResult_InProgressFlag,
        MacroResult_Waiting = MacroResult_InProgressFlag | MacroResult_YieldFlag,
        MacroResult_Finished = MacroResult_ActionFinishedFlag,
        MacroResult_JumpedForward = MacroResult_DoneFlag,
        MacroResult_JumpedBackward = MacroResult_DoneFlag | MacroResult_YieldFlag,
    } macro_result_t;

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
            struct {
                const char *text;
                uint16_t textLen;
                uint8_t cmdCount;
            } ATTR_PACKED cmd;
        };
        macro_action_type_t type;
    } ATTR_PACKED macro_action_t;

    typedef struct {
        // persistent scope data
        // these need to live in between macro calls
        struct {
            uint32_t previousMacroStartTime;
            uint8_t previousMacroIndex;
        } ps;

        // macro scope data
        // these can be destroyed at the end of macro runtime, and probably should be re-initialized with each macro start
        struct {
            macro_action_t currentMacroAction;
            key_state_t *currentMacroKey;
            uint32_t currentMacroStartTime;
            uint16_t currentMacroActionIndex;
            uint16_t bufferOffset;
            uint16_t commandBegin;
            uint16_t commandEnd;
            uint8_t parentMacroSlot;
            uint8_t currentMacroIndex;
            uint8_t postponeNextNCommands;
            uint8_t commandAddress;
            bool macroInterrupted : 1;
            bool macroSleeping : 1;
            bool macroBroken : 1;
            bool macroPlaying : 1;
            bool reportsUsed : 1;

            usb_mouse_report_t macroMouseReport;
            usb_basic_keyboard_report_t macroBasicKeyboardReport;
            usb_media_keyboard_report_t macroMediaKeyboardReport;
            usb_system_keyboard_report_t macroSystemKeyboardReport;
        } ms;

        // action scope data
        struct {
            //private data
            union {
                struct {
                    uint16_t textIdx;
                    uint8_t reportIdx;
                } dispatchData;

                struct {
                    uint32_t start;
                } delayData;

                struct {
                    uint32_t phase2Start;
                } secondaryRoleData;

                struct {
                    uint8_t layerIdx;

                } holdLayerData;
                struct {
                    uint8_t atKeyIdx;
                } keySeqData;
            };

            //shared data
            uint8_t actionPhase;
            bool actionActive : 1;
            bool currentConditionPassed : 1;
            bool currentIfShortcutConditionPassed : 1;
            bool currentIfSecondaryConditionPassed : 1;
            bool weInitiatedPostponing : 1;

        } as;
    }  macro_state_t;

// Variables:

    extern macro_reference_t AllMacros[MAX_MACRO_NUM];
    extern uint8_t AllMacrosCount;
    extern macro_state_t MacroState[MACRO_STATE_POOL_SIZE];
    extern bool MacroPlaying;
    extern layer_id_t Macros_ActiveLayer;
    extern bool Macros_ActiveLayerHeld;

// Functions:

    void Macros_StartMacro(uint8_t index, key_state_t *keyState, uint8_t parentMacroSlot, bool runFirstAction);
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
    void Macros_Initialize();
    void Macros_ClearStatus();
    bool Macros_IsLayerHeld();
    uint8_t Macros_ParseLayerId(const char* arg1, const char* cmdEnd);
    int32_t Macros_ParseInt(const char *a, const char *aEnd, const char* *parsedTill);
    bool Macros_ParseBoolean(const char *a, const char *aEnd);

#endif
