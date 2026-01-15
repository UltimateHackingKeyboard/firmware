#include "macros/key_timing.h"
#include "key_states.h"
#include "macros/status_buffer.h"
#include "secondary_role_driver.h"
#include "utils.h"
#include "timer.h"

bool RecordKeyTiming = false;

void KeyTiming_RecordKeystroke(key_state_t *keyState, bool active, uint32_t pressTime, uint32_t activationTime)
{
    const char* keyAbbreviation = Utils_KeyAbbreviation(keyState);
    Macros_SetStatusString( active ? "DOWN" : "UP", NULL);
    Macros_SetStatusChar(' ');
    Macros_SetStatusString(keyAbbreviation, NULL);
    Macros_SetStatusNum(pressTime);
    Macros_SetStatusNum(activationTime);
    Macros_SetStatusChar('\n');
}

void KeyTiming_RecordReport(usb_basic_keyboard_report_t* report)
{
    Macros_SetStatusNumSpaced(Timer_GetCurrentTime(), false);
    Utils_PrintReport(" OUT", ActiveUsbBasicKeyboardReport);
}

void KeyTiming_RecordComment(key_state_t* keyState, secondary_role_state_t state, int32_t resolutionLine)
{
    const char* keyAbbreviation = Utils_KeyAbbreviation(keyState);

    Macros_SetStatusNumSpaced(Timer_GetCurrentTime(), false);
    Macros_SetStatusChar(' ');
    Macros_SetStatusString(keyAbbreviation, NULL);
    Macros_SetStatusChar(' ');
    switch (state) {
    case SecondaryRoleState_Primary:
        Macros_SetStatusChar('P');
        break;
    case SecondaryRoleState_Secondary:
        Macros_SetStatusChar('S');
        break;
    case SecondaryRoleState_NoOp:
        Macros_SetStatusChar('N');
        break;
    case SecondaryRoleState_DontKnowYet:
        Macros_SetStatusChar('D');
        break;
    }
    Macros_SetStatusChar(':');
    Macros_SetStatusNumSpaced(resolutionLine, false);
    Macros_SetStatusChar('\n');
 
}

