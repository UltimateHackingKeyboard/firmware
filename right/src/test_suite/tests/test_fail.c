#include "tests.h"

// Always fails - used to verify that the test framework reports failures.
static const test_action_t test_fail[] = {
    TEST_SET_ACTION("u", "u"),
    TEST_PRESS______("u"),
    TEST_DELAY__(50),
    TEST_EXPECT__________("i"),
    TEST_RELEASE__U("u"),
    TEST_DELAY__(50),
    TEST_EXPECT__________(""),
    TEST_END()
};

static const test_t fail_tests[] = {
    { .name = "fail", .actions = test_fail },
};

const test_module_t TestModule_Fail = {
    .name = "Fail",
    .tests = fail_tests,
    .testCount = sizeof(fail_tests) / sizeof(fail_tests[0])
};
