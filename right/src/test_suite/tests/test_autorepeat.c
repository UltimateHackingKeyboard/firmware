#include "tests.h"

// Autorepeat test: verify that autoRepeat command produces repeated keystrokes
// Configure short delays for testing, then verify initial and repeated keystrokes
static const test_action_t test_autorepeat_basic[] = {
    // Configure short autorepeat timing for testing
    TEST_SET_CONFIG("autoRepeatDelay 30"),
    TEST_SET_CONFIG("autoRepeatRate 30"),

    // Set up key with autoRepeat macro
    TEST_SET_MACRO("u", "autoRepeat tapKey u\n"),

    // Press the key - should produce initial keystroke
    TEST_PRESS("u"),
    TEST_DELAY(20),
    TEST_EXPECT("u"),
    TEST_EXPECT(""),

    // Wait for autorepeat to trigger at least one more keystroke
    TEST_DELAY(50),
    TEST_EXPECT("u"),
    TEST_EXPECT(""),

    // Release key
    TEST_RELEASE("u"),
    TEST_DELAY(20),
    TEST_EXPECT(""),

    TEST_END()
};

// Autorepeat test with modifier: verify modifiers are applied to autorepeated keystrokes
static const test_action_t test_autorepeat_with_modifier[] = {
    // Configure short autorepeat timing for testing
    TEST_SET_CONFIG("autoRepeatDelay 30"),
    TEST_SET_CONFIG("autoRepeatRate 30"),

    // Set up key with autoRepeat and modifier
    TEST_SET_MACRO("u", "autoRepeat tapKey LS-u\n"),

    // Press the key - should produce initial shifted keystroke
    TEST_PRESS("u"),
    TEST_DELAY(20),
    TEST_EXPECT("LS"),
    TEST_EXPECT("LS-u"),
    TEST_EXPECT("LS"),
    TEST_EXPECT(""),

    // Wait for autorepeat
    TEST_DELAY(50),
    TEST_EXPECT("LS"),
    TEST_EXPECT("LS-u"),
    TEST_EXPECT("LS"),
    TEST_EXPECT(""),

    // Release key
    TEST_RELEASE("u"),
    TEST_DELAY(20),
    TEST_EXPECT(""),

    TEST_END()
};

static const test_t autorepeat_tests[] = {
    { .name = "autorepeat_basic", .actions = test_autorepeat_basic },
    { .name = "autorepeat_with_modifier", .actions = test_autorepeat_with_modifier },
};

const test_module_t TestModule_Autorepeat = {
    .name = "Autorepeat",
    .tests = autorepeat_tests,
    .testCount = sizeof(autorepeat_tests) / sizeof(autorepeat_tests[0])
};
