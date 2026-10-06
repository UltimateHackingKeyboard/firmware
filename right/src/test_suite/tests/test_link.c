#include "tests.h"
#include "slot.h"
#include "module.h"
#include "uart_defs.h"
#include "uart_parser.h"
#include "test_suite/test_statistics.h"

// Tests that the left-right bridge recovers from a stressed link without losing or reordering
// key events. Both halves run the script; each presses only the keys it owns, so what the
// right half validates has actually crossed the wire.
//
// The failure mode under test is the left half's scanner stalling: its sender blocks until a
// frame is acked, so lost acks delay the *next* state change.

// What one retry costs the sender: the ack budget plus the frame's own time on the wire,
// which the bridge charges separately.
#define LINK_FRAME_BYTES (3 + 2 + (MAX_KEY_COUNT_PER_MODULE/8+1) \
    + UART_LINK_START_END_BYTE_LEN + UART_LINK_CRC_BUF_LEN)
#define LINK_RETRY_COST_MS (UART_RESEND_DELAY + UART_FRAME_WIRE_TIME_MS(LINK_FRAME_BYTES))

// Retries the link is expected to absorb without the user noticing.
#define LINK_ABSORBED_RETRIES 2

// A key's changes are debounced (Cfg.DebounceTimePress/Release default): once a press or
// release commits, the key's next change waits the debounce out. Every phase - held, then
// released - must outlast it even when the change that opened it arrived late by the absorbed
// retries; otherwise the next change is postponed past a later key's and the report order
// breaks for reasons unrelated to the link.
#define LINK_DEBOUNCE_MS 50
#define LINK_PHASE_MIN_MS (LINK_DEBOUNCE_MS + LINK_ABSORBED_RETRIES * LINK_RETRY_COST_MS)

// Left-half keys are 'f' (81), 'g' (82), 'b' (89) and 'v' (88); 'j' (16) and 'k' are on the
// right.
#define LEFT_A "f"
#define LEFT_B "g"
#define LEFT_C "b"
#define LEFT_D "v"
#define RIGHT_A "j"
#define RIGHT_B "k"

// Stress runs only between two quiet steps. The right half sends StartTest to the left just
// before the script begins, and the left trails the right by that message's latency, so
// without them the StartTest exchange itself could be stressed - a dropped ack resends it,
// and the left would restart its script a second time, out of step with the right. A quiet
// step covers the exchange: its ack loop measures well below it (at most ~14ms).
#define LINK_QUIET_MS 22
#define LINK_STRESS_ON TEST_DELAY__(LINK_QUIET_MS), TEST_SET_BOOL(&UartStress_Active, true)
#define LINK_STRESS_OFF TEST_SET_BOOL(&UartStress_Active, false), TEST_DELAY__(LINK_QUIET_MS)

// Probes the stall directly. A dropped ack never delays the frame it belongs to - that one
// already arrived - only the sender's *next* frame. So a probe is: left change L1, a second
// left change L2 shortly after, and a right change R that L2 must still beat. Retries on L1
// hold L2 back until the sender is free again; L2's own corruption retries delay L2 itself.
//
// Measured on the stressed link: a timeout retry costs ~9.3ms, a nack retry ~3ms, and L2
// lands ~6ms (+-2) after the sender frees up - after L1's 1 timeout at ~18ms, after 2 at
// ~28ms, so after 3 at ~37ms. Nacks being cheap, the boundary is in timeouts: R midway
// between two and three fails, per a model of the link with these costs, ~2% of probes
// whose L1 needed two timeouts and ~99% of those needing three.
//
// L2 goes as early as possible: any retry on L1 still holds it back, and its own corruption
// retries get the most room before R.
#define LINK_PROBE_L2_MS 4
#define LINK_PROBE_R_MS 32

// Two probes, on disjoint keys, share a slot pair; every key changes once per two slots. A
// slot outlasts even a three-timeout L2, so a failing probe does not spill into the next.
#define LINK_SLOT_MS 40
_Static_assert(LINK_PROBE_R_MS < LINK_SLOT_MS, "probe does not fit its slot");
_Static_assert(2 * LINK_SLOT_MS >= LINK_PHASE_MIN_MS,
    "probe keys are not held long enough to clear debouncing");

#define LINK_PROBE(L1, L1_EXPECT, L2, L2_EXPECT, R, R_EXPECT) \
    L1, TEST_EXPECT__________(L1_EXPECT), TEST_DELAY__(LINK_PROBE_L2_MS), \
    L2, TEST_EXPECT__________(L2_EXPECT), TEST_DELAY__(LINK_PROBE_R_MS - LINK_PROBE_L2_MS), \
    R, TEST_EXPECT__________(R_EXPECT), TEST_DELAY__(LINK_SLOT_MS - LINK_PROBE_R_MS)

#define LINK_PROBES_PER_CYCLE 4

// Sized so a healthy run fails at TEST_SPURIOUS_FAILURE_RATE, the rate the suite's confidence
// figures assume: measured, a probe fails ~8e-4 of the time (the model above says ~4e-4, the
// excess unexplained), so 280 probes fail ~20% of runs while ~72% see a two-timeout streak.
#define LINK_PROBE_CYCLES 70

static const test_action_t test_link_probe[] = {
    LINK_STRESS_ON,
    TEST_SET_ACTION(LEFT_A, "f"),
    TEST_SET_ACTION(LEFT_B, "g"),
    TEST_SET_ACTION(LEFT_C, "b"),
    TEST_SET_ACTION(LEFT_D, "v"),
    TEST_SET_ACTION(RIGHT_A, "j"),
    TEST_SET_ACTION(RIGHT_B, "k"),

    LINK_PROBE(TEST_PRESS______(LEFT_A), "f", TEST_PRESS______(LEFT_B), "f g",
            TEST_PRESS______(RIGHT_A), "f g j"),
    LINK_PROBE(TEST_PRESS______(LEFT_C), "f g j b", TEST_PRESS______(LEFT_D), "f g j b v",
            TEST_PRESS______(RIGHT_B), "f g j b v k"),
    LINK_PROBE(TEST_RELEASE__U(LEFT_A), "g j b v k", TEST_RELEASE__U(LEFT_B), "j b v k",
            TEST_RELEASE__U(RIGHT_A), "b v k"),
    LINK_PROBE(TEST_RELEASE__U(LEFT_C), "v k", TEST_RELEASE__U(LEFT_D), "k",
            TEST_RELEASE__U(RIGHT_B), ""),

    LINK_STRESS_OFF,
    TEST_END()
};

static const test_t link_tests[] = {
    {
        .name = "probe",
        .actions = test_link_probe,
        .linkTestId = LinkTestId_Probe,
        .repeatCount = LINK_PROBE_CYCLES,
    },
};

const test_module_t TestModule_Link = {
    .name = "Link",
    .tests = link_tests,
    .testCount = sizeof(link_tests) / sizeof(link_tests[0])
};
