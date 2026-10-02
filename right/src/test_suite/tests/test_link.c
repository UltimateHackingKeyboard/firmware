#include "tests.h"
#include "slot.h"

// Link tests exercise the left-right bridge rather than any key-handling feature. They are
// the only tests whose input is not applied locally: the left half runs the same script and
// presses its own keys, so what the right half sees has actually traversed the link.
//
// The failure these catch is key loss and reordering, not link loss. A stalled left->right
// link makes a left key arrive late or not at all, while a right key pressed afterwards is
// handled immediately - so the output order inverts, or an event vanishes because the left
// scanner was blocked across the whole press.
//
// Input actions are spaced LINK_STEP_MS apart: wide enough that a couple of consecutive
// retries are absorbed without failing, narrow enough that a genuinely stalled link cannot
// hide. Every input action is followed by exactly one expectation, which makes the sequence
// of expectations a strict ordering check.
// 50ms of this is the press/release debounce the rig cannot avoid (see test_suite/CLAUDE.md);
// the remaining 45ms is the margin that lets a couple of consecutive retries be absorbed
// without failing. Lowering the debounce instead was tried and rejected: at 10ms the right
// half's own switch chatter registers and the test fails for reasons unrelated to the link.
#define LINK_STEP_MS 95

// 'f' is key id 81 -> slot 1 (SlotId_LeftKeyboardHalf), 'j' is 16 -> slot 0 (right half).
#define LEFT_KEY "f"
#define RIGHT_KEY "j"

// Alternate halves on every action. Each press adds its own character to the report and each
// release removes it, so a late or lost left event shows up as a wrong report at the very
// next expectation.
static const test_action_t test_link_interleaved[] = {
    TEST_SET_ACTION(LEFT_KEY, "f"),
    TEST_SET_ACTION(RIGHT_KEY, "j"),

    TEST_PRESS______(LEFT_KEY),
    TEST_DELAY__(LINK_STEP_MS),
    TEST_EXPECT__________("f"),
    TEST_PRESS______(RIGHT_KEY),
    TEST_DELAY__(LINK_STEP_MS),
    TEST_EXPECT__________("f j"),
    TEST_RELEASE__U(LEFT_KEY),
    TEST_DELAY__(LINK_STEP_MS),
    TEST_EXPECT__________("j"),
    TEST_RELEASE__U(RIGHT_KEY),
    TEST_DELAY__(LINK_STEP_MS),
    TEST_EXPECT__________(""),

    TEST_PRESS______(RIGHT_KEY),
    TEST_DELAY__(LINK_STEP_MS),
    TEST_EXPECT__________("j"),
    TEST_PRESS______(LEFT_KEY),
    TEST_DELAY__(LINK_STEP_MS),
    TEST_EXPECT__________("f j"),
    TEST_RELEASE__U(RIGHT_KEY),
    TEST_DELAY__(LINK_STEP_MS),
    TEST_EXPECT__________("f"),
    TEST_RELEASE__U(LEFT_KEY),
    TEST_DELAY__(LINK_STEP_MS),
    TEST_EXPECT__________(""),

    TEST_END()
};

// Left half alone, no interleaving: isolates plain loss from ordering. A press and release
// that both fall inside one stall are coalesced away and never reported at all.
static const test_action_t test_link_left_only[] = {
    TEST_SET_ACTION(LEFT_KEY, "f"),

    TEST_PRESS______(LEFT_KEY),
    TEST_DELAY__(LINK_STEP_MS),
    TEST_EXPECT__________("f"),
    TEST_RELEASE__U(LEFT_KEY),
    TEST_DELAY__(LINK_STEP_MS),
    TEST_EXPECT__________(""),
    TEST_PRESS______(LEFT_KEY),
    TEST_DELAY__(LINK_STEP_MS),
    TEST_EXPECT__________("f"),
    TEST_RELEASE__U(LEFT_KEY),
    TEST_DELAY__(LINK_STEP_MS),
    TEST_EXPECT__________(""),
    TEST_PRESS______(LEFT_KEY),
    TEST_DELAY__(LINK_STEP_MS),
    TEST_EXPECT__________("f"),
    TEST_RELEASE__U(LEFT_KEY),
    TEST_DELAY__(LINK_STEP_MS),
    TEST_EXPECT__________(""),

    TEST_END()
};

// Ids are the wire contract between the halves: the right half sends one in
// MessengerCommand_StartTest and the left half looks the script up by it. Keep them stable.
typedef enum {
    LinkTestId_Interleaved = 1,
    LinkTestId_LeftOnly = 2,
} link_test_id_t;

const test_action_t* LinkTest_GetActions(uint8_t linkTestId) {
    switch (linkTestId) {
        case LinkTestId_Interleaved:
            return test_link_interleaved;
        case LinkTestId_LeftOnly:
            return test_link_left_only;
        default:
            return NULL;
    }
}

static const test_t link_tests[] = {
    { .name = "interleaved", .actions = test_link_interleaved, .linkTestId = LinkTestId_Interleaved },
    { .name = "left_only", .actions = test_link_left_only, .linkTestId = LinkTestId_LeftOnly },
};

const test_module_t TestModule_Link = {
    .name = "Link",
    .tests = link_tests,
    .testCount = sizeof(link_tests) / sizeof(link_tests[0])
};
