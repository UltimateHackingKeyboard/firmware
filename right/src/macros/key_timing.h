#ifndef __KEY_TIMING_H__
#define __KEY_TIMING_H__

// Includes:

#include "key_states.h"
#include "usb_interfaces/usb_interface_basic_keyboard.h"

// Macros:

#define KEY_TIMING(code) if (RecordKeyTiming) { code; }

#define KEY_TIMING2(condition, code) if (RecordKeyTiming && condition) { code; }

// Typedefs:


// Variables:

    extern bool RecordKeyTiming;

// Functions:

void KeyTiming_RecordKeystroke(key_state_t *keyState, bool active, uint32_t pressTime, uint32_t activationTime);
void KeyTiming_RecordReport(usb_basic_keyboard_report_t* report);
void KeyTiming_RecordComment(key_state_t* keyState, const char* comment);

#endif
