#include "tests.h"

// Regression test for a tapKeySeq bug: when a code-expansion command such as
// hexCodeOf(...) sits in the middle of the sequence, the key that immediately
// follows the expansion used to be dropped.
//
// `tapKeySeq x y hexCodeOf(!) x y` must type
//     x y 2 1 x y
// ('!' == U+0021, so hexCodeOf(!) expands to the key sequence `2 1`). The bug
// dropped the final key of the whole sequence whenever a code-expansion command
// preceded it, so the tail came out as `... x` instead of `... x y`. Distinct
// keys (x/y) around the expansion make a dropped key surface as a wrong-char
// mismatch rather than only a timeout.
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
