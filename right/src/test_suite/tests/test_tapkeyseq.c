#include "tests.h"

// Regression test for a template expansion bug.
//
// `tapKeySeq x y hexCodeOf(!) x y` would produce "xy21y" due to wrong ConsumeWhite/isEnd expansion.

static const test_action_t test_tapkeyseq_hexcode[] = {
    TEST_SET_MACRO("u",
        "tapKeySeq x y hexCodeOf(!) x y\n"
    ),
    TEST_PRESS______("u"),
    TEST_DELAY__(20),
    // x
    TEST_EXPECT__________("x"),
    TEST_EXPECT__________(""),
    // y
    TEST_EXPECT__________("y"),
    TEST_EXPECT__________(""),
    // hexCodeOf(!) -> 2 1
    TEST_EXPECT__________("2"),
    TEST_EXPECT__________(""),
    TEST_EXPECT__________("1"),
    TEST_EXPECT__________(""),
    // x
    TEST_EXPECT__________("x"),
    TEST_EXPECT__________(""),
    // y  (the final key of the sequence — the one the bug dropped)
    TEST_EXPECT__________("y"),
    TEST_EXPECT__________(""),
    TEST_RELEASE__U("u"),
    TEST_DELAY__(20),
    TEST_EXPECT__________(""),
    TEST_END()
};

static const test_t tapkeyseq_tests[] = {
    { .name = "tapkeyseq_hexcode", .actions = test_tapkeyseq_hexcode },
};

const test_module_t TestModule_TapKeySeq = {
    .name = "TapKeySeq",
    .tests = tapkeyseq_tests,
    .testCount = sizeof(tapkeyseq_tests) / sizeof(tapkeyseq_tests[0])
};
