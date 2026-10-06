#include "test_suite.h"
#include "messenger.h"
#include "device.h"
#include "test_hooks.h"
#include "test_actions.h"
#include "test_input_machine.h"
#include "test_statistics.h"
#include "test_output_machine.h"
#include "tests/tests.h"
#include "logger.h"
#include "timer.h"
#include "keymap.h"
#include "config_manager.h"
#include "macros/core.h"
#include "macros/vars.h"
#include "mouse_controller.h"
#include "layer_stack.h"
#include "postponer.h"
#include "slot.h"
#include "key_states.h"
#include "module.h"
#include <string.h>

#if defined(__ZEPHYR__) && DEVICE_IS_UHK80_LEFT
#include "link_protocol.h"
#include "connections.h"
#include "bool_array_converter.h"
#include <zephyr/sys/atomic.h>
#endif

#if defined(__ZEPHYR__) && DEVICE_IS_KEYBOARD
#include "keyboard/battery_unloaded_calculator.h"
#include "keyboard/battery_percent_calculator.h"
#endif


#define INTER_TEST_DELAY___MS 100

// Test hooks state
bool TestHooks_Active = false;
bool TestSuite_Verbose = false;

// Test tracking
static uint16_t currentModuleIndex = 0;
static uint16_t currentTestIndex = 0;
static uint16_t totalTestCount = 0;
static uint16_t passedCount = 0;
static uint16_t partialCount = 0;
static uint16_t failedCount = 0;

// Module-scoped run limit
static uint16_t lastModuleIndexExclusive = 0;

// Single test mode
static bool singleTestMode = false;

// Rerun state for failed tests
static bool isRerunning = false;

// Inter-test delay state
static bool inInterTestDelay = false;
static uint32_t interTestDelayStart = 0;

// Whether the current test (including its rerun) has opened its log with a separator
static bool separatorPrinted = false;

void TestSuite_LogSeparatorOnce(void) {
    if (!separatorPrinted) {
        LogU("[TEST] ----------------------\n");
        separatorPrinted = true;
    }
}

static void closeTestLog(void) {
    if (separatorPrinted) {
        LogU("[TEST] ----------------------\n");
    }
}

static const test_t* getCurrentTest(void) {
    return &AllTestModules[currentModuleIndex]->tests[currentTestIndex];
}

static bool advanceToNextTest(void) {
    currentTestIndex++;
    if (currentTestIndex >= AllTestModules[currentModuleIndex]->testCount) {
        currentModuleIndex++;
        currentTestIndex = 0;
    }
    return currentModuleIndex < lastModuleIndexExclusive;
}

extern hid_keyboard_report_t *ActiveKeyboardReport;

// Repeats are re-runs of the whole test, not a wrap of the action index, so that each pass
// re-sends StartTest to the left half. Both halves therefore restart their timelines together
// every pass; wrapping the index instead let their independent delay timers drift apart over
// hundreds of steps, until a left change landed after a right one scripted later.
static const test_t *repeatingTest = NULL;
static uint16_t repeatsLeft = 0;
static uint16_t repetitionsPassed = 0;
static uint16_t repetitionsFailed = 0;

static void startTest(const test_t *test, const test_module_t *module) {
    if (test != repeatingTest) {
        repeatingTest = test;
        repeatsLeft = test->repeatCount > 0 ? test->repeatCount - 1 : 0;
        repetitionsPassed = 0;
        repetitionsFailed = 0;
    }

    Macros_StopAllMacros();
    ConfigManager_ResetConfiguration(false, false);
    LayerStack_Reset();
    PostponerExtended_ResetPostponer();
    if (!isRerunning) {
        separatorPrinted = false;
    }
    if (TestSuite_Verbose) {
        TestSuite_LogSeparatorOnce();
        LogU("[TEST] Running: %s/%s\n", module->name, test->name);
    }
    if (DEVICE_IS_UHK80_RIGHT && test->linkTestId != 0) {
        Messenger_Send2(DeviceId_Uhk80_Left, MessageId_Command, MessengerCommand_StartTest, &test->linkTestId, sizeof(test->linkTestId));
    }

    InputMachine_Start(test);
    OutputMachine_Start(test);
    OutputMachine_OnReportChange(ActiveKeyboardReport);
}

void TestHooks_CaptureReport(const hid_keyboard_report_t *report) {
    if (!TestHooks_Active) {
        return;
    }
    OutputMachine_OnReportChange(report);
}

// True once the inter-test pause has elapsed and the next test has been started.
static bool waitOutInterTestDelay(void) {
    if (Timer_GetElapsedTime(&interTestDelayStart) < INTER_TEST_DELAY___MS) {
        return false;
    }

    inInterTestDelay = false;
    const test_t *nextTest = getCurrentTest();
    const test_module_t *module = AllTestModules[currentModuleIndex];
    startTest(nextTest, module);
    return true;
}

#if defined(__ZEPHYR__) && DEVICE_IS_UHK80_LEFT
// The left half's whole contribution to a link test: hand the keys the shared interpreter has
// just pressed to the right half, in place of what scanAllKeys would have sent.
//
// On change only, exactly as scanAllKeys does. Sending every tick would make the sender block
// on txBufferBusy continuously, which both floods the link and hides the thing under test -
// the stall only matters when it delays the *next* state change.
//
// Pinned to the UART connection rather than left to determineChannel: with BLE to the right
// also up the default route can carry this over NUS instead, and the test would then be
// exercising the wrong link.
static void publishOwnKeyStates(void) {
    static uint8_t previous[MAX_KEY_COUNT_PER_MODULE/8+1];
    uint8_t length = MAX_KEY_COUNT_PER_MODULE/8+1;
    uint8_t bitmap[length];
    memset(bitmap, 0, length);

    for (uint8_t keyId = 0; keyId < MAX_KEY_COUNT_PER_MODULE; keyId++) {
        if (KeyStates[CURRENT_SLOT_ID][keyId].hardwareSwitchState) {
            BoolBitToBytes(true, keyId, bitmap);
        }
    }

    bool changed = memcmp(bitmap, previous, length) != 0;

    if (changed) {
        memcpy(previous, bitmap, length);
        Messenger_Send2Via(DeviceId_Uhk80_Right, ConnectionId_UartRight,
                MessageId_SyncableProperty, SyncablePropertyId_LeftHalfKeyStates, bitmap, length);
    }
}

// Set while the left half plays its part in a link test the right half is running. Otherwise
// the left half runs whatever suite it was asked to, like any other device.
static bool isLinkTestPeer = false;

// StartTest arrives on the messenger's receive path, while the input machine is ticked by the
// key scanner. The right restarts the left just as the left finishes its previous run, so
// starting from the receive path raced the finish: the scanner, having just seen the old run
// end, switched the freshly started one off and the left sat out the repetition. The receive
// path only posts the id; the scanner thread does the starting.
static atomic_t pendingLinkTestId = ATOMIC_INIT(0);

static void startPendingLinkTest(void) {
    uint8_t linkTestId = (uint8_t)atomic_set(&pendingLinkTestId, 0);

    if (linkTestId != 0) {
        const test_t *test = Tests_FindLinkTest(linkTestId);
        bool known = test != NULL;

        if (known) {
            InputMachine_Start(test);
        }
        isLinkTestPeer = known;
    }
}

// The left half has no USB reports to validate, so it runs the input machine only.
static void tickLinkTestPeer(void) {
    startPendingLinkTest();

    if (isLinkTestPeer) {
        InputMachine_Tick();
        publishOwnKeyStates();

        if (InputMachine_IsDone()) {
            isLinkTestPeer = false;
        }
    }

    // Re-checked after clearing, so a StartTest landing in between keeps the scanner coming.
    TestHooks_Active = isLinkTestPeer;
    if (atomic_get(&pendingLinkTestId) != 0) {
        TestHooks_Active = true;
    }
}
#endif

typedef enum {
    TestOutcome_Running,
    TestOutcome_Passed,
    TestOutcome_Failed,
    TestOutcome_TimedOut,
} test_outcome_t;

// Whether the current test (or repetition) has ended, and how. With input and output both done
// but macros still running it waits for them: they end, or the input machine times out.
static test_outcome_t evaluateCurrentTest(void) {
    bool inputDone = InputMachine_IsDone();
    bool outputDone = OutputMachine_IsDone();
    bool failed = InputMachine_Failed || OutputMachine_Failed;
    bool timedOut = InputMachine_TimedOut && !outputDone;
    bool waitingForMacros = outputDone && Macros_AnyMacroRunning() && !timedOut && !failed;
    bool ended = inputDone && (outputDone || timedOut || failed) && !waitingForMacros;

    test_outcome_t outcome = TestOutcome_Running;
    if (ended && failed) {
        outcome = TestOutcome_Failed;
    } else if (ended && timedOut) {
        outcome = TestOutcome_TimedOut;
    } else if (ended) {
        outcome = TestOutcome_Passed;
    }
    return outcome;
}

static void finishSuite(void) {
    Macros_StopAllMacros();
    if (!separatorPrinted) {
        // Otherwise the last test has already closed its log with a separator
        LogU("[TEST] ----------------------\n");
    }
    LogU("[TEST] Complete:\n");
    LogU("[TEST] Discrete tests: %d passed, %d partial, %d failed\n", passedCount, partialCount, failedCount);
    uint16_t repeated = TestStatistics_RepeatedTestCount();
    uint16_t repeatedFailed = TestStatistics_FailedRepeatedTestCount();
    uint16_t repeatedSucceeded = repeated - repeatedFailed;
    if (repeatedFailed > 0) {
        LogU("[TEST] Repeated tests: %u passed, %u failed, %u%% fail confidence\n", (unsigned)repeatedSucceeded,
                (unsigned)repeatedFailed, TestStatistics_AsPercent(TestStatistics_SuiteConfidence()));
    } else if (repeated > 0) {
        LogU("[TEST] Repeated tests: %u passed, %u failed\n", (unsigned)repeatedSucceeded, (unsigned)repeatedFailed);
    }
    LogU("[TEST] ----------------------\n");
    TestHooks_Active = false;
    ConfigManager_ResetConfiguration(false, false);
    LayerStack_Reset();
    PostponerExtended_ResetPostponer();
}

// Closes the test's log and goes on: to the next test after the inter-test delay, or to the
// summary when there is none or only one test was asked for.
static void moveToNextTest(void) {
    closeTestLog();
    bool anotherTest = !singleTestMode && advanceToNextTest();
    if (anotherTest) {
        inInterTestDelay = true;
        interTestDelayStart = Timer_GetCurrentTime();
    } else {
        finishSuite();
    }
}

static void concludeRepeatedTest(const test_t *test, const test_module_t *module) {
    uint16_t repetitions = repetitionsPassed + repetitionsFailed;
    bool anyFailed = repetitionsFailed > 0;
    float confidence = TestStatistics_RepeatedTestConfidence(repetitionsFailed, repetitions);

    if (anyFailed) {
        LogU("[TEST] Finished: %s/%s - FAIL (%u/%u failed, %u%% fail confidence)\n", module->name, test->name,
                (unsigned)repetitionsFailed, (unsigned)repetitions, TestStatistics_AsPercent(confidence));
    } else {
        LogU("[TEST] Finished: %s/%s - PASS\n", module->name, test->name);
    }
    TestStatistics_AddRepeatedTest(confidence, anyFailed);
    repeatingTest = NULL;
    moveToNextTest();
}

// A repeated test is a run of independent trials. Every repetition is played out, a failed
// one is counted rather than ending the run, and nothing is re-run verbosely: a rerun's pass
// would only blur the failure rate the repetitions exist to measure.
static void concludeRepetition(const test_t *test, const test_module_t *module, test_outcome_t outcome) {
    if (outcome == TestOutcome_Passed) {
        repetitionsPassed++;
    } else {
        repetitionsFailed++;
        // A mismatch is logged by the output machine; a timeout would pass silently.
        const test_action_t *pending = &test->actions[OutputMachine_ActionIndex];
        bool pendingIsExpect = pending->type == TestAction_Expect || pending->type == TestAction_CheckNow;
        if (outcome == TestOutcome_TimedOut && pendingIsExpect) {
            LOG_FAILURE("[TEST] <   TIMEOUT: still expecting '%s'\n", pending->expectShortcuts);
        }
    }

    if (repeatsLeft > 0) {
        repeatsLeft--;
        startTest(test, module);
    } else {
        concludeRepeatedTest(test, module);
    }
}

static void endVerboseRerun(void) {
    isRerunning = false;
    TestSuite_Verbose = false;
}

// A test's first failure: run it again, verbosely, to log what went wrong.
static void scheduleVerboseRerun(const test_t *test, const test_module_t *module, const char *failure) {
    LOG_FAILURE("[TEST] Finished: %s/%s - %s (rerunning verbose)\n", module->name, test->name, failure);
    isRerunning = true;
    TestSuite_Verbose = true;
    inInterTestDelay = true;
    interTestDelayStart = Timer_GetCurrentTime();
}

static void concludeTest(const test_t *test, const test_module_t *module, test_outcome_t outcome) {
    const char *failure = outcome == TestOutcome_Failed ? "FAIL" : "TIMEOUT";
    bool lastAttempt = isRerunning || singleTestMode;

    if (outcome == TestOutcome_Passed && isRerunning) {
        LogU("[TEST] Finished: %s/%s - PARTIAL (passed on rerun)\n", module->name, test->name);
        partialCount++;
        endVerboseRerun();
        moveToNextTest();
    } else if (outcome == TestOutcome_Passed) {
        LogU("[TEST] Finished: %s/%s - PASS\n", module->name, test->name);
        passedCount++;
        moveToNextTest();
    } else if (lastAttempt) {
        LOG_FAILURE("[TEST] Finished: %s/%s - %s\n", module->name, test->name, failure);
        failedCount++;
        endVerboseRerun();
        moveToNextTest();
    } else {
        scheduleVerboseRerun(test, module, failure);
    }
}

// One tick of the suite: waits out the gap between tests, or drives the current test and,
// once it has ended, concludes it.
static void stepSuite(void) {
    if (inInterTestDelay) {
        waitOutInterTestDelay();
    } else {
        InputMachine_Tick();
        test_outcome_t outcome = evaluateCurrentTest();

        if (outcome != TestOutcome_Running) {
            const test_t *test = getCurrentTest();
            const test_module_t *module = AllTestModules[currentModuleIndex];
            bool timedOut = InputMachine_TimedOut && !OutputMachine_IsDone();

            if (timedOut) {
                Macros_StopAllMacros();
            }
            if (test->repeatCount > 1) {
                concludeRepetition(test, module, outcome);
            } else {
                concludeTest(test, module, outcome);
            }
        }
    }
}

// Routes to whichever side of a test this device is responsible for. In a link test the right
// half owns the suite - sequencing, expectations, verdicts - while the left only produces keys.
void TestHooks_Tick(void) {
    if (!TestHooks_Active) {
        return;
    }

#if defined(__ZEPHYR__) && DEVICE_IS_UHK80_LEFT
    if (isLinkTestPeer || atomic_get(&pendingLinkTestId) != 0) {
        tickLinkTestPeer();
    } else {
        stepSuite();
    }
#else
    stepSuite();
#endif
}

// Entry point for the StartTest bridge command. Resolving and switching happens here rather
// than in the messenger, which should not know what a test is. A zero or unknown id stops
// whatever was running.
void TestHooks_StartLinkTest(uint8_t linkTestId) {
#if defined(__ZEPHYR__) && DEVICE_IS_UHK80_LEFT
    atomic_set(&pendingLinkTestId, linkTestId);
    TestHooks_Active = true;
#else
    (void)linkTestId;
#endif
}


void TestSuite_Init(void) {
    TestHooks_Active = false;
}

uint8_t TestSuite_RunAll(void) {
    currentModuleIndex = 0;
    currentTestIndex = 0;
    passedCount = 0;
    partialCount = 0;
    failedCount = 0;
    TestStatistics_Reset();
    inInterTestDelay = false;
    isRerunning = false;
    singleTestMode = false;
    lastModuleIndexExclusive = AllTestModulesCount;
    TestSuite_Verbose = false;

    // Count total tests
    totalTestCount = 0;
    for (uint16_t i = 0; i < AllTestModulesCount; i++) {
        totalTestCount += AllTestModules[i]->testCount;
    }

    LogU("[TEST] Running custom unit tests...\n");

    MacroVariables_RunTests();
#if defined(__ZEPHYR__) && DEVICE_IS_KEYBOARD
    BatteryCalculator_RunTests();
    BatteryCalculator_RunPercentTests();
#endif

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

            // Found it - run with verbose logging, unless it is a repeated test: those run
            // hundreds of repetitions, so they log as in the full suite whichever way they are
            // started.
            currentModuleIndex = mi;
            currentTestIndex = ti;
            passedCount = 0;
            partialCount = 0;
            failedCount = 0;
            TestStatistics_Reset();
            inInterTestDelay = false;
            isRerunning = false;
            singleTestMode = true;
            TestSuite_Verbose = test->repeatCount <= 1;

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

static uint8_t TestSuite_RunModule(const char *moduleStart, const char *moduleEnd) {
    for (uint16_t mi = 0; mi < AllTestModulesCount; mi++) {
        const test_module_t *module = AllTestModules[mi];
        if (!streq(moduleStart, moduleEnd, module->name)) {
            continue;
        }

        currentModuleIndex = mi;
        currentTestIndex = 0;
        passedCount = 0;
        partialCount = 0;
        failedCount = 0;
        TestStatistics_Reset();
        inInterTestDelay = false;
        isRerunning = false;
        singleTestMode = false;
        lastModuleIndexExclusive = mi + 1;
        TestSuite_Verbose = false;

        totalTestCount = module->testCount;

        LogU("[TEST] Running module: %s (%d tests)\n", module->name, totalTestCount);

        if (totalTestCount == 0) {
            return 0;
        }

        const test_t *firstTest = getCurrentTest();
        startTest(firstTest, module);
        TestHooks_Active = true;

        return 0;
    }

    LogU("[TEST] Module not found: %.*s\n",
        (int)(moduleEnd - moduleStart), moduleStart);
    return 255;
}

uint8_t TestSuite_Run(string_segment_t module, string_segment_t test) {
    // "all" means run everything
    if (module.start != NULL && streq(module.start, module.end, "all")) {
        module.start = NULL;
    }

    // Support slash notation: "Module/test" as a single argument
    if (module.start != NULL && test.start == NULL) {
        for (const char *p = module.start; p < module.end; p++) {
            if (*p == '/') {
                test.start = p + 1;
                test.end = module.end;
                module.end = p;
                break;
            }
        }
    }

    if (module.start == NULL) {
        return TestSuite_RunAll();
    } else if (test.start == NULL) {
        return TestSuite_RunModule(module.start, module.end);
    } else {
        return TestSuite_RunSingle(module.start, module.end, test.start, test.end);
    }
}
