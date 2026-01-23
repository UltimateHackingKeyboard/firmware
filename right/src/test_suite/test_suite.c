#include "test_suite.h"
#include "test_hooks.h"
#include "test_actions.h"
#include <string.h>

// Test hooks state
bool TestHooks_Active = false;
bool TestHooks_KeyStates[SLOT_COUNT][MAX_KEY_COUNT_PER_MODULE];

void TestHooks_CaptureReport(const usb_basic_keyboard_report_t *report) {
    // TODO: store report in circular buffer for validation
    (void)report;
}

void TestHooks_Tick(void) {
    // TODO: advance test state machine
}

void TestSuite_Init(void) {
    TestHooks_Active = false;
    memset(TestHooks_KeyStates, 0, sizeof(TestHooks_KeyStates));
}

uint8_t TestSuite_RunAll(void) {
    // TODO: run all registered tests
    return 0;
}
