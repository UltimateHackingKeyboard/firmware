#include "tests.h"

// Reproduces a reported UHK60-vs-UHK80 behavior difference around the
// playtime boundary after `delayUntilReleaseMax`.
//
// Reported macro (simplified):
//
//     setVar myTimeout 500
//     delayUntilReleaseMax $myTimeout
//     ifNotInterrupted ifNotPlaytime $myTimeout goTo releasedQuickly
//     ...long path...
//     releasedQuickly:
//
// When the key is held *longer* than the timeout, `delayUntilReleaseMax`
// times out instead of seeing a release. The intent of `ifNotPlaytime
// $myTimeout` is then "we are past the timeout, so do NOT treat this as a
// quick release". `ifNotPlaytime` is true while elapsed <= timeout.
//
// The discrepancy:
//   - UHK80 wakes a hair *after* startTime + timeout, so elapsed > timeout
//     and `ifNotPlaytime` is false -> the quick branch is NOT taken (expected).
//   - UHK60 wakes exactly at startTime + timeout, so elapsed == timeout and
//     `ifNotPlaytime` is true -> the quick branch IS taken (unexpected).
//
// The user-side workaround is `ifNotPlaytime ($myTimeout-1)`.
//
// This test holds the key well past the timeout and asserts the EXPECTED
// behavior: the "long" branch runs (tapKey n) and the "quick" branch does
// not (tapKey o). It passes on UHK80 and fails on UHK60, reproducing the
// report.

static const test_action_t test_playtime_boundary_after_release_max[] = {
    TEST_SET_MACRO("u",
        "setVar myTimeout 100\n"
        "delayUntilReleaseMax $myTimeout\n"
        "ifNotInterrupted ifNotPlaytime $myTimeout tapKey o\n"   // "releasedQuickly" branch
        "ifPlaytime $myTimeout tapKey n"                          // long branch
    ),
    TEST_EXPECT__________(""),

    TEST_PRESS______("u"),
    // Hold well past the 100ms timeout (plus ~50ms press debounce) so that
    // delayUntilReleaseMax times out while the key is still held.
    TEST_DELAY__(150),
    // Expected: the long branch runs.
    TEST_EXPECT__________("n"),
    TEST_EXPECT__________(""),
    TEST_RELEASE__U("u"),
    TEST_DELAY__(50),
    TEST_END()
};

static const test_t playtime_tests[] = {
    { .name = "playtime_boundary_after_release_max", .actions = test_playtime_boundary_after_release_max },
};

const test_module_t TestModule_Playtime = {
    .name = "Playtime",
    .tests = playtime_tests,
    .testCount = sizeof(playtime_tests) / sizeof(playtime_tests[0])
};
