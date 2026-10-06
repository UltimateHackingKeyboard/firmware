#include "test_input_machine.h"
#include "test_hooks.h"
#include "device.h"
#include "test_suite.h"
#include "macros/keyid_parser.h"
#include "macros/shortcut_parser.h"
#include "macros/set_command.h"
#include "str_utils.h"
#include "key_action.h"
#include "key_states.h"
#include "slot.h"
#include "keymap.h"
#include "layer.h"
#include "timer.h"
#include "event_scheduler.h"
#include "secondary_role_driver.h"
#include "logger.h"
#include "utils.h"
#include "usb_report_updater.h"

// The left half only produces keys for a link test and logs none of its actions - its log
// is reserved for the bridge's own diagnostics.
#define LOG_VERBOSE(fmt, ...) do { if (TestSuite_Verbose && !DEVICE_IS_UHK80_LEFT) LogU(fmt, ##__VA_ARGS__); } while(0)

#define TEST_TIMEOUT_MS 100

// InputMachine state
const test_t *InputMachine_CurrentTest = NULL;
uint16_t InputMachine_ActionIndex = 0;
bool InputMachine_Failed = false;
bool InputMachine_TimedOut = false;
static uint32_t endReachedTime = 0;
static bool endReached = false;

// Delay state
static bool inDelay = false;

// The test's schedule: its start plus every delay so far. A delay waits until this point
// rather than for its length from whenever it happened to begin, so the ticks spent on other
// actions don't accumulate into drift - which matters in link tests, where both halves run
// the script and their timelines must stay in step.
static uint32_t currentTestTime = 0;

// Key ID parsing: convert string like "u" or "leftShift" to slot + keyId
bool TestInput_ParseKeyId(const char *keyIdStr, uint8_t *slotId, uint8_t *keyId) {
    if (keyIdStr == NULL) {
        return false;
    }

    const char *end = keyIdStr;
    while (*end != '\0') end++;

    parser_context_t ctx = {
        .macroState = NULL,
        .begin = keyIdStr,
        .at = keyIdStr,
        .end = end,
        .nestingLevel = 0,
        .nestingBound = 0,
    };

    uint8_t combinedId = MacroKeyIdParser_TryConsumeKeyId(&ctx);
    if (combinedId == 255) {
        return false;
    }

    *slotId = combinedId / 64;
    *keyId = combinedId % 64;
    return true;
}
extern hid_keyboard_report_t *ActiveKeyboardReport;
// Build expected report from space-separated shortcut string and validate against actual
// If logFailure is true, logs details on mismatch
static bool validateReport(const char *expectShortcuts, bool logFailure) {
    const hid_keyboard_report_t *actual = ActiveKeyboardReport;

    // Build expected: combine modifiers and scancodes from all shortcuts
    uint8_t expectedMods = 0;
    uint8_t expectedScancodes[6] = { 0 };
    uint8_t scancodeCount = 0;

    const char *at = expectShortcuts;
    while (*at != '\0') {
        while (*at == ' ') at++;
        if (*at == '\0') break;

        const char *shortcutEnd = at;
        while (*shortcutEnd != '\0' && *shortcutEnd != ' ') shortcutEnd++;

        // Skip empty tokens
        if (shortcutEnd == at) {
            continue;
        }

        key_action_t keyAction = { 0 };
        if (!MacroShortcutParser_Parse(at, shortcutEnd, MacroSubAction_Tap, NULL, &keyAction)) {
            if (logFailure) LOG_FAILURE("[TEST] FAIL: invalid shortcut in '%s'\n", expectShortcuts);
            return false;
        }

        if (keyAction.type == KeyActionType_Keystroke) {
            expectedMods |= keyAction.keystroke.modifiers;
            if (keyAction.keystroke.scancode != 0 && scancodeCount < 6) {
                expectedScancodes[scancodeCount++] = keyAction.keystroke.scancode;
            }
        }

        at = shortcutEnd;
    }

    bool match = true;
    if (actual->modifiers != expectedMods) match = false;
    for (int i = 0; i < scancodeCount && match; i++) {
        if (!KeyboardReport_ContainsScancode(actual, expectedScancodes[i])) match = false;
    }
    if (KeyboardReport_ScancodeCount(actual) != scancodeCount) match = false;

    if (!match && logFailure) {
        LOG_FAILURE("[TEST] <   FAIL: Expect '%s', got '%s'\n",
            expectShortcuts, Utils_GetUsbReportString(actual));
    }

    return match;
}

void InputMachine_Start(const test_t *test) {
    InputMachine_CurrentTest = test;
    InputMachine_ActionIndex = 0;
    InputMachine_Failed = false;
    InputMachine_TimedOut = false;
    endReached = false;
    endReachedTime = 0;
    inDelay = false;
    currentTestTime = Timer_GetCurrentTime();
}

// Only the half that runs the user logic consumes this event. Setting it on the left leaves it
// permanently pending, which user_logic reports on every pass - enough to flood the left's log
// buffer and make its diagnostics unreadable.
static void notifyStateMatrixChanged(void) {
    bool consumedLocally = !DEVICE_IS_UHK80_LEFT;

    if (consumedLocally) {
        EventVector_Set(EventVector_StateMatrix);
        EventVector_WakeMain();
    }
}

// In a link test both halves run the same script and each presses only the keys it physically
// owns, so the other half's presses are its job, not ours.
static bool isForeignSlot(uint8_t slotId) {
    return InputMachine_CurrentTest != NULL
        && InputMachine_CurrentTest->linkTestId != 0
        && slotId != CURRENT_SLOT_ID;
}

void InputMachine_Tick(void) {
    if (InputMachine_CurrentTest == NULL || InputMachine_Failed || InputMachine_TimedOut) {
        return;
    }

    // Check timeout if we've reached End
    if (endReached) {
        if (Timer_GetElapsedTime(&endReachedTime) >= TEST_TIMEOUT_MS) {
            InputMachine_TimedOut = true;
        }
        return;
    }

    while (true) {
        const test_action_t *action = &InputMachine_CurrentTest->actions[InputMachine_ActionIndex];

        switch (action->type) {
            case TestAction_Press: {
                uint8_t slotId, keyId;
                if (TestInput_ParseKeyId(action->keyId, &slotId, &keyId)) {
                    if (isForeignSlot(slotId)) {
                        LOG_VERBOSE("[TEST] > Press [%s] - other half's job\n", action->keyId);
                        InputMachine_ActionIndex++;
                        return;
                    }
                    KeyStates[slotId][keyId].hardwareSwitchState = true;
                    LOG_VERBOSE("[TEST] > Press [%s]\n", action->keyId);
                    notifyStateMatrixChanged();
                } else {
                    LOG_FAILURE("[TEST] FAIL: Press [%s] - invalid key\n", action->keyId);
                    InputMachine_Failed = true;
                    return;
                }
                InputMachine_ActionIndex++;
                return;
            }

            case TestAction_Release: {
                uint8_t slotId, keyId;
                if (TestInput_ParseKeyId(action->keyId, &slotId, &keyId)) {
                    if (isForeignSlot(slotId)) {
                        LOG_VERBOSE("[TEST] > Release [%s] - other half's job\n", action->keyId);
                        InputMachine_ActionIndex++;
                        return;
                    }
                    KeyStates[slotId][keyId].hardwareSwitchState = false;
                    LOG_VERBOSE("[TEST] > Release [%s]\n", action->keyId);
                    notifyStateMatrixChanged();
                } else {
                    LOG_FAILURE("[TEST] FAIL: Release [%s] - invalid key\n", action->keyId);
                    InputMachine_Failed = true;
                    return;
                }
                InputMachine_ActionIndex++;
                return;
            }

            case TestAction_Delay: {
                if (!inDelay) {
                    inDelay = true;
                    currentTestTime += action->delayMs;
                }
                bool due = (int32_t)(Timer_GetCurrentTime() - currentTestTime) >= 0;
                if (due) {
                    LOG_VERBOSE("[TEST] > Delay %dms\n", action->delayMs);
                    inDelay = false;
                    InputMachine_ActionIndex++;
                    break;
                }
                return;
            }

            case TestAction_SetAction: {
                uint8_t slotId, keyId;
                if (!TestInput_ParseKeyId(action->keyId, &slotId, &keyId)) {
                    LOG_FAILURE("[TEST] FAIL: SetAction [%s] - invalid key\n", action->keyId);
                    InputMachine_Failed = true;
                    return;
                }

                const char *shortcut = action->shortcutStr;
                const char *shortcutEnd = shortcut;
                while (*shortcutEnd != '\0') shortcutEnd++;

                key_action_t keyAction = { 0 };
                if (!MacroShortcutParser_Parse(shortcut, shortcutEnd, MacroSubAction_Tap, NULL, &keyAction)) {
                    LOG_FAILURE("[TEST] FAIL: SetAction [%s] = '%s' - invalid shortcut\n", action->keyId, action->shortcutStr);
                    InputMachine_Failed = true;
                    return;
                }

                CurrentKeymap[LayerId_Base][slotId][keyId].action = keyAction;
                LOG_VERBOSE("[TEST] > SetAction [%s] = '%s'\n", action->keyId, action->shortcutStr);
                InputMachine_ActionIndex++;
                break;
            }

            case TestAction_SetMacro: {
                uint8_t slotId, keyId;
                if (!TestInput_ParseKeyId(action->keyId, &slotId, &keyId)) {
                    LOG_FAILURE("[TEST] FAIL: SetMacro [%s] - invalid key\n", action->keyId);
                    InputMachine_Failed = true;
                    return;
                }

                key_action_t keyAction = {
                    .type = KeyActionType_InlineMacro,
                    .inlineMacro = {
                        .text = action->macroText
                    }
                };

                CurrentKeymap[action->layerId][slotId][keyId].action = keyAction;
                LOG_VERBOSE("[TEST] > SetMacro layer %d [%s] = '%s'\n", action->layerId, action->keyId, action->macroText);
                InputMachine_ActionIndex++;
                break;
            }

            case TestAction_SetLayerHold: {
                uint8_t slotId, keyId;
                if (!TestInput_ParseKeyId(action->keyId, &slotId, &keyId)) {
                    LOG_FAILURE("[TEST] FAIL: SetLayerHold [%s] - invalid key\n", action->keyId);
                    InputMachine_Failed = true;
                    return;
                }

                key_action_t keyAction = {
                    .type = KeyActionType_SwitchLayer,
                    .switchLayer = {
                        .layer = action->layerId,
                        .mode = action->switchLayerMode
                    }
                };

                CurrentKeymap[LayerId_Base][slotId][keyId].action = keyAction;
                CurrentKeymap[action->layerId][slotId][keyId].action = keyAction;
                LOG_VERBOSE("[TEST] > SetLayerHold [%s] = layer %d, mode %d\n", action->keyId, action->layerId, action->switchLayerMode);
                InputMachine_ActionIndex++;
                break;
            }

            case TestAction_SetLayerAction: {
                uint8_t slotId, keyId;
                if (!TestInput_ParseKeyId(action->keyId, &slotId, &keyId)) {
                    LOG_FAILURE("[TEST] FAIL: SetLayerAction [%s] - invalid key\n", action->keyId);
                    InputMachine_Failed = true;
                    return;
                }

                const char *shortcut = action->shortcutStr;
                const char *shortcutEnd = shortcut;
                while (*shortcutEnd != '\0') shortcutEnd++;

                key_action_t keyAction = { 0 };
                if (!MacroShortcutParser_Parse(shortcut, shortcutEnd, MacroSubAction_Tap, NULL, &keyAction)) {
                    LOG_FAILURE("[TEST] FAIL: SetLayerAction layer %d [%s] = '%s' - invalid shortcut\n", action->layerId, action->keyId, action->shortcutStr);
                    InputMachine_Failed = true;
                    return;
                }

                CurrentKeymap[action->layerId][slotId][keyId].action = keyAction;
                LOG_VERBOSE("[TEST] > SetLayerAction layer %d [%s] = '%s'\n", action->layerId, action->keyId, action->shortcutStr);
                InputMachine_ActionIndex++;
                break;
            }

            case TestAction_SetSecondaryRole: {
                uint8_t slotId, keyId;
                if (!TestInput_ParseKeyId(action->keyId, &slotId, &keyId)) {
                    LOG_FAILURE("[TEST] FAIL: SetSecondaryRole [%s] - invalid key\n", action->keyId);
                    InputMachine_Failed = true;
                    return;
                }

                key_action_t keyAction = {
                    .type = KeyActionType_Keystroke,
                    .keystroke = {
                        .keystrokeType = KeystrokeType_Basic,
                        .scancode = action->primaryScancode,
                        .secondaryRole = action->secondaryRoleId,
                        .modifiers = 0
                    }
                };

                CurrentKeymap[LayerId_Base][slotId][keyId].action = keyAction;
                LOG_VERBOSE("[TEST] > SetSecondaryRole [%s] = scancode %d, secondary %d\n",
                    action->keyId, action->primaryScancode, action->secondaryRoleId);
                InputMachine_ActionIndex++;
                break;
            }

            case TestAction_SetGenericAction: {
                uint8_t slotId, keyId;
                if (!TestInput_ParseKeyId(action->keyId, &slotId, &keyId)) {
                    LOG_FAILURE("[TEST] FAIL: SetGenericAction [%s] - invalid key\n", action->keyId);
                    InputMachine_Failed = true;
                    return;
                }

                CurrentKeymap[LayerId_Base][slotId][keyId].action = action->keyAction;
                LOG_VERBOSE("[TEST] > SetGenericAction [%s] = type %d\n", action->keyId, action->keyAction.type);
                InputMachine_ActionIndex++;
                break;
            }

            case TestAction_SetConfig: {
                const char *configText = action->configText;
                const char *configEnd = configText;
                while (*configEnd != '\0') configEnd++;

                parser_context_t ctx = {
                    .macroState = NULL,
                    .begin = configText,
                    .at = configText,
                    .end = configEnd,
                    .nestingLevel = 0,
                    .nestingBound = 0,
                };

                Macro_ProcessSetCommand(&ctx);
                LOG_VERBOSE("[TEST] > SetConfig '%s'\n", action->configText);
                InputMachine_ActionIndex++;
                break;
            }

            case TestAction_SetBool:
                if (action->boolPtr != NULL) {
                    *action->boolPtr = action->boolValue;
                }
                LOG_VERBOSE("[TEST] > SetBool %s\n", action->boolValue ? "true" : "false");
                InputMachine_ActionIndex++;
                break;

            case TestAction_CheckNow:
                if (!validateReport(action->expectShortcuts, true)) {  // Always log failures
                    InputMachine_Failed = true;
                    return;
                }
                LOG_VERBOSE("[TEST] <   CheckNow '%s' - Ok\n", action->expectShortcuts);
                InputMachine_ActionIndex++;
                break;

            case TestAction_Expect:
            case TestAction_ExpectMaybe:
                // OutputMachine handles these
                InputMachine_ActionIndex++;
                break;

            case TestAction_End:
            default:
                endReached = true;
                endReachedTime = Timer_GetCurrentTime();
                return;
        }
    }
}

bool InputMachine_IsDone(void) {
    if (InputMachine_CurrentTest == NULL) {
        return true;
    }
    if (InputMachine_Failed || InputMachine_TimedOut) {
        return true;
    }
    return endReached;
}
