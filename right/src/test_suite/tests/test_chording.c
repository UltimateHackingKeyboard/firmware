#include "tests.h"

// Chording test: keys pressed within chordingDelay are reordered by priority
// Priority: 0=keystrokes/mouse, 1=secondary roles/macros, 2=layer/keymap switches
// Press J then Ctrl quickly -> should output Ctrl+J (Ctrl reordered before J)
static const test_action_t test_chording_basic[] = {
    TEST_SET_CONFIG("chordingDelay 50"),
    TEST_SET_ACTION("j", "j"),
    TEST_SET_ACTION("p", "LC"),

    // Press j, then ctrl within chording delay
    // Without chording: j down, ctrl down -> j, C-j
    // With chording: ctrl reordered first -> C-j
    TEST_PRESS______("j"),
    TEST_DELAY__(10),
    TEST_PRESS______("p"),
    TEST_DELAY__(100),

    TEST_EXPECT__________("LC"),
    TEST_EXPECT__________("LC-j"),

    TEST_RELEASE__U("j"),
    TEST_RELEASE__U("p"),
    TEST_DELAY__(50),

    TEST_EXPECT__________("LC"),
    TEST_EXPECT__________(""),

    TEST_END()
};

// Chording test: layer switch gets highest priority
// Press J then layer switch -> layer switch happens first
static const test_action_t test_chording_layer_priority[] = {
    TEST_SET_CONFIG("chordingDelay 50"),
    TEST_SET_ACTION("j", "j"),
    TEST_SET_LAYER_HOLD("p", 1),
    TEST_SET_LAYER_ACTION(1, "j", "k"),

    // Press j, then layer hold within chording delay
    // Layer switch should be reordered to happen first
    // So 'j' is looked up on layer 1, outputting 'k'
    TEST_PRESS______("j"),
    TEST_DELAY__(10),
    TEST_PRESS______("p"),
    TEST_DELAY__(100),

    TEST_EXPECT__________("k"),

    TEST_RELEASE__U("j"),
    TEST_RELEASE__U("p"),
    TEST_DELAY__(50),

    TEST_EXPECT__________(""),

    TEST_END()
};

// Chording disabled: keys processed in order pressed
static const test_action_t test_chording_disabled[] = {
    TEST_SET_CONFIG("chordingDelay 0"),
    TEST_SET_ACTION("j", "j"),
    TEST_SET_ACTION("p", "LC"),

    // Press j, then ctrl - without chording, j goes first
    TEST_PRESS______("j"),
    TEST_DELAY__(10),
    TEST_PRESS______("p"),
    TEST_DELAY__(50),

    TEST_EXPECT__________("j"),
    TEST_EXPECT__________("LC-j"),

    TEST_RELEASE__U("j"),
    TEST_RELEASE__U("p"),
    TEST_DELAY__(50),

    TEST_EXPECT__________("LC"),
    TEST_EXPECT__________(""),

    TEST_END()
};

static const test_t chording_tests[] = {
    { .name = "basic",          .actions = test_chording_basic },
    { .name = "layer_priority", .actions = test_chording_layer_priority },
    { .name = "disabled",       .actions = test_chording_disabled },
};

const test_module_t TestModule_Chording = {
    .name = "Chording",
    .tests = chording_tests,
    .testCount = sizeof(chording_tests) / sizeof(chording_tests[0])
};
