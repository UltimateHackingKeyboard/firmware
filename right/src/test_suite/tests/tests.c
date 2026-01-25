#include "tests.h"

// Basic test: press a key, expect scancode, release, expect empty
static const test_action_t test_basic_keypress[] = {
    TEST_SET_ACTION("u", "a"),
    TEST_PRESS("u"),
    TEST_EXPECT("a"),
    TEST_RELEASE("u"),
    TEST_EXPECT(""),
    TEST_END()
};

// Macro test: inline macro with two tapKeys
static const test_action_t test_macro_two_tapkeys[] = {
    TEST_SET_MACRO("u",
        "tapKey a\n"
        "tapKey b\n"
    ),
    TEST_PRESS("u"),
    TEST_EXPECT("a"),
    TEST_EXPECT(""),
    TEST_EXPECT("b"),
    TEST_EXPECT(""),
    TEST_RELEASE("u"),
    TEST_EXPECT(""),
    TEST_END()
};

// All tests
const test_t AllTests[] = {
    { .name = "basic_keypress", .actions = test_basic_keypress },
    { .name = "macro_two_tapkeys", .actions = test_macro_two_tapkeys },
};

const uint16_t AllTestsCount = sizeof(AllTests) / sizeof(AllTests[0]);
