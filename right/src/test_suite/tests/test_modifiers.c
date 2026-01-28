#include "tests.h"

// Modifier test: keystroke with shift modifier produces correct report sequence
// LS-u should produce: LS (modifier alone), LS-u (modifier+key), LS (key released), empty
static const test_action_t test_modifier_keystroke[] = {
    TEST_SET_ACTION("u", "LS-u"),
    TEST_PRESS("u"),
    TEST_DELAY(20),
    TEST_EXPECT("LS"),
    TEST_EXPECT("LS-u"),
    TEST_RELEASE("u"),
    TEST_DELAY(20),
    TEST_EXPECT("LS"),
    TEST_EXPECT(""),
    TEST_END()
};

// Multiple modifiers: CS-u (Ctrl+Shift+u)
static const test_action_t test_multiple_modifiers[] = {
    TEST_SET_ACTION("u", "CS-u"),
    TEST_PRESS("u"),
    TEST_DELAY(20),
    TEST_EXPECT("CS"),
    TEST_EXPECT("CS-u"),
    TEST_RELEASE("u"),
    TEST_DELAY(20),
    TEST_EXPECT("CS"),
    TEST_EXPECT(""),
    TEST_END()
};

// Tap LS-u via macro: should produce LS, LS-u, LS (then implicitly empty)
static const test_action_t test_tap_modifier[] = {
    TEST_SET_MACRO("u", "tapKey LS-u\n"),
    TEST_PRESS("u"),
    TEST_DELAY(20),
    TEST_EXPECT("LS"),
    TEST_EXPECT("LS-u"),
    TEST_EXPECT("LS"),
    TEST_EXPECT(""),
    TEST_RELEASE("u"),
    TEST_DELAY(20),
    TEST_EXPECT(""),
    TEST_END()
};

static const test_t modifier_tests[] = {
    { .name = "modifier_keystroke", .actions = test_modifier_keystroke },
    { .name = "multiple_modifiers", .actions = test_multiple_modifiers },
    { .name = "tap_modifier", .actions = test_tap_modifier },
};

const test_module_t TestModule_Modifiers = {
    .name = "Modifiers",
    .tests = modifier_tests,
    .testCount = sizeof(modifier_tests) / sizeof(modifier_tests[0])
};
