#include "tests.h"
#include "test_suite/test_hooks.h"

// Tests that key states still arrive when the messenger fails sends. Both halves run the script
// and fail one in TEST_HOOKS_MESSENGER_FAIL_RECIPROCAL of their key state and state sync sends;
// the senders must retry them. Each half presses only the keys it owns, so what the right half
// validates has crossed the link.
//
// A failed send is retried on the next key scan, a millisecond later, so the left keys arrive
// well within a phase. A phase still has to clear debouncing (50 ms).
#define MESSENGER_FAILURE_PHASE_MS 80
#define MESSENGER_FAILURE_STEPS 12
#define MESSENGER_FAILURE_CYCLE_MS (MESSENGER_FAILURE_STEPS * MESSENGER_FAILURE_PHASE_MS)
#define MESSENGER_FAILURE_CYCLES (TEST_REPEAT_FOR_SECONDS * 1000 / MESSENGER_FAILURE_CYCLE_MS)

// Retries are unbounded while the halves are connected, so a healthy run doesn't fail; this is
// only a floor for the statistics. Without the retries nearly every cycle fails.
#define MESSENGER_FAILURE_CYCLE_FAILURE_RATE 0.001

#define STEP(ACTION, EXPECT) ACTION, TEST_EXPECT__________(EXPECT), TEST_DELAY__(MESSENGER_FAILURE_PHASE_MS)

// Left-half keys are 'f', 'g', 'b' and 'v'; 'j' and 'k' are on the right.
static const test_action_t test_messenger_failure_typing[] = {
    TEST_SET_BOOL(&TestHooks_FailMessengerSends, true),
    TEST_SET_ACTION("f", "f"),
    TEST_SET_ACTION("g", "g"),
    TEST_SET_ACTION("b", "b"),
    TEST_SET_ACTION("v", "v"),
    TEST_SET_ACTION("j", "j"),
    TEST_SET_ACTION("k", "k"),

    STEP(TEST_PRESS______("f"), "f"),
    STEP(TEST_PRESS______("j"), "f j"),
    STEP(TEST_PRESS______("g"), "f j g"),
    STEP(TEST_RELEASE__U("f"), "j g"),
    STEP(TEST_PRESS______("b"), "j g b"),
    STEP(TEST_RELEASE__U("j"), "g b"),
    STEP(TEST_PRESS______("v"), "g b v"),
    STEP(TEST_RELEASE__U("g"), "b v"),
    STEP(TEST_PRESS______("k"), "b v k"),
    STEP(TEST_RELEASE__U("b"), "v k"),
    STEP(TEST_RELEASE__U("v"), "k"),
    STEP(TEST_RELEASE__U("k"), ""),

    TEST_SET_BOOL(&TestHooks_FailMessengerSends, false),
    TEST_END()
};

static const test_t messenger_failure_tests[] = {
    {
        .name = "typing",
        .actions = test_messenger_failure_typing,
        .linkTestId = LinkTestId_MessengerFailure,
        .repeatCount = MESSENGER_FAILURE_CYCLES,
        .spuriousFailureRate = MESSENGER_FAILURE_CYCLE_FAILURE_RATE,
    },
};

const test_module_t TestModule_MessengerFailure = {
    .name = "MessengerFailure",
    .tests = messenger_failure_tests,
    .testCount = sizeof(messenger_failure_tests) / sizeof(messenger_failure_tests[0])
};
