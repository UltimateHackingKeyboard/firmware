#include "test_input_machine.h"
#include "test_hooks.h"
#include "macros/keyid_parser.h"
#include "macros/shortcut_parser.h"
#include "str_utils.h"
#include "key_action.h"
#include "keymap.h"
#include "timer.h"
#include "usb_interfaces/usb_interface_basic_keyboard.h"
#include "logger.h"

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
static uint32_t delayStartTime = 0;

// Key ID parsing: convert string like "u" or "leftShift" to slot + keyId
static bool parseKeyId(const char *keyIdStr, uint8_t *slotId, uint8_t *keyId) {
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

// Build expected report from space-separated shortcut string and validate against actual
// If logFailure is true, logs details on mismatch
static bool validateReport(const char *expectShortcuts, bool logFailure) {
    const usb_basic_keyboard_report_t *actual = ActiveUsbBasicKeyboardReport;

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

        key_action_t keyAction = { 0 };
        if (!MacroShortcutParser_Parse(at, shortcutEnd, MacroSubAction_Tap, NULL, &keyAction)) {
            if (logFailure) LogU("[TEST] FAIL: invalid shortcut in '%s'\n", expectShortcuts);
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
        if (!UsbBasicKeyboard_ContainsScancode(actual, expectedScancodes[i])) match = false;
    }
    if (UsbBasicKeyboard_ScancodeCount(actual) != scancodeCount) match = false;

    if (!match && logFailure) {
        LogU("[TEST] <   FAIL: Expect '%s', got mods=0x%02x count=%d\n",
            expectShortcuts, actual->modifiers, (int)UsbBasicKeyboard_ScancodeCount(actual));
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
    delayStartTime = 0;
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
                if (parseKeyId(action->keyId, &slotId, &keyId)) {
                    TestHooks_KeyStates[slotId][keyId] = true;
                    LogU("[TEST] > Press '%s'\n", action->keyId);
                } else {
                    LogU("[TEST] FAIL: Press '%s' - invalid key\n", action->keyId);
                    InputMachine_Failed = true;
                    return;
                }
                InputMachine_ActionIndex++;
                return;
            }

            case TestAction_Release: {
                uint8_t slotId, keyId;
                if (parseKeyId(action->keyId, &slotId, &keyId)) {
                    TestHooks_KeyStates[slotId][keyId] = false;
                    LogU("[TEST] > Release '%s'\n", action->keyId);
                } else {
                    LogU("[TEST] FAIL: Release '%s' - invalid key\n", action->keyId);
                    InputMachine_Failed = true;
                    return;
                }
                InputMachine_ActionIndex++;
                return;
            }

            case TestAction_Delay:
                if (!inDelay) {
                    inDelay = true;
                    delayStartTime = Timer_GetCurrentTime();
                    return;
                } else {
                    if (Timer_GetElapsedTime(&delayStartTime) >= action->delayMs) {
                        LogU("[TEST] > Delay %dms\n", action->delayMs);
                        inDelay = false;
                        InputMachine_ActionIndex++;
                        break;
                    }
                    return;
                }

            case TestAction_SetAction: {
                uint8_t slotId, keyId;
                if (!parseKeyId(action->keyId, &slotId, &keyId)) {
                    LogU("[TEST] FAIL: SetAction '%s' - invalid key\n", action->keyId);
                    InputMachine_Failed = true;
                    return;
                }

                const char *shortcut = action->shortcutStr;
                const char *shortcutEnd = shortcut;
                while (*shortcutEnd != '\0') shortcutEnd++;

                key_action_t keyAction = { 0 };
                if (!MacroShortcutParser_Parse(shortcut, shortcutEnd, MacroSubAction_Tap, NULL, &keyAction)) {
                    LogU("[TEST] FAIL: SetAction '%s' = '%s' - invalid shortcut\n", action->keyId, action->shortcutStr);
                    InputMachine_Failed = true;
                    return;
                }

                CurrentKeymap[LayerId_Base][slotId][keyId] = keyAction;
                LogU("[TEST] > SetAction '%s' = '%s'\n", action->keyId, action->shortcutStr);
                InputMachine_ActionIndex++;
                break;
            }

            case TestAction_CheckNow:
                if (!validateReport(action->expectShortcuts, true)) {
                    InputMachine_Failed = true;
                    return;
                }
                LogU("[TEST] <   CheckNow '%s' - Ok\n", action->expectShortcuts);
                InputMachine_ActionIndex++;
                break;

            case TestAction_Expect:
                // OutputMachine handles this
                InputMachine_ActionIndex++;
                break;

            case TestAction_End:
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
