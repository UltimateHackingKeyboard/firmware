#ifndef __MACROS_H__
#define __MACROS_H__

// Includes:

    #include <stdint.h>
    #include <stdbool.h>
    #include "attributes.h"
    #include "key_action.h"
    #include "usb_device_config.h"
    #include "key_states.h"
    #include "str_utils.h"
    #include "macros/typedefs.h"
    #include "macros/vars.h"
    #include "event_scheduler.h"

// Macros:
    #define MACRO_CYCLES_TO_POSTPONE 4

    #define MAX_MACRO_NUM 255
    #define MACRO_STATE_POOL_SIZE 16
    #define MACRO_HISTORY_POOL_SIZE 16
    #define MACRO_SCOPE_STATE_POOL_SIZE (MACRO_STATE_POOL_SIZE*2)
    #define MAX_REG_COUNT 32

    #define MAX_MACRO_ARGUMENT_COUNT 8

    #define ALTMASK (HID_KEYBOARD_MODIFIER_LEFTALT | HID_KEYBOARD_MODIFIER_RIGHTALT)
    #define CTRLMASK (HID_KEYBOARD_MODIFIER_LEFTCTRL | HID_KEYBOARD_MODIFIER_RIGHTCTRL)
    #define SHIFTMASK (HID_KEYBOARD_MODIFIER_LEFTSHIFT | HID_KEYBOARD_MODIFIER_RIGHTSHIFT)
    #define GUIMASK (HID_KEYBOARD_MODIFIER_LEFTGUI | HID_KEYBOARD_MODIFIER_RIGHTGUI)

    typedef enum {
        MacroIndex_InlineMacro = 254,
        MacroIndex_None = 255,

        MacroIndex_MaxUserDefinableCount = 254,
        MacroIndex_MaxCount = 255,
    } macro_index_t;

// Typedefs:

    typedef struct {
        uint16_t firstMacroActionOffset;
        uint8_t macroActionsCount;
        uint8_t macroNameOffset; //negative w.r.t. firstMacroActionOffset
    } macro_reference_t;

    typedef enum {
        Scheduler_Preemptive,
        Scheduler_Blocking,
    } macro_scheduler_t;

    typedef enum {
        MacroActionType_Key,
        MacroActionType_MouseButton,
        MacroActionType_MoveMouse,
        MacroActionType_ScrollMouse,
        MacroActionType_Delay,
        MacroActionType_Text,
        MacroActionType_Command,
    } macro_action_type_t;

    typedef struct {
        union {
            struct {
                macro_sub_action_t action;
                keystroke_type_t type;
                uint16_t scancode;
                uint8_t inputModMask;
                uint8_t outputModMask;
                uint8_t stickyModMask;
            } ATTR_PACKED key;
            struct {
                macro_sub_action_t action;
                uint32_t mouseButtonsMask : 24;
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

    typedef enum {
        AutoRepeatState_Executing = 0,
        AutoRepeatState_Waiting = 1
    } macro_autorepeat_state_t;

    // data that are needed for local scopes
    typedef struct {
        bool slotUsed;
        uint8_t parentScopeIndex;

        // local macro scope data
        struct {
            uint16_t commandBegin;
            uint16_t commandEnd;
            uint16_t commandAddress;
            bool lastIfSucceeded : 1;
        } ATTR_PACKED ms;

        // local action scope data
        struct {
            bool braceExecuting : 1;
            bool isWhileScope : 1;
            bool whileExecuting : 1;
            bool modifierPostpone : 1;
            bool modifierSuppressMods : 1;
            bool currentConditionPassed : 1;
            bool currentIfShortcutConditionPassed : 1;
            bool currentIfSecondaryConditionPassed : 1;
            bool currentIfHoldConditionPassed : 1;
        } ATTR_PACKED as;
    } ATTR_PACKED macro_scope_state_t;

    typedef struct macro_state_t macro_state_t;

    typedef struct {
        uint32_t macroStartTime;
        uint8_t macroIndex;
    } macro_history_t;

    struct macro_state_t {
        // local scope data
        macro_scope_state_t *ls;

        // macro scope data
        // these can be destroyed at the end of macro runtime, and probably should be re-initialized with each macro start
        struct {
            macro_action_t currentMacroAction;
            key_state_t *currentMacroKey;
            uint32_t currentMacroStartTime;
            uint16_t currentMacroActionIndex;
            uint16_t currentMacroArgumentOffset;
            uint16_t bufferOffset;
            uint8_t currentMacroKeyStamp;
            uint8_t parentMacroSlot;
            uint8_t currentMacroIndex;
            uint8_t postponeNextNCommands;
            // ---- 4-aligned ----
            uint8_t nextSlot;
            uint8_t oneShot : 2;
            bool macroInterrupted : 1;
            // TODO: refactor macroSleeping, macroBroken and macroPlaying into a single state?
            bool macroSleeping : 1;
            bool macroBroken : 1;
            bool macroPlaying : 1;
            bool macroScheduled : 1;
            bool reportsUsed : 1;
            bool wakeMeOnTime : 1;
            bool wakeMeOnKeystateChange: 1;
            bool autoRepeatInitialDelayPassed: 1;
            macro_autorepeat_state_t autoRepeatPhase: 1;
            // ---- 4-aligned ----
            macro_argref_t arguments[MAX_MACRO_ARGUMENT_COUNT];

            macro_usb_keyboard_reports_t reports;

            uint8_t argumentCount : 4;
            bool macroHeadersProcessed : 1;
        } ms;

        // action scope data
        struct {
            //private data
            union {
                struct {
                    uint16_t textIdx;
                    uint16_t subIndex;
                    uint16_t stringOffset;
                    enum {
                        REPORT_EMPTY = 0,
                        REPORT_PARTIAL,
                        REPORT_FULL
                    } reportState : 8;
                } dispatchData;

                struct {
                    uint32_t start;
                } delayData;

                struct {
                    uint32_t phase2Start;
                } secondaryRoleData;

                struct {
                    uint8_t layerStackIdx;

                } holdLayerData;
                struct {
                    uint8_t atKeyIdx;
                    bool toggleShouldActivate : 1;
                } keySeqData;
            };

            //shared data
            uint8_t actionPhase;
            bool actionActive : 1;
        } as;
    };


    // Schedule is given by a single-linked circular list.
    typedef struct {
        // Current slot is the next slot to be run. Previous reference is
        // required in order to be able to unschedule current slot.
        uint8_t previousSlotIdx;
        uint8_t currentSlotIdx;
        // LastQueuedSlot is the slot after which the new schedules should be
        // added. Either points to current (usually) or to the last added
        // element (if it still waits to be executed for the first time).
        uint8_t lastQueuedSlot;
        // Total number of scheduled slots.
        uint8_t activeSlotCount;
        // Slots that remain to be executed in current cycle. (They might get
        // executed or not - if quota is exceeded or if some macro returns
        // blocking state, then schedule cycle is not completed within one
        // UHK update cycle.)
        uint8_t remainingCount;
    } scheduler_state_t;

// Variables:

    extern macro_reference_t AllMacros[MacroIndex_MaxCount];
    extern uint8_t AllMacrosCount;
    extern macro_state_t MacroState[MACRO_STATE_POOL_SIZE];
    extern macro_history_t MacroHistory[MACRO_HISTORY_POOL_SIZE];
    extern macro_scope_state_t MacroScopeState[MACRO_SCOPE_STATE_POOL_SIZE];
    extern macro_state_t *S;
    extern bool MacroPlaying;
    extern scheduler_state_t Macros_SchedulerState;
    extern uint32_t Macros_WakeMeOnTime;
    extern bool Macros_WakeMeOnOneShot;
    extern bool Macros_WakeMeOnKeystateChange;
    extern bool Macros_WakedBecauseOfOneShot;
    extern bool Macros_WakedBecauseOfTime;
    extern bool Macros_WakedBecauseOfKeystateChange;
    extern uint16_t AutoRepeatInitialDelay;
    extern uint16_t AutoRepeatDelayRate;
    extern bool Macros_ParserError;
    extern bool Macros_DryRun;
    extern bool Macros_ValidationInProgress;
    extern macro_usb_keyboard_reports_t Macros_PersistentReports;

// Functions:

    bool Macros_PushScope(parser_context_t* ctx);
    bool Macros_PopScope(parser_context_t* ctx);
    bool Macros_LoadNextCommand();
    bool Macros_LoadNextAction();
    bool Macros_ClaimReports(void);
    bool Macros_CurrentMacroKeyIsActive();
    bool Macros_IsLayerHeld();
    bool Macros_MacroHasActiveInstance(macro_index_t macroIdx);
    char Macros_ConsumeStatusChar();
    int32_t Macros_ConsumeInt(parser_context_t* ctx);
    macro_result_t goTo(parser_context_t* ctx);
    macro_result_t Macros_CallMacro(uint8_t macroIndex);
    macro_result_t Macros_ExecMacro(uint8_t macroIndex);
    macro_result_t Macros_ForkMacro(uint8_t macroIndex);
    macro_result_t Macros_GoToAddress(uint8_t address);
    macro_result_t Macros_GoToLabel(parser_context_t* ctx);
    macro_result_t Macros_SleepTillKeystateChange();
    macro_result_t Macros_SleepTillTime(uint32_t time, const char* reason);
    uint8_t Macros_ConsumeLayerId(parser_context_t* ctx);
    uint8_t Macros_QueueMacro(uint8_t index, key_state_t *keyState, uint8_t timestamp, uint8_t queueAfterSlot);
    uint8_t Macros_StartMacro(uint8_t index, key_state_t *keyState, uint16_t argumentOffset, uint8_t timestamp, uint8_t parentMacroSlot, bool runFirstAction, const char *inlineText);
    uint8_t Macros_StartInlineMacro(const char *text, key_state_t *keyState, uint8_t timestamp);
    uint8_t Macros_TryConsumeKeyId(parser_context_t* ctx);
    void Macros_ContinueMacro(void);
    void Macros_Initialize();
    void Macros_ResetBasicKeyboardReports();
    void Macros_SignalInterrupt(void);
    void Macros_ValidateAllMacros();
    void Macros_ValidateMacro(uint8_t macroIndex, uint16_t argumentOffset, uint8_t moduleId, uint8_t keyIdx, uint8_t keymapIdx, uint8_t layerIdx);
    void Macros_WakeBecauseOfKeystateChange();
    void Macros_WakeBecauseOfOneShot();

#endif
