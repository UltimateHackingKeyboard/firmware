#include "test_suite.h"
#include "test_hooks.h"
#include "test_actions.h"
#include "test_input_machine.h"
#include "test_output_machine.h"
#include "tests/tests.h"
#include "logger.h"
#include "timer.h"
#include "usb_interfaces/usb_interface_basic_keyboard.h"
#include "keymap.h"
#include "config_manager.h"

#define INTER_TEST_DELAY___MS 100

// Test hooks state
bool TestHooks_Active = false;
bool TestSuite_Verbose = false;

// Test tracking
static uint16_t currentModuleIndex = 0;
static uint16_t currentTestIndex = 0;
static uint16_t totalTestCount = 0;
static uint16_t passedCount = 0;
static uint16_t failedCount = 0;

// Single test mode
static bool singleTestMode = false;

// Rerun state for failed tests
static bool isRerunning = false;
static uint16_t rerunModuleIndex = 0;
static uint16_t rerunTestIndex = 0;

// Inter-test delay state
static bool inInterTestDelay = false;
static uint32_t interTestDelayStart = 0;

static const test_t* getCurrentTest(void) {
    return &AllTestModules[currentModuleIndex]->tests[currentTestIndex];
}

static bool advanceToNextTest(void) {
    currentTestIndex++;
    if (currentTestIndex >= AllTestModules[currentModuleIndex]->testCount) {
        currentModuleIndex++;
        currentTestIndex = 0;
    }
    return currentModuleIndex < AllTestModulesCount;
}

static void startTest(const test_t *test, const test_module_t *module) {
    ConfigManager_ResetConfiguration(false);
    if (TestSuite_Verbose) {
        LogU("[TEST] ----------------------\n");
        LogU("[TEST] Running: %s/%s\n", module->name, test->name);
    }
    InputMachine_Start(test);
    OutputMachine_Start(test);
    OutputMachine_OnReportChange(ActiveUsbBasicKeyboardReport);
}

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
        if (Timer_GetElapsedTime(&interTestDelayStart) >= INTER_TEST_DELAY___MS) {
            inInterTestDelay = false;
            const test_t *nextTest = getCurrentTest();
            const test_module_t *module = AllTestModules[currentModuleIndex];
            startTest(nextTest, module);
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
        const test_t *test = getCurrentTest();
        const test_module_t *module = AllTestModules[currentModuleIndex];

        if (failed || timedOut) {
            if (isRerunning || singleTestMode) {
                // Already rerunning with verbose (or single test mode), log final result
                if (failed) {
                    LogU("[TEST] Finished: %s/%s - FAIL\n", module->name, test->name);
                } else {
                    LogU("[TEST] Finished: %s/%s - TIMEOUT\n", module->name, test->name);
                }
                LogU("[TEST] ----------------------\n");
                failedCount++;
                isRerunning = false;
                TestSuite_Verbose = false;  // Reset to non-verbose for remaining tests

                if (singleTestMode) {
                    goto finish;
                }

                // Continue from where we left off
                currentModuleIndex = rerunModuleIndex;
                currentTestIndex = rerunTestIndex;
                if (advanceToNextTest()) {
                    inInterTestDelay = true;
                    interTestDelayStart = Timer_GetCurrentTime();
                } else {
                    goto finish;
                }
            } else {
                // First failure - save position and rerun with verbose
                rerunModuleIndex = currentModuleIndex;
                rerunTestIndex = currentTestIndex;
                isRerunning = true;
                TestSuite_Verbose = true;

                inInterTestDelay = true;
                interTestDelayStart = Timer_GetCurrentTime();
            }
        } else {
            LogU("[TEST] Finished: %s/%s - PASS\n", module->name, test->name);
            passedCount++;
            if (isRerunning) {
                isRerunning = false;
                TestSuite_Verbose = false;  // Reset to non-verbose for remaining tests
            }

            if (singleTestMode) {
                goto finish;
            }

            // Move to next test
            if (advanceToNextTest()) {
                inInterTestDelay = true;
                interTestDelayStart = Timer_GetCurrentTime();
            } else {
                goto finish;
            }
        }
    }
    return;

finish:
    LogU("[TEST] ----------------------\n");
    LogU("[TEST] Complete: %d passed, %d failed\n", passedCount, failedCount);
    TestHooks_Active = false;
    ConfigManager_ResetConfiguration(false);
}

void TestSuite_Init(void) {
    TestHooks_Active = false;
}

uint8_t TestSuite_RunAll(void) {
    currentModuleIndex = 0;
    currentTestIndex = 0;
    passedCount = 0;
    failedCount = 0;
    inInterTestDelay = false;
    isRerunning = false;
    singleTestMode = false;
    TestSuite_Verbose = false;

    // Count total tests
    totalTestCount = 0;
    for (uint16_t i = 0; i < AllTestModulesCount; i++) {
        totalTestCount += AllTestModules[i]->testCount;
    }

    LogU("[TEST] Starting test suite (%d tests in %d modules)\n", totalTestCount, AllTestModulesCount);

    if (totalTestCount == 0) {
        return 0;
    }

    // Start first test
    const test_t *firstTest = getCurrentTest();
    const test_module_t *module = AllTestModules[currentModuleIndex];
    startTest(firstTest, module);
    TestHooks_Active = true;

    return totalTestCount;
}

static bool streq(const char *a, const char *aEnd, const char *b) {
    while (a < aEnd && *b) {
        if (*a++ != *b++) return false;
    }
    return a == aEnd && *b == '\0';
}

uint8_t TestSuite_RunSingle(const char *moduleStart, const char *moduleEnd, const char *testStart, const char *testEnd) {
    // Find the module and test
    for (uint16_t mi = 0; mi < AllTestModulesCount; mi++) {
        const test_module_t *module = AllTestModules[mi];
        if (!streq(moduleStart, moduleEnd, module->name)) continue;

        for (uint16_t ti = 0; ti < module->testCount; ti++) {
            const test_t *test = &module->tests[ti];
            if (!streq(testStart, testEnd, test->name)) continue;

            // Found it - run with verbose logging
            currentModuleIndex = mi;
            currentTestIndex = ti;
            passedCount = 0;
            failedCount = 0;
            inInterTestDelay = false;
            isRerunning = false;
            singleTestMode = true;
            TestSuite_Verbose = true;  // Always verbose for single test

            LogU("[TEST] Running single test: %s/%s\n", module->name, test->name);
            startTest(test, module);
            TestHooks_Active = true;

            return 0;
        }
    }

    LogU("[TEST] Test not found: %.*s/%.*s\n",
        (int)(moduleEnd - moduleStart), moduleStart,
        (int)(testEnd - testStart), testStart);
    return 255;
}
