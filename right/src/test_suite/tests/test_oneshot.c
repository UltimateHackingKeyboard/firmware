#include "tests.h"

// Oneshot test: oneShot LS, oneShot LC, then o tap, then p tap
// Expected reports: LS -> LSLC -> LSLC-o -> o -> (empty) -> p -> (empty)
static const test_action_t test_oneshot_modifiers[] = {
    // Set up keys with oneshot macros and regular keystrokes
    TEST_SET_MACRO("u", "oneShot holdKey LS\n"),
    TEST_SET_MACRO("i", "oneShot holdKey LC\n"),
    TEST_SET_ACTION("o", "o"),
    TEST_SET_ACTION("p", "p"),

    // Trigger oneShot LS
    TEST_PRESS______("u"),
    TEST_DELAY__(60),
    TEST_EXPECT__________("LS"),
    TEST_RELEASE__U("u"),
    TEST_DELAY__(20),

    // Trigger oneShot LC (should combine with LS)
    TEST_PRESS______("i"),
    TEST_DELAY__(60),
    TEST_EXPECT__________("LSLC"),
    TEST_RELEASE__U("i"),
    TEST_DELAY__(20),

    // Tap o - should get both modifiers applied, then they release
    TEST_PRESS______("o"),
    TEST_DELAY__(60),
    TEST_EXPECT__________("LSLC-o"),
    TEST_EXPECT___________MAYBE("LC-o"),
    TEST_EXPECT___________MAYBE("LS-o"),
    TEST_RELEASE__U("o"),
    TEST_DELAY__(20),
    TEST_EXPECT__________("o"),  // Modifiers unwind while key may still be releasing
    TEST_EXPECT__________(""),

    // Tap p - should be unmodified now
    TEST_PRESS______("p"),
    TEST_DELAY__(60),
    TEST_EXPECT__________("p"),
    TEST_RELEASE__U("p"),
    TEST_DELAY__(20),
    TEST_EXPECT__________(""),

    TEST_END()
};

static const test_t oneshot_tests[] = {
    { .name = "oneshot_modifiers", .actions = test_oneshot_modifiers },
};

const test_module_t TestModule_Oneshot = {
    .name = "Oneshot",
    .tests = oneshot_tests,
    .testCount = sizeof(oneshot_tests) / sizeof(oneshot_tests[0])
};
