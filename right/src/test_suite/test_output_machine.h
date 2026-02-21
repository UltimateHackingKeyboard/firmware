#ifndef __TEST_OUTPUT_MACHINE_H__
#define __TEST_OUTPUT_MACHINE_H__

#include <stdint.h>
#include <stdbool.h>
#include "test_actions.h"
#include "hid/keyboard_report.h"

// OutputMachine state
extern const test_t *OutputMachine_CurrentTest;
extern uint16_t OutputMachine_ActionIndex;
extern bool OutputMachine_Failed;

// Initialize the output machine for a new test
void OutputMachine_Start(const test_t *test);

// Process a report change - called from USB send hook
// Validates Expect and CheckNow actions
// Skips (advances past) input actions
void OutputMachine_OnReportChange(const hid_keyboard_report_t *report);

// Check if test is complete (reached End or failed)
bool OutputMachine_IsDone(void);

#endif
