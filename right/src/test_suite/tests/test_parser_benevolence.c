#include "tests.h"

// Test: if condition evaluates to true, key should be produced
static const test_action_t test_if_true_produces_output[] = {
    TEST_SET_MACRO("u",
        "if(1 == 1) tapKey u\n"
    ),
    TEST_PRESS______("u"),
    TEST_DELAY__(20),
    TEST_EXPECT__________("u"),
    TEST_EXPECT__________(""),
    TEST_RELEASE__U("u"),
    TEST_DELAY__(20),
    TEST_EXPECT__________(""),
    TEST_END()
};

static const test_t parser_benevolence_tests[] = {
    { .name = "if_true_produces_output", .actions = test_if_true_produces_output },
};

const test_module_t TestModule_ParserBenevolence = {
    .name = "ParserBenevolence",
    .tests = parser_benevolence_tests,
    .testCount = sizeof(parser_benevolence_tests) / sizeof(parser_benevolence_tests[0])
};
