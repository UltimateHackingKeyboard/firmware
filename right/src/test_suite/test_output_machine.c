#include "test_output_machine.h"
#include "usb_interfaces/usb_interface_basic_keyboard.h"
#include "macros/shortcut_parser.h"
#include "key_action.h"
#include "logger.h"
#include <string.h>

// OutputMachine state
const test_t *OutputMachine_CurrentTest = NULL;
uint16_t OutputMachine_ActionIndex = 0;
bool OutputMachine_Failed = false;

static int16_t lastSeenActionIndex = -1;

// Validate report against space-separated shortcut string
// If logFailure is true, logs details on mismatch
static bool validateReport(const usb_basic_keyboard_report_t *actual, const char *expectShortcuts, bool logFailure) {
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

void OutputMachine_Start(const test_t *test) {
    OutputMachine_CurrentTest = test;
    OutputMachine_ActionIndex = 0;
    OutputMachine_Failed = false;
    lastSeenActionIndex = -1;
}

void OutputMachine_OnReportChange(const usb_basic_keyboard_report_t *report) {
    if (OutputMachine_CurrentTest == NULL || OutputMachine_Failed) {
        return;
    }

    // Skip past input actions to find next Expect/CheckNow
    while (true) {
        const test_action_t *action = &OutputMachine_CurrentTest->actions[OutputMachine_ActionIndex];

        switch (action->type) {
            case TestAction_Press:
            case TestAction_Release:
            case TestAction_Delay:
            case TestAction_SetAction:
            case TestAction_SetMacro:
                OutputMachine_ActionIndex++;
                break;

            case TestAction_CheckNow:
            case TestAction_Expect:
                // Try to validate against current expect
                if (validateReport(report, action->expectShortcuts, false)) {
                    // Match - continue processing (there may be more expects matching this report)
                    LogU("[TEST] <   Ok - Expect '%s'\n", action->expectShortcuts);
                    lastSeenActionIndex = OutputMachine_ActionIndex;
                    OutputMachine_ActionIndex++;
                    break;  // Continue loop to check next action
                }
                // No match - check if this is a duplicate of last seen
                if (lastSeenActionIndex >= 0) {
                    const test_action_t *lastSeenAction = &OutputMachine_CurrentTest->actions[lastSeenActionIndex];
                    if (validateReport(report, lastSeenAction->expectShortcuts, false)) {
                        return;  // Duplicate of last seen, ignore
                    }
                }
                // Not a match and not a duplicate - fail
                validateReport(report, action->expectShortcuts, true);  // Log the failure
                OutputMachine_Failed = true;
                return;

            case TestAction_End:
                return;
        }
    }
}

bool OutputMachine_IsDone(void) {
    if (OutputMachine_CurrentTest == NULL) {
        return true;
    }
    if (OutputMachine_Failed) {
        return true;
    }
    bool atEnd = OutputMachine_CurrentTest->actions[OutputMachine_ActionIndex].type == TestAction_End;
    return atEnd;
}
