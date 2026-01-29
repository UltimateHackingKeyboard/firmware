#include "tests.h"
#include "layer.h"

// press mod, press i, release i, release mod
static const test_action_t test_layer_hold[] = {
    TEST_SET_LAYER_HOLD("u", LayerId_Mod),
    TEST_SET_LAYER_ACTION(LayerId_Mod, "i", "i"),
    TEST_SET_ACTION("i", "i"),
    TEST_PRESS______("u"),
    TEST_DELAY__(20),
    TEST_EXPECT__________(""),
    TEST_PRESS______("i"),
    TEST_DELAY__(20),
    TEST_EXPECT__________("i"),
    TEST_RELEASE__U("i"),
    TEST_DELAY__(20),
    TEST_EXPECT__________(""),
    TEST_RELEASE__U("u"),
    TEST_DELAY__(20),
    TEST_EXPECT__________(""),
    TEST_END()
};

// press mod, press i, release i (LS-i), release mod
static const test_action_t test_layer_hold_with_modifier[] = {
    TEST_SET_LAYER_HOLD("u", LayerId_Mod),
    TEST_SET_LAYER_ACTION(LayerId_Mod, "i", "LS-i"),
    TEST_PRESS______("u"),
    TEST_DELAY__(20),
    TEST_EXPECT__________(""),
    TEST_PRESS______("i"),
    TEST_DELAY__(50),
    TEST_EXPECT__________("LS"),
    TEST_EXPECT__________("LS-i"),
    TEST_RELEASE__U("i"),
    TEST_DELAY__(20),
    TEST_EXPECT__________("LS"),
    TEST_EXPECT__________(""),
    TEST_RELEASE__U("u"),
    TEST_DELAY__(20),
    TEST_EXPECT__________(""),
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


