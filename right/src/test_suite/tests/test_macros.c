#include "tests.h"

// Macro test: inline macro with two tapKeys
static const test_action_t test_macro_two_tapkeys[] = {
    TEST_SET_MACRO("u",
        "tapKey a\n"
        "tapKey b\n"
    ),
    TEST_PRESS("u"),
    TEST_DELAY(20),
    TEST_EXPECT("a"),
    TEST_EXPECT(""),
    TEST_EXPECT("b"),
    TEST_EXPECT(""),
    TEST_RELEASE("u"),
    TEST_DELAY(20),
    TEST_EXPECT(""),
    TEST_END()
};

// Macro with modifier
static const test_action_t test_macro_with_modifier[] = {
    TEST_SET_MACRO("u",
        "tapKey LS-a\n"
    ),
    TEST_PRESS("u"),
    TEST_DELAY(20),
    TEST_EXPECT("LS"),
    TEST_EXPECT("LS-a"),
    TEST_EXPECT("LS"),
    TEST_EXPECT(""),
    TEST_RELEASE("u"),
    TEST_DELAY(20),
    TEST_EXPECT(""),
    TEST_END()
};

static const test_t macro_tests[] = {
    { .name = "macro_two_tapkeys", .actions = test_macro_two_tapkeys },
    { .name = "macro_with_modifier", .actions = test_macro_with_modifier },
};

const test_module_t TestModule_Macros = {
    .name = "Macros",
    .tests = macro_tests,
    .testCount = sizeof(macro_tests) / sizeof(macro_tests[0])
};
