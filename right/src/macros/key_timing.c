#include "macros/key_timing.h"
#include "key_states.h"
#include "logger.h"
#include "secondary_role_driver.h"
#include "utils.h"
#include "timer.h"

bool RecordKeyTiming = false;

static log_target_t defaultTarget = LogTarget_Uart | LogTarget_ErrorBuffer;

void KeyTiming_RecordKeystroke(key_state_t *keyState, bool active, uint32_t pressTime, uint32_t activationTime)
{
    const char* keyAbbreviation = Utils_KeyAbbreviation(keyState);
    LogTo(DEVICE_ID, defaultTarget, "%s %s %d %d\n",
        active ? "DOWN" : "UP", keyAbbreviation, pressTime, activationTime);
}

extern hid_keyboard_report_t *ActiveKeyboardReport;

void KeyTiming_RecordReport(hid_keyboard_report_t* report)
{
    LogTo(DEVICE_ID, defaultTarget, "%d OUT %s\n",
        Timer_GetCurrentTime(), Utils_GetUsbReportString(ActiveKeyboardReport));
}

void KeyTiming_RecordComment(key_state_t* keyState, secondary_role_state_t state, int32_t resolutionLine)
{
    const char* keyAbbreviation = Utils_KeyAbbreviation(keyState);

    char stateChar;
    switch (state) {
    case SecondaryRoleState_Primary:
        stateChar = 'P';
        break;
    case SecondaryRoleState_Secondary:
        stateChar = 'S';
        break;
    case SecondaryRoleState_NoOp:
        stateChar = 'N';
        break;
    case SecondaryRoleState_DontKnowYet:
        stateChar = 'D';
        break;
    default:
        stateChar = '?';
        break;
    }
    LogTo(DEVICE_ID, defaultTarget, "%d %s %c:%d\n",
        Timer_GetCurrentTime(), keyAbbreviation, stateChar, resolutionLine);
}
