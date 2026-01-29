#include "tests.h"

// Modifier test: keystroke with shift modifier produces correct report sequence
// LS-u should produce: LS (modifier alone), LS-u (modifier+key), LS (key released), empty
static const test_action_t test_modifier_keystroke[] = {
    TEST_SET_ACTION("u", "LS-u"),
    TEST_PRESS______("u"),
    TEST_DELAY__(20),
    TEST_EXPECT__________("LS"),
    TEST_EXPECT__________("LS-u"),
    TEST_RELEASE__U("u"),
    TEST_DELAY__(20),
    TEST_EXPECT__________("LS"),
    TEST_EXPECT__________(""),
    TEST_END()
};

static const test_t modifier_tests[] = {
    { .name = "modifier_keystroke", .actions = test_modifier_keystroke },
};

const test_module_t TestModule_Modifiers = {
    .name = "Modifiers",
    .tests = modifier_tests,
    .testCount = sizeof(modifier_tests) / sizeof(modifier_tests[0])
};
