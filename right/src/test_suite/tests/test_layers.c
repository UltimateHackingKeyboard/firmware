#include "tests.h"
#include "layer.h"

// Layer hold: press layer key, press key on that layer, release both
static const test_action_t test_layer_hold[] = {
    TEST_SET_LAYER_HOLD("u", LayerId_Mod),
    TEST_SET_LAYER_ACTION(LayerId_Mod, "i", "x"),
    TEST_SET_ACTION("i", "a"),  // Base layer binding
    TEST_PRESS("u"),            // Hold layer key
    TEST_DELAY(20),
    TEST_EXPECT(""),            // Layer switch produces no report
    TEST_PRESS("i"),            // Press key while layer held
    TEST_DELAY(20),
    TEST_EXPECT("x"),           // Should get Mod layer binding
    TEST_RELEASE("i"),
    TEST_DELAY(20),
    TEST_EXPECT(""),
    TEST_RELEASE("u"),
    TEST_DELAY(20),
    TEST_EXPECT(""),
    TEST_END()
};

// Layer hold with modifier on layer
static const test_action_t test_layer_hold_with_modifier[] = {
    TEST_SET_LAYER_HOLD("u", LayerId_Mod),
    TEST_SET_LAYER_ACTION(LayerId_Mod, "i", "LS-a"),
    TEST_PRESS("u"),            // Hold layer key
    TEST_DELAY(20),
    TEST_EXPECT(""),
    TEST_PRESS("i"),            // Press key with modifier binding
    TEST_DELAY(50),             // Wait for debouncing, otherwise, sticky mods will be reset before i's release.
    TEST_EXPECT("LS"),
    TEST_EXPECT("LS-a"),
    TEST_RELEASE("i"),
    TEST_DELAY(20),
    TEST_EXPECT("LS"),
    TEST_EXPECT(""),
    TEST_RELEASE("u"),
    TEST_DELAY(20),
    TEST_EXPECT(""),
    TEST_END()
};

static const test_t layer_tests[] = {
    { .name = "layer_hold", .actions = test_layer_hold },
    { .name = "layer_hold_with_modifier", .actions = test_layer_hold_with_modifier },
};

const test_module_t TestModule_Layers = {
    .name = "Layers",
    .tests = layer_tests,
    .testCount = sizeof(layer_tests) / sizeof(layer_tests[0])
};


