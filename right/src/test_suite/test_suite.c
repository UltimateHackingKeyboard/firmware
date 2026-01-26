#include "test_suite.h"
#include "test_hooks.h"
#include "test_actions.h"
#include "test_input_machine.h"
#include "test_output_machine.h"
#include "tests/tests.h"
#include "logger.h"
#include "timer.h"
#include "usb_interfaces/usb_interface_basic_keyboard.h"

#define INTER_TEST_DELAY_MS 100

// Test hooks state
bool TestHooks_Active = false;

// Test tracking
static uint16_t currentModuleIndex = 0;
static uint16_t currentTestIndex = 0;
static uint16_t totalTestCount = 0;
static uint16_t passedCount = 0;
static uint16_t failedCount = 0;

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
            const test_t *nextTest = getCurrentTest();
            const test_module_t *module = AllTestModules[currentModuleIndex];
            LogU("[TEST] ----------------------\n");
            LogU("[TEST] Running: %s/%s\n", module->name, nextTest->name);
            InputMachine_Start(nextTest);
            OutputMachine_Start(nextTest);
            // Run one iteration with current report so initial state can be validated
            OutputMachine_OnReportChange(ActiveUsbBasicKeyboardReport);
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

        if (failed) {
            LogU("[TEST] Finished: %s/%s - FAIL\n", module->name, test->name);
            failedCount++;
        } else if (timedOut) {
            LogU("[TEST] Finished: %s/%s - TIMEOUT\n", module->name, test->name);
            failedCount++;
        } else {
            LogU("[TEST] Finished: %s/%s - PASS\n", module->name, test->name);
            passedCount++;
        }

        // Move to next test
        if (advanceToNextTest()) {
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
    currentModuleIndex = 0;
    currentTestIndex = 0;
    passedCount = 0;
    failedCount = 0;
    inInterTestDelay = false;

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
    LogU("[TEST] ----------------------\n");
    LogU("[TEST] Running: %s/%s\n", module->name, firstTest->name);
    InputMachine_Start(firstTest);
    OutputMachine_Start(firstTest);
    // Run one iteration with current report so initial state can be validated
    OutputMachine_OnReportChange(ActiveUsbBasicKeyboardReport);
    TestHooks_Active = true;

    return totalTestCount;
}
