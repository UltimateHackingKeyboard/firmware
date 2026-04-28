#include "tests.h"
#include "layer.h"

// Same scenario but with a plain `LA-tab` keystroke binding instead of a macro.
// Under Stick_Smart, the Alt modifier is sticky for Alt+Tab when a layer is held.
static const test_action_t test_sticky_alt_tab_two_taps[] = {
    TEST_SET_LAYER_HOLD("n", LayerId_Mod),
    TEST_SET_LAYER_ACTION(LayerId_Mod, "i", "LA-tab"),

    // Hold the mod layer
    TEST_PRESS______("n"),
    TEST_DELAY__(50),
    TEST_EXPECT__________(""),

    // First tap: Alt down, then Alt+Tab, then Tab releases but Alt sticks
    TEST_PRESS______("i"),
    TEST_DELAY__(50),
    TEST_EXPECT__________("LA"),
    TEST_EXPECT__________("LA-tab"),
    TEST_RELEASE__U("i"),
    TEST_DELAY__(50),
    TEST_EXPECT__________("LA"),

    // Second tap: Alt must NOT have released — go straight back to Alt+Tab.
    TEST_PRESS______("i"),
    TEST_DELAY__(50),
    TEST_EXPECT__________("LA-tab"),
    TEST_RELEASE__U("i"),
    TEST_DELAY__(50),
    TEST_EXPECT__________("LA"),

    // Release layer key: sticky Alt clears on layer change
    TEST_RELEASE__U("n"),
    TEST_DELAY__(50),
    TEST_EXPECT__________(""),

    TEST_END()
};

// Tap a layer-mapped `holdKey sLA-tab` macro twice while the layer is held.
// Alt must remain held between the two Tabs (sticky behavior under Stick_Smart
// keeps the modifier active as long as the layer is held).
static const test_action_t test_sticky_holdkey_two_taps[] = {
    TEST_SET_LAYER_HOLD("n", LayerId_Mod),
    TEST_SET_LAYER_MACRO(LayerId_Mod, "i", "holdKey sLA-tab\n"),

    // Hold the mod layer
    TEST_PRESS______("n"),
    TEST_DELAY__(50),
    TEST_EXPECT__________(""),

    // First tap of the macro: Alt down, then Alt+Tab, then Tab releases but Alt sticks
    TEST_PRESS______("i"),
    TEST_DELAY__(50),
    TEST_EXPECT__________("LA"),
    TEST_EXPECT__________("LA-tab"),
    TEST_RELEASE__U("i"),
    TEST_DELAY__(50),
    TEST_EXPECT__________("LA"),

    // Second tap: Alt must NOT have released — go straight back to Alt+Tab.
    // No empty report between the two taps.
    TEST_PRESS______("i"),
    TEST_DELAY__(50),
    TEST_EXPECT__________("LA-tab"),
    TEST_RELEASE__U("i"),
    TEST_DELAY__(50),
    TEST_EXPECT__________("LA"),

    // Release layer key: sticky Alt clears on layer change
    TEST_RELEASE__U("n"),
    TEST_DELAY__(50),
    TEST_EXPECT__________(""),

    TEST_END()
};

static const test_t sticky_tests[] = {
    { .name = "sticky_alt_tab_two_taps", .actions = test_sticky_alt_tab_two_taps },
    { .name = "sticky_holdkey_two_taps", .actions = test_sticky_holdkey_two_taps },
};

const test_module_t TestModule_Sticky = {
    .name = "Sticky",
    .tests = sticky_tests,
    .testCount = sizeof(sticky_tests) / sizeof(sticky_tests[0])
};
