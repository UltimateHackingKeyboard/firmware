#include "tests.h"

// AutoShift: hold a key long enough and it becomes shifted
// Tap 'j' quickly -> 'j'
// Hold 'j' past autoShiftDelay -> 'J' (LS-j)
static const test_action_t test_autoshift_basic[] = {
    TEST_SET_CONFIG("autoShiftDelay 100"),
    TEST_SET_ACTION("j", "j"),

    // Quick tap - should produce 'j'
    TEST_PRESS______("j"),
    TEST_DELAY__(30),
    TEST_RELEASE__U("j"),
    TEST_DELAY__(50),

    TEST_EXPECT__________("j"),
    TEST_EXPECT__________(""),

    TEST_END()
};

// AutoShift: holding past the delay triggers shift
static const test_action_t test_autoshift_hold[] = {
    TEST_SET_CONFIG("autoShiftDelay 100"),
    TEST_SET_ACTION("j", "j"),

    // Hold past delay - should produce shifted 'J'
    TEST_PRESS______("j"),
    TEST_DELAY__(150),

    TEST_EXPECT__________("LS"),
    TEST_EXPECT__________("LS-j"),
    TEST_EXPECT__________("LS"),

    TEST_RELEASE__U("j"),
    TEST_DELAY__(50),

    TEST_EXPECT__________(""),

    TEST_END()
};

// AutoShift: keys with modifiers are not eligible
static const test_action_t test_autoshift_with_modifier[] = {
    TEST_SET_CONFIG("autoShiftDelay 100"),
    TEST_SET_ACTION("j", "LC-j"),

    // Key already has modifier - autoshift should not apply
    TEST_PRESS______("j"),
    TEST_DELAY__(150),

    TEST_EXPECT__________("LC"),
    TEST_EXPECT__________("LC-j"),
    TEST_EXPECT__________("LC"),

    TEST_RELEASE__U("j"),
    TEST_DELAY__(50),

    TEST_EXPECT__________(""),

    TEST_END()
};

// AutoShift: number keys are eligible
static const test_action_t test_autoshift_number[] = {
    TEST_SET_CONFIG("autoShiftDelay 100"),
    TEST_SET_ACTION("j", "7"),

    // Hold '7' past delay - should produce '&' (LS-7)
    TEST_PRESS______("j"),
    TEST_DELAY__(150),

    TEST_EXPECT__________("LS"),
    TEST_EXPECT__________("LS-7"),
    TEST_EXPECT__________("LS"),

    TEST_RELEASE__U("j"),
    TEST_DELAY__(50),

    TEST_EXPECT__________(""),

    TEST_END()
};

// AutoShift disabled: holding does not shift
static const test_action_t test_autoshift_disabled[] = {
    TEST_SET_CONFIG("autoShiftDelay 0"),
    TEST_SET_ACTION("j", "j"),

    // Hold key - without autoshift, just regular 'j'
    TEST_PRESS______("j"),
    TEST_DELAY__(150),

    TEST_EXPECT__________("j"),

    TEST_RELEASE__U("j"),
    TEST_DELAY__(50),

    TEST_EXPECT__________(""),

    TEST_END()
};

static const test_t autoshift_tests[] = {
    { .name = "basic",         .actions = test_autoshift_basic },
    { .name = "hold",          .actions = test_autoshift_hold },
    { .name = "with_modifier", .actions = test_autoshift_with_modifier },
    { .name = "number",        .actions = test_autoshift_number },
    { .name = "disabled",      .actions = test_autoshift_disabled },
};

const test_module_t TestModule_AutoShift = {
    .name = "AutoShift",
    .tests = autoshift_tests,
    .testCount = sizeof(autoshift_tests) / sizeof(autoshift_tests[0])
};
