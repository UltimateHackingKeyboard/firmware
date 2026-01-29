#include "tests.h"
#include "lufa/HIDClassCommon.h"
#include "secondary_role_driver.h"

// Secondary role test: positive safetyMargin (release-order based)
// Release as secondary role, but resolve as primary due to positive safety margin.
// Key u has primary=u, secondary=LS
// Press u, press i, release i, 20ms, release u -> u i
static const test_action_t test_secondary_role_positive_safety[] = {
    // Configure for release-order based triggering with positive safetyMargin
    TEST_SET_CONFIG("secondaryRole.defaultStrategy advanced"),
    TEST_SET_CONFIG("secondaryRole.advanced.timeout 350"),
    TEST_SET_CONFIG("secondaryRole.advanced.timeoutAction secondary"),
    TEST_SET_CONFIG("secondaryRole.advanced.safetyMargin 50"),
    TEST_SET_CONFIG("secondaryRole.advanced.triggeringEvent release"),

    // Set up u as secondary role key: tap=u, hold=LS
    TEST_SET_SECONDARY_ROLE("u", HID_KEYBOARD_SC_U, SecondaryRole_LeftShift),
    // Set i to output i
    TEST_SET_ACTION("i", "i"),

    // Press u (secondary role key) - nothing happens yet (postponed)
    TEST_PRESS______("u"),
    TEST_DELAY__(20),

    // Press i while holding u - this may trigger secondary depending on config
    TEST_PRESS______("i"),
    TEST_DELAY__(60),

    // Release i first - with release-order triggering and positive margin,
    // releasing i before u should trigger secondary role
    TEST_RELEASE__U("i"),
    TEST_DELAY__(20),

    // Release u - secondary role (LS) should now activate
    TEST_RELEASE__U("u"),
    TEST_DELAY__(50),

    // Expected: LS activated, i typed with LS modifier, then both released
    TEST_EXPECT__________("u"),
    TEST_EXPECT__________("u i"),
    TEST_EXPECT__________("u "),
    TEST_EXPECT__________(""),

    TEST_END()
};

// Secondary role test: negative safetyMargin (biased towards secondary role)
// Release as a primary role, but resolve as secondary due to negative safety margin.
// Key u has primary=u, secondary=LS
// Press u, press i, release u, release i -> LS LS-i i
static const test_action_t test_secondary_role_negative_safety[] = {
    // Configure with negative safetyMargin - biased towards secondary role
    TEST_SET_CONFIG("secondaryRole.defaultStrategy advanced"),
    TEST_SET_CONFIG("secondaryRole.advanced.timeout 350"),
    TEST_SET_CONFIG("secondaryRole.advanced.timeoutAction secondary"),
    TEST_SET_CONFIG("secondaryRole.advanced.safetyMargin -50"),
    TEST_SET_CONFIG("secondaryRole.advanced.triggeringEvent release"),
    TEST_SET_SECONDARY_ROLE("u", HID_KEYBOARD_SC_U, SecondaryRole_LeftShift),
    TEST_SET_ACTION("i", "i"),
    TEST_PRESS______("u"),
    TEST_DELAY__(20),
    TEST_PRESS______("i"),
    TEST_DELAY__(60),
    TEST_RELEASE__U("u"),
    TEST_DELAY__(20),
    TEST_RELEASE__U("i"),
    TEST_DELAY__(50),
    TEST_EXPECT__________("LS"),
    TEST_EXPECT__________("LS-i"),
    TEST_EXPECT__________("i"),
    TEST_EXPECT__________(""),
    TEST_END()
};

static const test_t secondary_role_tests[] = {
    { .name = "positive_safety_margin", .actions = test_secondary_role_positive_safety },
    { .name = "negative_safety_margin", .actions = test_secondary_role_negative_safety },
};

const test_module_t TestModule_SecondaryRoles = {
    .name = "SecondaryRoles",
    .tests = secondary_role_tests,
    .testCount = sizeof(secondary_role_tests) / sizeof(secondary_role_tests[0])
};
