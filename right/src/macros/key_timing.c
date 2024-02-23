#include "macros/key_timing.h"
#include "key_states.h"
#include "macros/status_buffer.h"
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
    Macros_SetStatusNumSpaced(CurrentTime, false);
    Utils_PrintReport(" OUT", ActiveUsbBasicKeyboardReport);
}

void KeyTiming_RecordComment(key_state_t* keyState, const char* comment)
{
    const char* keyAbbreviation = Utils_KeyAbbreviation(keyState);

    Macros_SetStatusNumSpaced(CurrentTime, false);
    Macros_SetStatusChar(' ');
    Macros_SetStatusString(keyAbbreviation, NULL);
    Macros_SetStatusChar(' ');
    Macros_SetStatusString(comment, NULL);
    Macros_SetStatusChar('\n');
}

