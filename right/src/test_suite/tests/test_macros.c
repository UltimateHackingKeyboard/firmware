#include "tests.h"

// Macro test: inline macro with two tapKeys
static const test_action_t test_macro_two_tapkeys[] = {
    TEST_SET_MACRO("u",
        "tapKey u\n"
        "tapKey i\n"
    ),
    TEST_PRESS______("u"),
    TEST_DELAY__(20),
    TEST_EXPECT__________("u"),
    TEST_EXPECT__________(""),
    TEST_EXPECT__________("i"),
    TEST_EXPECT__________(""),
    TEST_RELEASE__U("u"),
    TEST_DELAY__(20),
    TEST_EXPECT__________(""),
    TEST_END()
};

// Macro with modifier
static const test_action_t test_macro_with_modifier[] = {
    TEST_SET_MACRO("u",
        "tapKey LS-u\n"
    ),
    TEST_PRESS______("u"),
    TEST_DELAY__(20),
    TEST_EXPECT__________("LS"),
    TEST_EXPECT__________("LS-u"),
    TEST_EXPECT__________("LS"),
    TEST_EXPECT__________(""),
    TEST_RELEASE__U("u"),
    TEST_DELAY__(20),
    TEST_EXPECT__________(""),
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
