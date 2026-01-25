#include "tests.h"

// Modifier test: keystroke with shift modifier produces correct report sequence
// LS-a should produce: LS (modifier alone), LS-a (modifier+key), LS (key released), empty
static const test_action_t test_modifier_keystroke[] = {
    TEST_SET_ACTION("u", "LS-a"),
    TEST_PRESS("u"),
    TEST_DELAY(20),
    TEST_EXPECT("LS"),
    TEST_EXPECT("LS-a"),
    TEST_RELEASE("u"),
    TEST_DELAY(20),
    TEST_EXPECT("LS"),
    TEST_EXPECT(""),
    TEST_END()
};

// Multiple modifiers: CS-a (Ctrl+Shift+a)
static const test_action_t test_multiple_modifiers[] = {
    TEST_SET_ACTION("u", "CS-a"),
    TEST_PRESS("u"),
    TEST_DELAY(20),
    TEST_EXPECT("CS"),
    TEST_EXPECT("CS-a"),
    TEST_RELEASE("u"),
    TEST_DELAY(20),
    TEST_EXPECT("CS"),
    TEST_EXPECT(""),
    TEST_END()
};

static const test_t modifier_tests[] = {
    { .name = "modifier_keystroke", .actions = test_modifier_keystroke },
    { .name = "multiple_modifiers", .actions = test_multiple_modifiers },
};

const test_module_t TestModule_Modifiers = {
    .name = "Modifiers",
    .tests = modifier_tests,
    .testCount = sizeof(modifier_tests) / sizeof(modifier_tests[0])
};
