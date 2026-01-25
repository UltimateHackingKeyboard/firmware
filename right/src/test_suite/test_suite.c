#include "test_suite.h"
#include "test_hooks.h"
#include "test_actions.h"
#include "test_input_machine.h"
#include "test_output_machine.h"
#include "tests/tests.h"
#include "logger.h"
#include "timer.h"

#define INTER_TEST_DELAY_MS 100

// Test hooks state
bool TestHooks_Active = false;

// Test tracking
static uint16_t currentTestIndex = 0;
static uint16_t passedCount = 0;
static uint16_t failedCount = 0;

// Inter-test delay state
static bool inInterTestDelay = false;
static uint32_t interTestDelayStart = 0;

void TestHooks_CaptureReport(const usb_basic_keyboard_report_t *report) {
    if (!TestHooks_Active) {
        return;
    }
    OutputMachine_OnReportChange(report);
}

void TestHooks_Tick(void) {
    if (!TestHooks_Active) {
        return;
    }

    // Handle inter-test delay
    if (inInterTestDelay) {
        if (Timer_GetElapsedTime(&interTestDelayStart) >= INTER_TEST_DELAY_MS) {
            inInterTestDelay = false;
            const test_t *nextTest = &AllTests[currentTestIndex];
            LogU("[TEST] ----------------------\n");
            LogU("[TEST] Running: %s\n", nextTest->name);
            InputMachine_Start(nextTest);
            OutputMachine_Start(nextTest);
        }
        return;
    }

    InputMachine_Tick();

    // Check for completion or failure
    bool inputDone = InputMachine_IsDone();
    bool outputDone = OutputMachine_IsDone();
    bool failed = InputMachine_Failed || OutputMachine_Failed;
    bool timedOut = InputMachine_TimedOut && !outputDone;

    if (inputDone && (outputDone || timedOut || failed)) {
        const test_t *test = &AllTests[currentTestIndex];

        if (failed) {
            LogU("[TEST] Finished: %s - FAIL\n", test->name);
            failedCount++;
        } else if (timedOut) {
            LogU("[TEST] Finished: %s - TIMEOUT\n", test->name);
            failedCount++;
        } else {
            LogU("[TEST] Finished: %s - PASS\n", test->name);
            passedCount++;
        }

        // Move to next test
        currentTestIndex++;
        if (currentTestIndex < AllTestsCount) {
            // Start inter-test delay
            inInterTestDelay = true;
            interTestDelayStart = Timer_GetCurrentTime();
        } else {
            LogU("[TEST] ----------------------\n");
            LogU("[TEST] Complete: %d passed, %d failed\n", passedCount, failedCount);
            TestHooks_Active = false;
        }
    }
}

void TestSuite_Init(void) {
    TestHooks_Active = false;
}

uint8_t TestSuite_RunAll(void) {
    currentTestIndex = 0;
    passedCount = 0;
    failedCount = 0;
    inInterTestDelay = false;

    LogU("[TEST] Starting test suite (%d tests)\n", AllTestsCount);

    if (AllTestsCount == 0) {
        return 0;
    }

    // Start first test
    const test_t *firstTest = &AllTests[0];
    LogU("[TEST] ----------------------\n");
    LogU("[TEST] Running: %s\n", firstTest->name);
    InputMachine_Start(firstTest);
    OutputMachine_Start(firstTest);
    TestHooks_Active = true;

    return AllTestsCount;
}
