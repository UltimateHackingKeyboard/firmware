#ifndef __TEST_INPUT_MACHINE_H__
#define __TEST_INPUT_MACHINE_H__

#include <stdint.h>
#include <stdbool.h>
#include "test_actions.h"

// InputMachine state
extern const test_t *InputMachine_CurrentTest;
extern uint16_t InputMachine_ActionIndex;
extern bool InputMachine_Failed;
extern bool InputMachine_TimedOut;

// Initialize the input machine for a new test
void InputMachine_Start(const test_t *test);

// Resolves a test-script key id string to slot/key. Shared with the left half's link-test
// producer, so both sides read the same script identically.
bool TestInput_ParseKeyId(const char *keyIdStr, uint8_t *slotId, uint8_t *keyId);

// Process one tick - called from scanner
// Processes Press, Release, Delay, SetAction, CheckNow
// Skips (advances past) Expect
// Stops at End
void InputMachine_Tick(void);

// Check if test is complete (reached End or failed)
bool InputMachine_IsDone(void);

#endif
