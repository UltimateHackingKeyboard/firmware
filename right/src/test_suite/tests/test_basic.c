#include "tests.h"

// Basic test: press a key, expect scancode, release, expect empty
static const test_action_t test_basic_keypress[] = {
    TEST_SET_ACTION("u", "u"),
    TEST_PRESS("u"),
    TEST_DELAY(20),
    TEST_EXPECT("u"),
    TEST_RELEASE("u"),
    TEST_DELAY(20),
    TEST_EXPECT(""),
    TEST_END()
};

// Two keys pressed simultaneously
static const test_action_t test_two_keys[] = {
    TEST_SET_ACTION("u", "u"),
    TEST_SET_ACTION("i", "i"),
    TEST_PRESS("u"),
    TEST_DELAY(20),
    TEST_EXPECT("u"),
    TEST_PRESS("i"),
    TEST_DELAY(20),
    TEST_EXPECT("u i"),
    TEST_RELEASE("u"),
    TEST_DELAY(20),
    TEST_EXPECT("i"),
    TEST_RELEASE("i"),
    TEST_DELAY(20),
    TEST_EXPECT(""),
    TEST_END()
};

static const test_t basic_tests[] = {
    { .name = "basic_keypress", .actions = test_basic_keypress },
    { .name = "two_keys", .actions = test_two_keys },
};

const test_module_t TestModule_Basic = {
    .name = "Basic",
    .tests = basic_tests,
    .testCount = sizeof(basic_tests) / sizeof(basic_tests[0])
};
