#include "tests.h"

// =============================================================================
// ifShortcut tests
// =============================================================================

// ifShortcut basic: hold key, press another to trigger shortcut
static const test_action_t test_ifshortcut_basic[] = {
    TEST_SET_MACRO("j", "ifShortcut k final tapKey n\n holdKey j"),
    TEST_SET_ACTION("k", "k"),

    // Hold 'j', press 'k' -> should trigger tapKey n
    TEST_PRESS______("j"),
    TEST_DELAY__(20),
    TEST_PRESS______("k"),
    TEST_DELAY__(50),

    TEST_EXPECT__________("n"),
    TEST_EXPECT__________(""),

    TEST_RELEASE__U("k"),
    TEST_RELEASE__U("j"),
    TEST_DELAY__(50),

    TEST_END()
};

// ifShortcut: no match falls through to holdKey
static const test_action_t test_ifshortcut_no_match[] = {
    TEST_SET_MACRO("j", "ifShortcut k final tapKey n\n holdKey j"),
    TEST_SET_ACTION("k", "k"),

    // Just hold 'j' without pressing 'k' -> should output 'j'
    TEST_PRESS______("j"),
    TEST_DELAY__(20),

    TEST_EXPECT__________("j"),

    TEST_RELEASE__U("j"),
    TEST_DELAY__(50),

    TEST_EXPECT__________(""),

    TEST_END()
};

// ifNotShortcut: triggers when shortcut is NOT pressed
static const test_action_t test_ifnotshortcut[] = {
    TEST_SET_MACRO("j", "ifNotShortcut k final tapKey m\n holdKey j"),
    TEST_SET_ACTION("k", "k"),

    // Hold 'j' without pressing 'k' -> ifNotShortcut triggers, outputs 'm'
    TEST_PRESS______("j"),
    TEST_DELAY__(20),

    TEST_EXPECT__________("m"),
    TEST_EXPECT__________(""),

    TEST_RELEASE__U("j"),
    TEST_DELAY__(50),

    TEST_END()
};

// ifShortcut noConsume: matched keys are not consumed
static const test_action_t test_ifshortcut_noconsume[] = {
    TEST_SET_MACRO("j", "ifShortcut noConsume k final tapKey n\n holdKey j"),
    TEST_SET_ACTION("k", "k"),

    // Hold 'j', press 'k' -> triggers n, but 'k' is not consumed so it also outputs
    TEST_PRESS______("j"),
    TEST_DELAY__(20),
    TEST_PRESS______("k"),
    TEST_DELAY__(50),

    TEST_EXPECT__________("n"),
    TEST_EXPECT__________(""),   // tapKey n releases
    TEST_EXPECT__________("k"),  // k is processed (not consumed)

    TEST_RELEASE__U("k"),
    TEST_RELEASE__U("j"),
    TEST_DELAY__(50),

    TEST_EXPECT__________(""),

    TEST_END()
};

// ifShortcut anyOrder: keys can be pressed in any order
static const test_action_t test_ifshortcut_anyorder[] = {
    TEST_SET_MACRO("j", "ifShortcut anyOrder k l final tapKey n\n holdKey j"),
    TEST_SET_ACTION("k", "k"),
    TEST_SET_ACTION("l", "l"),

    // Press 'j', then 'l', then 'k' (reverse order) -> should still match
    TEST_PRESS______("j"),
    TEST_DELAY__(20),
    TEST_PRESS______("l"),
    TEST_DELAY__(20),
    TEST_PRESS______("k"),
    TEST_DELAY__(50),

    TEST_EXPECT__________("n"),
    TEST_EXPECT__________(""),

    TEST_RELEASE__U("k"),
    TEST_RELEASE__U("l"),
    TEST_RELEASE__U("j"),
    TEST_DELAY__(50),

    TEST_END()
};

// ifShortcut orGate: any single key triggers the condition
static const test_action_t test_ifshortcut_orgate[] = {
    TEST_SET_MACRO("j", "ifShortcut orGate k l final tapKey n\n holdKey j"),
    TEST_SET_ACTION("k", "k"),
    TEST_SET_ACTION("l", "l"),

    // Press 'j', then just 'l' (not 'k') -> orGate means either triggers
    TEST_PRESS______("j"),
    TEST_DELAY__(20),
    TEST_PRESS______("l"),
    TEST_DELAY__(50),

    TEST_EXPECT__________("n"),
    TEST_EXPECT__________(""),

    TEST_RELEASE__U("l"),
    TEST_RELEASE__U("j"),
    TEST_DELAY__(50),

    TEST_END()
};

// ifShortcut timeoutIn: fails if timeout expires before match
static const test_action_t test_ifshortcut_timeout[] = {
    TEST_SET_MACRO("j", "ifShortcut timeoutIn 50 k final tapKey n\n holdKey j"),
    TEST_SET_ACTION("k", "k"),

    // Press 'j', wait past timeout, then press 'k' -> should NOT match
    TEST_PRESS______("j"),
    TEST_DELAY__(100),

    // Timeout expired, falls through to holdKey j
    TEST_EXPECT__________("j"),

    TEST_PRESS______("k"),
    TEST_DELAY__(50),

    TEST_EXPECT__________("j k"),

    TEST_RELEASE__U("k"),
    TEST_RELEASE__U("j"),
    TEST_DELAY__(50),

    TEST_EXPECT__________("j"),
    TEST_EXPECT__________(""),

    TEST_END()
};

// ifShortcut transitive: timer references most recent key, not activation key
static const test_action_t test_ifshortcut_transitive[] = {
    TEST_SET_MACRO("j", "ifShortcut transitive k l final tapKey n\n holdKey j"),
    TEST_SET_ACTION("k", "k"),
    TEST_SET_ACTION("l", "l"),

    // With transitive: can release 'j' after pressing 'k', continue with 'l'
    TEST_PRESS______("j"),
    TEST_DELAY__(20),
    TEST_PRESS______("k"),
    TEST_DELAY__(20),
    TEST_RELEASE__U("j"),
    TEST_DELAY__(20),
    TEST_PRESS______("l"),
    TEST_DELAY__(50),

    TEST_EXPECT__________("n"),
    TEST_EXPECT__________(""),

    TEST_RELEASE__U("l"),
    TEST_RELEASE__U("k"),
    TEST_DELAY__(50),

    TEST_END()
};

// =============================================================================
// ifGesture tests
// =============================================================================

// ifGesture basic: sequential key taps (like Vim's gg)
static const test_action_t test_ifgesture_basic[] = {
    TEST_SET_MACRO("j", "ifGesture k final tapKey n\n holdKey j"),
    TEST_SET_ACTION("k", "k"),

    // Tap 'j', then tap 'k' -> should trigger tapKey n
    TEST_PRESS______("j"),
    TEST_DELAY__(20),
    TEST_RELEASE__U("j"),
    TEST_DELAY__(20),
    TEST_PRESS______("k"),
    TEST_DELAY__(50),

    TEST_EXPECT__________("n"),
    TEST_EXPECT__________(""),

    TEST_RELEASE__U("k"),
    TEST_DELAY__(50),

    TEST_END()
};

// ifGesture: doubletap using $thisKeyId
static const test_action_t test_ifgesture_doubletap[] = {
    TEST_SET_MACRO("j", "ifGesture j final tapKey n\n holdKey j"),

    // Tap 'j' twice -> triggers n
    TEST_PRESS______("j"),
    TEST_DELAY__(60),
    TEST_RELEASE__U("j"),
    TEST_DELAY__(60),
    TEST_PRESS______("j"),
    TEST_DELAY__(60),

    TEST_EXPECT__________("n"),
    TEST_EXPECT__________(""),

    TEST_RELEASE__U("j"),
    TEST_DELAY__(50),

    TEST_END()
};

// ifNotGesture: triggers when gesture is NOT completed
static const test_action_t test_ifnotgesture[] = {
    TEST_SET_MACRO("j", "ifNotGesture k final tapKey m\n tapKey j"),
    TEST_SET_ACTION("k", "k"),
    TEST_SET_ACTION("l", "l"),

    // Tap 'j', then tap 'l' (not 'k') -> ifNotGesture triggers
    TEST_PRESS______("j"),
    TEST_DELAY__(20),
    TEST_RELEASE__U("j"),
    TEST_DELAY__(20),
    TEST_PRESS______("l"),
    TEST_DELAY__(50),

    TEST_EXPECT__________("m"),
    TEST_EXPECT__________(""),   // tapKey m releases
    TEST_EXPECT__________("l"),  // l is processed (not consumed)

    TEST_RELEASE__U("l"),
    TEST_DELAY__(50),

    TEST_EXPECT__________(""),

    TEST_END()
};

// ifGesture noConsume: matched keys are not consumed
static const test_action_t test_ifgesture_noconsume[] = {
    TEST_SET_MACRO("j", "ifGesture noConsume k final tapKey n\n holdKey j"),
    TEST_SET_ACTION("k", "k"),

    // Tap 'j', tap 'k' -> triggers n, but 'k' also outputs
    TEST_PRESS______("j"),
    TEST_DELAY__(20),
    TEST_RELEASE__U("j"),
    TEST_DELAY__(20),
    TEST_PRESS______("k"),
    TEST_DELAY__(50),

    TEST_EXPECT__________("n"),
    TEST_EXPECT__________(""),   // tapKey n releases
    TEST_EXPECT__________("k"),  // k is processed (not consumed)

    TEST_RELEASE__U("k"),
    TEST_DELAY__(50),

    TEST_EXPECT__________(""),

    TEST_END()
};

// ifGesture anyOrder: keys can be tapped in any order
static const test_action_t test_ifgesture_anyorder[] = {
    TEST_SET_MACRO("j", "ifGesture anyOrder k l final tapKey n\n holdKey j"),
    TEST_SET_ACTION("k", "k"),
    TEST_SET_ACTION("l", "l"),

    // Tap 'j', 'l', 'k' (reverse order) -> should still match
    TEST_PRESS______("j"),
    TEST_DELAY__(20),
    TEST_RELEASE__U("j"),
    TEST_DELAY__(20),
    TEST_PRESS______("l"),
    TEST_DELAY__(20),
    TEST_RELEASE__U("l"),
    TEST_DELAY__(20),
    TEST_PRESS______("k"),
    TEST_DELAY__(50),

    TEST_EXPECT__________("n"),
    TEST_EXPECT__________(""),

    TEST_RELEASE__U("k"),
    TEST_DELAY__(50),

    TEST_END()
};

// ifGesture orGate: any single key triggers the condition
static const test_action_t test_ifgesture_orgate[] = {
    TEST_SET_MACRO("j", "ifGesture orGate k l final tapKey n\n holdKey j"),
    TEST_SET_ACTION("k", "k"),
    TEST_SET_ACTION("l", "l"),

    // Tap 'j', then just 'l' -> orGate means either triggers
    TEST_PRESS______("j"),
    TEST_DELAY__(20),
    TEST_RELEASE__U("j"),
    TEST_DELAY__(20),
    TEST_PRESS______("l"),
    TEST_DELAY__(50),

    TEST_EXPECT__________("n"),
    TEST_EXPECT__________(""),

    TEST_RELEASE__U("l"),
    TEST_DELAY__(50),

    TEST_END()
};

// ifGesture timeoutIn: fails if timeout expires
static const test_action_t test_ifgesture_timeout[] = {
    TEST_SET_MACRO("j", "ifGesture timeoutIn 50 k final tapKey n\n tapKey j"),
    TEST_SET_ACTION("k", "k"),

    // Tap 'j', wait past timeout -> gesture fails, tapKey j executes
    TEST_PRESS______("j"),
    TEST_DELAY__(20),
    TEST_RELEASE__U("j"),
    TEST_DELAY__(100),

    TEST_EXPECT__________("j"),
    TEST_EXPECT__________(""),

    TEST_END()
};

// ifGesture cancelIn: breaks macro entirely on timeout
static const test_action_t test_ifgesture_cancelin[] = {
    TEST_SET_MACRO("j", "ifGesture cancelIn 50 k final tapKey n\n tapKey j"),
    TEST_SET_ACTION("k", "k"),

    // Tap 'j', wait past cancelIn -> macro breaks entirely, no output
    TEST_PRESS______("j"),
    TEST_DELAY__(20),
    TEST_RELEASE__U("j"),
    TEST_DELAY__(100),

    // cancelIn breaks the macro, no tapKey j fallback
    TEST_CHECK_NOW(""),

    TEST_END()
};

static const test_t ifshortcut_gesture_tests[] = {
    // ifShortcut tests
    { .name = "ifshortcut_basic",      .actions = test_ifshortcut_basic },
    { .name = "ifshortcut_no_match",   .actions = test_ifshortcut_no_match },
    { .name = "ifnotshortcut",         .actions = test_ifnotshortcut },
    { .name = "ifshortcut_noconsume",  .actions = test_ifshortcut_noconsume },
    { .name = "ifshortcut_anyorder",   .actions = test_ifshortcut_anyorder },
    { .name = "ifshortcut_orgate",     .actions = test_ifshortcut_orgate },
    { .name = "ifshortcut_timeout",    .actions = test_ifshortcut_timeout },
    { .name = "ifshortcut_transitive", .actions = test_ifshortcut_transitive },
    // ifGesture tests
    { .name = "ifgesture_basic",       .actions = test_ifgesture_basic },
    { .name = "ifgesture_doubletap",   .actions = test_ifgesture_doubletap },
    { .name = "ifnotgesture",          .actions = test_ifnotgesture },
    { .name = "ifgesture_noconsume",   .actions = test_ifgesture_noconsume },
    { .name = "ifgesture_anyorder",    .actions = test_ifgesture_anyorder },
    { .name = "ifgesture_orgate",      .actions = test_ifgesture_orgate },
    { .name = "ifgesture_timeout",     .actions = test_ifgesture_timeout },
    { .name = "ifgesture_cancelin",    .actions = test_ifgesture_cancelin },
};

const test_module_t TestModule_IfShortcutGesture = {
    .name = "IfShortcutGesture",
    .tests = ifshortcut_gesture_tests,
    .testCount = sizeof(ifshortcut_gesture_tests) / sizeof(ifshortcut_gesture_tests[0])
};
