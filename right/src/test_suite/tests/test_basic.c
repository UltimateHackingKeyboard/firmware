#include "tests.h"

// press u, release u
static const test_action_t test_basic_keypress[] = {
    TEST_SET_ACTION("u", "u"),
    TEST_PRESS______("u"),
    TEST_DELAY__(20),
    TEST_EXPECT__________("u"),
    TEST_RELEASE__U("u"),
    TEST_DELAY__(20),
    TEST_EXPECT__________(""),
    TEST_END()
};

// Press u, press i, release u, release i
static const test_action_t test_two_keys[] = {
    TEST_SET_ACTION("u", "u"),
    TEST_SET_ACTION("i", "i"),
    TEST_PRESS______("u"),
    TEST_DELAY__(20),
    TEST_EXPECT__________("u"),
    TEST_PRESS______("i"),
    TEST_DELAY__(20),
    TEST_EXPECT__________("u i"),
    TEST_RELEASE__U("u"),
    TEST_DELAY__(20),
    TEST_EXPECT__________("i"),
    TEST_RELEASE__U("i"),
    TEST_DELAY__(20),
    TEST_EXPECT__________(""),
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
