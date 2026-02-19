#include "tests.h"

static const test_action_t test_postpone_holdkey_no_deadlock[] = {
    TEST_SET_MACRO("j", "postponeKeys {\n"
                        "  holdKey j\n"
                        "}"),
    TEST_SET_MACRO("k", "tapKey i"),

    TEST_PRESS______("j"),
    TEST_DELAY__(50),
    TEST_EXPECT__________("j"),
    TEST_RELEASE__U("j"),
    TEST_DELAY__(50),
    TEST_EXPECT__________(""),
    TEST_PRESS______("k"),
    TEST_DELAY__(50),
    TEST_EXPECT__________("i"),
    TEST_EXPECT__________(""),
    TEST_RELEASE__U("k"),
    TEST_DELAY__(50),
    TEST_END()
};

static const test_action_t test_holdkey_no_premature_release[] = {
    TEST_SET_MACRO("i", "holdKey i"),
    TEST_SET_MACRO("u", "holdKey u"),
    TEST_SET_MACRO("o", "postponeKeys\n"
                        "tapKey o"),

    TEST_PRESS______("i"),
    TEST_DELAY__(50),
    TEST_EXPECT__________("i"),
    TEST_PRESS______("u"),
    TEST_DELAY__(50),
    TEST_EXPECT__________("i u"),
    TEST_RELEASE__U("u"),
    TEST_DELAY__(50),
    TEST_EXPECT__________("i"),
    TEST_PRESS______("o"),
    TEST_DELAY__(50),
    TEST_EXPECT__________("i o"),
    TEST_RELEASE__U("o"),
    TEST_DELAY__(50),
    TEST_EXPECT__________("i"),
    TEST_RELEASE__U("i"),
    TEST_DELAY__(50),
    TEST_END()
};

static const test_action_t test_postpone_loop_tapkey[] = {
    TEST_SET_MACRO("i", "postponeKeys while (1) {\n"
                        "    ifReleased break\n"
                        "    tapKey i\n"
                        "    delayUntil 20\n"
                        "}"),
    TEST_SET_ACTION("u", "u"),

    TEST_PRESS______("i"),
    TEST_DELAY__(30),
    TEST_RELEASE__U("i"),
    TEST_DELAY__(50),
    TEST_PRESS______("u"),
    TEST_DELAY__(50),
    TEST_EXPECT__________("i"),
    TEST_EXPECT__________(""),
    TEST_EXPECT__________("i"),
    TEST_EXPECT__________(""),
    TEST_EXPECT__________("u"),
    TEST_RELEASE__U("u"),
    TEST_DELAY__(50),
    TEST_END()
};

static const test_t current_macro_key_is_active_tests[] = {
    { .name = "postpone_holdkey_no_deadlock",   .actions = test_postpone_holdkey_no_deadlock },
    { .name = "holdkey_no_premature_release",   .actions = test_holdkey_no_premature_release },
    { .name = "postpone_loop_tapkey",           .actions = test_postpone_loop_tapkey },
};

const test_module_t TestModule_CurrentMacroKeyIsActive = {
    .name = "CurrentMacroKeyIsActive",
    .tests = current_macro_key_is_active_tests,
    .testCount = sizeof(current_macro_key_is_active_tests) / sizeof(current_macro_key_is_active_tests[0])
};
