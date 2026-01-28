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
    TEST_PRESS("u"),
    TEST_DELAY(60),
    TEST_EXPECT("LS"),
    TEST_RELEASE("u"),
    TEST_DELAY(20),

    // Trigger oneShot LC (should combine with LS)
    TEST_PRESS("i"),
    TEST_DELAY(60),
    TEST_EXPECT("LSLC"),
    TEST_RELEASE("i"),
    TEST_DELAY(20),

    // Tap o - should get both modifiers applied, then they release
    TEST_PRESS("o"),
    TEST_DELAY(60),
    TEST_EXPECT("LSLC-o"),
    TEST_EXPECT_MAYBE("LC-o"),
    TEST_EXPECT_MAYBE("LS-o"),
    TEST_RELEASE("o"),
    TEST_DELAY(20),
    TEST_EXPECT("o"),  // Modifiers unwind while key may still be releasing
    TEST_EXPECT(""),

    // Tap p - should be unmodified now
    TEST_PRESS("p"),
    TEST_DELAY(60),
    TEST_EXPECT("p"),
    TEST_RELEASE("p"),
    TEST_DELAY(20),
    TEST_EXPECT(""),

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
