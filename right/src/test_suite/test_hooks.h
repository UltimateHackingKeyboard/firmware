#ifndef __TEST_HOOKS_H__
#define __TEST_HOOKS_H__

#include <stdbool.h>
#include "usb_interfaces/usb_interface_basic_keyboard.h"

// True when test suite is running. Check this in:
// - USB send: call TestHooks_CaptureReport() before sending
extern bool TestHooks_Active;

// Called by USB code to capture reports for validation
void TestHooks_CaptureReport(const usb_basic_keyboard_report_t *report);

// Called each update cycle to advance the test state machine
void TestHooks_Tick(void);

#endif
