#include "tests.h"
#include "layer.h"

// Test: layer doubletap toggle - tap-tap toggles layer on
// Setup: key "u" is HoldAndDoubleTapToggle for Mod layer, key "i" has different action on Mod layer
// Action: tap "u", release, tap "u" again quickly (within doubletap timeout)
// Expected: layer toggles on after second tap, "i" produces Mod layer action
static const test_action_t test_layer_doubletap_toggle[] = {
    TEST_SET_CONFIG("doubletapTimeout 400"),
    TEST_SET_LAYER_DOUBLETAP_TOGGLE("u", LayerId_Mod),
    TEST_SET_ACTION("i", "i"),
    TEST_SET_LAYER_ACTION(LayerId_Mod, "i", "n"),

    // First tap - just hold layer temporarily
    TEST_PRESS______("u"),
    TEST_DELAY__(50),
    TEST_EXPECT__________(""),
    TEST_RELEASE__U("u"),
    TEST_DELAY__(50),
    TEST_EXPECT__________(""),

    // Second tap quickly - should toggle layer on
    TEST_PRESS______("u"),
    TEST_DELAY__(50),
    TEST_EXPECT__________(""),
    TEST_RELEASE__U("u"),
    TEST_DELAY__(50),
    TEST_EXPECT__________(""),

    // Now press "i" - should produce Mod layer action (n) because layer is toggled
    TEST_PRESS______("i"),
    TEST_DELAY__(50),
    TEST_EXPECT__________("n"),
    TEST_RELEASE__U("i"),
    TEST_DELAY__(50),
    TEST_EXPECT__________(""),

    // Wait for doubletap window to expire before toggling off
    TEST_DELAY__(500),

    // Tap layer key again to toggle off (outside doubletap window)
    TEST_PRESS______("u"),
    TEST_DELAY__(50),
    TEST_EXPECT__________(""),
    TEST_RELEASE__U("u"),
    TEST_DELAY__(50),
    TEST_EXPECT__________(""),

    // Now "i" should produce base layer action (i)
    TEST_PRESS______("i"),
    TEST_DELAY__(50),
    TEST_EXPECT__________("i"),
    TEST_RELEASE__U("i"),
    TEST_DELAY__(50),
    TEST_EXPECT__________(""),

    TEST_END()
};

// Test: layer doubletap interrupt - pressing another key between taps prevents toggle
// Setup: key "u" is HoldAndDoubleTapToggle for Mod layer
// Action: tap "u", press another key "i", tap "u" again
// Expected: layer does NOT toggle (interrupted by "i")
static const test_action_t test_layer_doubletap_interrupt[] = {
    TEST_SET_CONFIG("doubletapTimeout 400"),
    TEST_SET_LAYER_DOUBLETAP_TOGGLE("u", LayerId_Mod),
    TEST_SET_ACTION("i", "i"),
    TEST_SET_LAYER_ACTION(LayerId_Mod, "i", "n"),

    // First tap
    TEST_PRESS______("u"),
    TEST_DELAY__(50),
    TEST_EXPECT__________(""),
    TEST_RELEASE__U("u"),
    TEST_DELAY__(50),
    TEST_EXPECT__________(""),

    // Interrupt with another key
    TEST_PRESS______("i"),
    TEST_DELAY__(50),
    TEST_EXPECT__________("i"),
    TEST_RELEASE__U("i"),
    TEST_DELAY__(50),
    TEST_EXPECT__________(""),

    // Second tap - should NOT toggle because interrupted
    TEST_PRESS______("u"),
    TEST_DELAY__(50),
    TEST_EXPECT__________(""),
    TEST_RELEASE__U("u"),
    TEST_DELAY__(50),
    TEST_EXPECT__________(""),

    // Now "i" should produce base layer action (i), not Mod layer (n)
    TEST_PRESS______("i"),
    TEST_DELAY__(50),
    TEST_EXPECT__________("i"),
    TEST_RELEASE__U("i"),
    TEST_DELAY__(50),
    TEST_EXPECT__________(""),

    TEST_END()
};

// Test: ifDoubletap macro command - doubletap triggers conditional
// Setup: macro with ifDoubletap that types different keys based on doubletap
// Action: single tap, then doubletap
// Expected: single tap produces one output, doubletap produces different output
static const test_action_t test_macro_if_doubletap[] = {
    TEST_SET_CONFIG("doubletapTimeout 400"),
    TEST_SET_MACRO("u",
        "ifDoubletap tapKey n\n"
        "tapKey u\n"
    ),

    // First tap - should produce "u" (not doubletap)
    TEST_PRESS______("u"),
    TEST_DELAY__(50),
    TEST_EXPECT__________("u"),
    TEST_EXPECT__________(""),
    TEST_RELEASE__U("u"),
    TEST_DELAY__(50),
    TEST_EXPECT__________(""),

    // Wait for doubletap window to pass
    TEST_DELAY__(500),

    // Single tap again - should produce "u" (not doubletap)
    TEST_PRESS______("u"),
    TEST_DELAY__(50),
    TEST_EXPECT__________("u"),
    TEST_EXPECT__________(""),
    TEST_RELEASE__U("u"),
    TEST_DELAY__(50),
    TEST_EXPECT__________(""),

    // Now doubletap quickly - should produce "n" then "u"
    TEST_PRESS______("u"),
    TEST_DELAY__(50),
    TEST_EXPECT__________("n"),
    TEST_EXPECT__________(""),
    TEST_EXPECT__________("u"),
    TEST_EXPECT__________(""),
    TEST_RELEASE__U("u"),
    TEST_DELAY__(50),
    TEST_EXPECT__________(""),

    TEST_END()
};

// Test: ifNotDoubletap macro command - inverse of ifDoubletap
static const test_action_t test_macro_if_not_doubletap[] = {
    TEST_SET_CONFIG("doubletapTimeout 400"),
    TEST_SET_MACRO("u",
        "ifNotDoubletap tapKey u\n"
        "tapKey n\n"
    ),

    // First tap - should produce "u" then "n" (not doubletap, so ifNotDoubletap fires)
    TEST_PRESS______("u"),
    TEST_DELAY__(50),
    TEST_EXPECT__________("u"),
    TEST_EXPECT__________(""),
    TEST_EXPECT__________("n"),
    TEST_EXPECT__________(""),
    TEST_RELEASE__U("u"),
    TEST_DELAY__(50),
    TEST_EXPECT__________(""),

    // Doubletap quickly - should only produce "n" (doubletap, so ifNotDoubletap skipped)
    TEST_PRESS______("u"),
    TEST_DELAY__(50),
    TEST_EXPECT__________("n"),
    TEST_EXPECT__________(""),
    TEST_RELEASE__U("u"),
    TEST_DELAY__(50),
    TEST_EXPECT__________(""),

    TEST_END()
};

static const test_t doubletap_tests[] = {
    { .name = "layer_doubletap_toggle", .actions = test_layer_doubletap_toggle },
    { .name = "layer_doubletap_interrupt", .actions = test_layer_doubletap_interrupt },
    { .name = "macro_if_doubletap", .actions = test_macro_if_doubletap },
    { .name = "macro_if_not_doubletap", .actions = test_macro_if_not_doubletap },
};

const test_module_t TestModule_Doubletap = {
    .name = "Doubletap",
    .tests = doubletap_tests,
    .testCount = sizeof(doubletap_tests) / sizeof(doubletap_tests[0])
};
