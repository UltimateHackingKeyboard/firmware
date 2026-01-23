#ifndef __TEST_HOOKS_H__
#define __TEST_HOOKS_H__

#include <stdbool.h>
#include "slot.h"
#include "module.h"
#include "usb_interfaces/usb_interface_basic_keyboard.h"

// True when test suite is running. Check this in:
// - Key scanner: skip hardware scan, use TestHooks_KeyStates instead
// - USB send: call TestHooks_CaptureReport() before sending
extern bool TestHooks_Active;

// Synthetic key states set by tests (indexed like KeyStates)
extern bool TestHooks_KeyStates[SLOT_COUNT][MAX_KEY_COUNT_PER_MODULE];

// Called by USB code to capture reports for validation
void TestHooks_CaptureReport(const usb_basic_keyboard_report_t *report);

// Called by key scanner to advance the test state machine
void TestHooks_Tick(void);

#endif
