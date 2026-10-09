#ifndef __TEST_HOOKS_H__
#define __TEST_HOOKS_H__

#include <stdbool.h>
#include "hid/keyboard_report.h"

// True when test suite is running. Check this in:
// - USB send: call TestHooks_CaptureReport() before sending
extern bool TestHooks_Active;

// Called by USB code to capture reports for validation
void TestHooks_CaptureReport(const hid_keyboard_report_t *report);

// Called each update cycle to advance the test state machine
void TestHooks_Tick(void);

// Joins a link test the peer has started, or stops the running one when given 0. Called from
// the bridge's receive path, so it must only flip state.
void TestHooks_StartLinkTest(uint8_t linkTestId);

#endif
