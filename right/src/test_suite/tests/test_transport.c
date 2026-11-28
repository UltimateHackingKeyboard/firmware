#include "tests.h"
#include "test_suite/test_suite.h"
#include "test_suite/test_hooks.h"
#include "event_scheduler.h"
#include "logger.h"
#include "hid/transport.h"
#include "usb_report_sender.h"

// A report attempt fails when its send is dropped or, sent, its callback is. The engine holds
// the report until it is delivered, so a run fails only when one of its reports - a press and a
// release for each of the 60 keystrokes - fails THROTTLE_MAX_RETRIES + 1 attempts in a row and
// is given up. Folded at compile time.
#define TRANSPORT_REPORTS_PER_RUN 120
#define TRANSPORT_ATTEMPT_PASS_RATE (1.0 - 1.0 / UNRELIABLE_TRANSPORT_DROP_RECIPROCAL)
#define TRANSPORT_ATTEMPT_FAILURE_RATE (1.0 - TRANSPORT_ATTEMPT_PASS_RATE * TRANSPORT_ATTEMPT_PASS_RATE)
#define TRANSPORT_RUN_FAILURE_RATE \
    (TRANSPORT_REPORTS_PER_RUN * __builtin_pow(TRANSPORT_ATTEMPT_FAILURE_RATE, THROTTLE_MAX_RETRIES + 1))
#define TRANSPORT_CYCLE_MS 3600 // the script's delays: 60 keystrokes, (10 + 50) ms each
#define TRANSPORT_RUNS (TEST_REPEAT_FOR_SECONDS * 1000 / TRANSPORT_CYCLE_MS)

// Enable the unreliable transport test mode, type a short (non-meaningful)
// paragraph by pressing and releasing several keys over the normal key path, and
// verify that every keystroke still arrives correctly despite the artificially
// dropped/duplicated transport reports (which the transport layer must recover
// from via retries). Restore the transport mode at the end.
static const test_action_t test_transport_unreliable_typing[] = {
    TEST_SET_BOOL(&UnreliableTransportTestMode, true),
    TEST_SET_ACTION("u", "u"),
    TEST_SET_ACTION("i", "i"),
    TEST_SET_ACTION("o", "o"),
    TEST_SET_ACTION("p", "p"),
    TEST_SET_ACTION("h", "h"),
    TEST_SET_ACTION("j", "j"),
    TEST_SET_ACTION("k", "k"),
    TEST_SET_ACTION("l", "l"),
    TEST_SET_ACTION("n", "n"),
    TEST_SET_ACTION("m", "m"),
    TEST_SET_ACTION("y", "y"),
    TEST_PRESS______("l"),
    TEST_DELAY__(10),
    TEST_EXPECT__________("l"),
    TEST_RELEASE__U("l"),
    TEST_DELAY__(50),
    TEST_EXPECT__________(""),
    TEST_PRESS______("y"),
    TEST_DELAY__(10),
    TEST_EXPECT__________("y"),
    TEST_RELEASE__U("y"),
    TEST_DELAY__(50),
    TEST_EXPECT__________(""),
    TEST_PRESS______("o"),
    TEST_DELAY__(10),
    TEST_EXPECT__________("o"),
    TEST_RELEASE__U("o"),
    TEST_DELAY__(50),
    TEST_EXPECT__________(""),
    TEST_PRESS______("j"),
    TEST_DELAY__(10),
    TEST_EXPECT__________("j"),
    TEST_RELEASE__U("j"),
    TEST_DELAY__(50),
    TEST_EXPECT__________(""),
    TEST_PRESS______("n"),
    TEST_DELAY__(10),
    TEST_EXPECT__________("n"),
    TEST_RELEASE__U("n"),
    TEST_DELAY__(50),
    TEST_EXPECT__________(""),
    TEST_PRESS______("u"),
    TEST_DELAY__(10),
    TEST_EXPECT__________("u"),
    TEST_RELEASE__U("u"),
    TEST_DELAY__(50),
    TEST_EXPECT__________(""),
    TEST_PRESS______("p"),
    TEST_DELAY__(10),
    TEST_EXPECT__________("p"),
    TEST_RELEASE__U("p"),
    TEST_DELAY__(50),
    TEST_EXPECT__________(""),
    TEST_PRESS______("k"),
    TEST_DELAY__(10),
    TEST_EXPECT__________("k"),
    TEST_RELEASE__U("k"),
    TEST_DELAY__(50),
    TEST_EXPECT__________(""),
    TEST_PRESS______("m"),
    TEST_DELAY__(10),
    TEST_EXPECT__________("m"),
    TEST_RELEASE__U("m"),
    TEST_DELAY__(50),
    TEST_EXPECT__________(""),
    TEST_PRESS______("i"),
    TEST_DELAY__(10),
    TEST_EXPECT__________("i"),
    TEST_RELEASE__U("i"),
    TEST_DELAY__(50),
    TEST_EXPECT__________(""),
    TEST_PRESS______("h"),
    TEST_DELAY__(10),
    TEST_EXPECT__________("h"),
    TEST_RELEASE__U("h"),
    TEST_DELAY__(50),
    TEST_EXPECT__________(""),
    TEST_PRESS______("l"),
    TEST_DELAY__(10),
    TEST_EXPECT__________("l"),
    TEST_RELEASE__U("l"),
    TEST_DELAY__(50),
    TEST_EXPECT__________(""),
    TEST_PRESS______("y"),
    TEST_DELAY__(10),
    TEST_EXPECT__________("y"),
    TEST_RELEASE__U("y"),
    TEST_DELAY__(50),
    TEST_EXPECT__________(""),
    TEST_PRESS______("o"),
    TEST_DELAY__(10),
    TEST_EXPECT__________("o"),
    TEST_RELEASE__U("o"),
    TEST_DELAY__(50),
    TEST_EXPECT__________(""),
    TEST_PRESS______("j"),
    TEST_DELAY__(10),
    TEST_EXPECT__________("j"),
    TEST_RELEASE__U("j"),
    TEST_DELAY__(50),
    TEST_EXPECT__________(""),
    TEST_PRESS______("n"),
    TEST_DELAY__(10),
    TEST_EXPECT__________("n"),
    TEST_RELEASE__U("n"),
    TEST_DELAY__(50),
    TEST_EXPECT__________(""),
    TEST_PRESS______("u"),
    TEST_DELAY__(10),
    TEST_EXPECT__________("u"),
    TEST_RELEASE__U("u"),
    TEST_DELAY__(50),
    TEST_EXPECT__________(""),
    TEST_PRESS______("p"),
    TEST_DELAY__(10),
    TEST_EXPECT__________("p"),
    TEST_RELEASE__U("p"),
    TEST_DELAY__(50),
    TEST_EXPECT__________(""),
    TEST_PRESS______("k"),
    TEST_DELAY__(10),
    TEST_EXPECT__________("k"),
    TEST_RELEASE__U("k"),
    TEST_DELAY__(50),
    TEST_EXPECT__________(""),
    TEST_PRESS______("m"),
    TEST_DELAY__(10),
    TEST_EXPECT__________("m"),
    TEST_RELEASE__U("m"),
    TEST_DELAY__(50),
    TEST_EXPECT__________(""),
    TEST_PRESS______("i"),
    TEST_DELAY__(10),
    TEST_EXPECT__________("i"),
    TEST_RELEASE__U("i"),
    TEST_DELAY__(50),
    TEST_EXPECT__________(""),
    TEST_PRESS______("h"),
    TEST_DELAY__(10),
    TEST_EXPECT__________("h"),
    TEST_RELEASE__U("h"),
    TEST_DELAY__(50),
    TEST_EXPECT__________(""),
    TEST_PRESS______("l"),
    TEST_DELAY__(10),
    TEST_EXPECT__________("l"),
    TEST_RELEASE__U("l"),
    TEST_DELAY__(50),
    TEST_EXPECT__________(""),
    TEST_PRESS______("y"),
    TEST_DELAY__(10),
    TEST_EXPECT__________("y"),
    TEST_RELEASE__U("y"),
    TEST_DELAY__(50),
    TEST_EXPECT__________(""),
    TEST_PRESS______("o"),
    TEST_DELAY__(10),
    TEST_EXPECT__________("o"),
    TEST_RELEASE__U("o"),
    TEST_DELAY__(50),
    TEST_EXPECT__________(""),
    TEST_PRESS______("j"),
    TEST_DELAY__(10),
    TEST_EXPECT__________("j"),
    TEST_RELEASE__U("j"),
    TEST_DELAY__(50),
    TEST_EXPECT__________(""),
    TEST_PRESS______("n"),
    TEST_DELAY__(10),
    TEST_EXPECT__________("n"),
    TEST_RELEASE__U("n"),
    TEST_DELAY__(50),
    TEST_EXPECT__________(""),
    TEST_PRESS______("u"),
    TEST_DELAY__(10),
    TEST_EXPECT__________("u"),
    TEST_RELEASE__U("u"),
    TEST_DELAY__(50),
    TEST_EXPECT__________(""),
    TEST_PRESS______("p"),
    TEST_DELAY__(10),
    TEST_EXPECT__________("p"),
    TEST_RELEASE__U("p"),
    TEST_DELAY__(50),
    TEST_EXPECT__________(""),
    TEST_PRESS______("k"),
    TEST_DELAY__(10),
    TEST_EXPECT__________("k"),
    TEST_RELEASE__U("k"),
    TEST_DELAY__(50),
    TEST_EXPECT__________(""),
    TEST_PRESS______("m"),
    TEST_DELAY__(10),
    TEST_EXPECT__________("m"),
    TEST_RELEASE__U("m"),
    TEST_DELAY__(50),
    TEST_EXPECT__________(""),
    TEST_PRESS______("i"),
    TEST_DELAY__(10),
    TEST_EXPECT__________("i"),
    TEST_RELEASE__U("i"),
    TEST_DELAY__(50),
    TEST_EXPECT__________(""),
    TEST_PRESS______("h"),
    TEST_DELAY__(10),
    TEST_EXPECT__________("h"),
    TEST_RELEASE__U("h"),
    TEST_DELAY__(50),
    TEST_EXPECT__________(""),
    TEST_PRESS______("l"),
    TEST_DELAY__(10),
    TEST_EXPECT__________("l"),
    TEST_RELEASE__U("l"),
    TEST_DELAY__(50),
    TEST_EXPECT__________(""),
    TEST_PRESS______("y"),
    TEST_DELAY__(10),
    TEST_EXPECT__________("y"),
    TEST_RELEASE__U("y"),
    TEST_DELAY__(50),
    TEST_EXPECT__________(""),
    TEST_PRESS______("o"),
    TEST_DELAY__(10),
    TEST_EXPECT__________("o"),
    TEST_RELEASE__U("o"),
    TEST_DELAY__(50),
    TEST_EXPECT__________(""),
    TEST_PRESS______("j"),
    TEST_DELAY__(10),
    TEST_EXPECT__________("j"),
    TEST_RELEASE__U("j"),
    TEST_DELAY__(50),
    TEST_EXPECT__________(""),
    TEST_PRESS______("n"),
    TEST_DELAY__(10),
    TEST_EXPECT__________("n"),
    TEST_RELEASE__U("n"),
    TEST_DELAY__(50),
    TEST_EXPECT__________(""),
    TEST_PRESS______("u"),
    TEST_DELAY__(10),
    TEST_EXPECT__________("u"),
    TEST_RELEASE__U("u"),
    TEST_DELAY__(50),
    TEST_EXPECT__________(""),
    TEST_PRESS______("p"),
    TEST_DELAY__(10),
    TEST_EXPECT__________("p"),
    TEST_RELEASE__U("p"),
    TEST_DELAY__(50),
    TEST_EXPECT__________(""),
    TEST_PRESS______("k"),
    TEST_DELAY__(10),
    TEST_EXPECT__________("k"),
    TEST_RELEASE__U("k"),
    TEST_DELAY__(50),
    TEST_EXPECT__________(""),
    TEST_PRESS______("m"),
    TEST_DELAY__(10),
    TEST_EXPECT__________("m"),
    TEST_RELEASE__U("m"),
    TEST_DELAY__(50),
    TEST_EXPECT__________(""),
    TEST_PRESS______("i"),
    TEST_DELAY__(10),
    TEST_EXPECT__________("i"),
    TEST_RELEASE__U("i"),
    TEST_DELAY__(50),
    TEST_EXPECT__________(""),
    TEST_PRESS______("h"),
    TEST_DELAY__(10),
    TEST_EXPECT__________("h"),
    TEST_RELEASE__U("h"),
    TEST_DELAY__(50),
    TEST_EXPECT__________(""),
    TEST_PRESS______("l"),
    TEST_DELAY__(10),
    TEST_EXPECT__________("l"),
    TEST_RELEASE__U("l"),
    TEST_DELAY__(50),
    TEST_EXPECT__________(""),
    TEST_PRESS______("y"),
    TEST_DELAY__(10),
    TEST_EXPECT__________("y"),
    TEST_RELEASE__U("y"),
    TEST_DELAY__(50),
    TEST_EXPECT__________(""),
    TEST_PRESS______("o"),
    TEST_DELAY__(10),
    TEST_EXPECT__________("o"),
    TEST_RELEASE__U("o"),
    TEST_DELAY__(50),
    TEST_EXPECT__________(""),
    TEST_PRESS______("j"),
    TEST_DELAY__(10),
    TEST_EXPECT__________("j"),
    TEST_RELEASE__U("j"),
    TEST_DELAY__(50),
    TEST_EXPECT__________(""),
    TEST_PRESS______("n"),
    TEST_DELAY__(10),
    TEST_EXPECT__________("n"),
    TEST_RELEASE__U("n"),
    TEST_DELAY__(50),
    TEST_EXPECT__________(""),
    TEST_PRESS______("u"),
    TEST_DELAY__(10),
    TEST_EXPECT__________("u"),
    TEST_RELEASE__U("u"),
    TEST_DELAY__(50),
    TEST_EXPECT__________(""),
    TEST_PRESS______("p"),
    TEST_DELAY__(10),
    TEST_EXPECT__________("p"),
    TEST_RELEASE__U("p"),
    TEST_DELAY__(50),
    TEST_EXPECT__________(""),
    TEST_PRESS______("k"),
    TEST_DELAY__(10),
    TEST_EXPECT__________("k"),
    TEST_RELEASE__U("k"),
    TEST_DELAY__(50),
    TEST_EXPECT__________(""),
    TEST_PRESS______("m"),
    TEST_DELAY__(10),
    TEST_EXPECT__________("m"),
    TEST_RELEASE__U("m"),
    TEST_DELAY__(50),
    TEST_EXPECT__________(""),
    TEST_PRESS______("i"),
    TEST_DELAY__(10),
    TEST_EXPECT__________("i"),
    TEST_RELEASE__U("i"),
    TEST_DELAY__(50),
    TEST_EXPECT__________(""),
    TEST_PRESS______("h"),
    TEST_DELAY__(10),
    TEST_EXPECT__________("h"),
    TEST_RELEASE__U("h"),
    TEST_DELAY__(50),
    TEST_EXPECT__________(""),
    TEST_PRESS______("l"),
    TEST_DELAY__(10),
    TEST_EXPECT__________("l"),
    TEST_RELEASE__U("l"),
    TEST_DELAY__(50),
    TEST_EXPECT__________(""),
    TEST_PRESS______("y"),
    TEST_DELAY__(10),
    TEST_EXPECT__________("y"),
    TEST_RELEASE__U("y"),
    TEST_DELAY__(50),
    TEST_EXPECT__________(""),
    TEST_PRESS______("o"),
    TEST_DELAY__(10),
    TEST_EXPECT__________("o"),
    TEST_RELEASE__U("o"),
    TEST_DELAY__(50),
    TEST_EXPECT__________(""),
    TEST_PRESS______("j"),
    TEST_DELAY__(10),
    TEST_EXPECT__________("j"),
    TEST_RELEASE__U("j"),
    TEST_DELAY__(50),
    TEST_EXPECT__________(""),
    TEST_PRESS______("n"),
    TEST_DELAY__(10),
    TEST_EXPECT__________("n"),
    TEST_RELEASE__U("n"),
    TEST_DELAY__(50),
    TEST_EXPECT__________(""),
    TEST_SET_BOOL(&UnreliableTransportTestMode, false),
    TEST_END()
};

// The host stops polling with a key down, the key is released meanwhile, and the host polls
// again. The host gets the press late; it has to get the release as well.
static const test_action_t test_transport_host_stops_polling[] = {
    TEST_SET_ACTION("j", "j"),
    TEST_SET_BOOL(&Hid_TestSinkStalled, true),
    TEST_PRESS______("j"),

    // The press times out (32 ms) and is resent until given up (255 ms of backoff).
    TEST_DELAY__(400),
    TEST_RELEASE__U("j"),
    TEST_DELAY__(100),
    TEST_CALL(Hid_ResumeTestSink),
    TEST_EXPECT__________("j"),
    TEST_EXPECT__________(""),
    TEST_DELAY__(100),
    TEST_END()
};

// Fails the test if no keyboard report failed since the hook was armed: then nothing was raced.
static bool confirmedInFail(void) {
    bool fired = !UsbReportSender_TestConfirmsInFail;
    UsbReportSender_TestConfirmsInFail = false;
    if (!fired) {
        LOG_FAILURE("[TEST] FAIL: no failed keyboard report for the confirmation to race\n");
    }
    return fired;
}

// The host takes a report at the very moment its timeout is being handled. The report must not
// be sent again from the switched buffer, which holds the state before it (a key release).
static const test_action_t test_transport_confirmation_races_timeout[] = {
    TEST_SET_ACTION("j", "j"),
    TEST_SET_ACTION("k", "k"),
    TEST_SET_BOOL(&Hid_TestSinkStalled, true),
    TEST_SET_BOOL(&UsbReportSender_TestConfirmsInFail, true),
    TEST_PRESS______("j"),
    TEST_EXPECT__________("j"),

    // The press times out after 32 ms.
    TEST_DELAY__(100),
    TEST_CALL(confirmedInFail),
    TEST_PRESS______("k"),
    TEST_EXPECT__________("j k"),
    TEST_DELAY__(60),
    TEST_RELEASE__U("k"),
    TEST_EXPECT__________("j"),
    TEST_DELAY__(60),
    TEST_RELEASE__U("j"),
    TEST_EXPECT__________(""),
    TEST_DELAY__(60),
    TEST_END()
};

// The host takes a report that was given up on, at the very moment the next report fails to be
// sent. The confirmation advances the baseline to that next report, which the host never got.
static const test_action_t test_transport_confirmation_races_failed_send[] = {
    TEST_SET_ACTION("j", "j"),
    TEST_SET_ACTION("k", "k"),
    TEST_SET_BOOL(&Hid_TestSinkStalled, true),
    TEST_PRESS______("j"),

    // The press times out (32 ms) and is resent until given up (255 ms of backoff).
    TEST_DELAY__(400),
    TEST_SET_BOOL(&UsbReportSender_TestConfirmsInFail, true),
    TEST_PRESS______("k"),
    TEST_EXPECT__________("j"),
    TEST_EXPECT__________("j k"),
    TEST_DELAY__(100),
    TEST_CALL(confirmedInFail),
    TEST_RELEASE__U("k"),
    TEST_EXPECT__________("j"),
    TEST_DELAY__(60),
    TEST_RELEASE__U("j"),
    TEST_EXPECT__________(""),
    TEST_DELAY__(60),
    TEST_END()
};

// Fails the test if a mouse or controls report was attempted while their interfaces were down.
static bool downInterfacesLeftAlone(void) {
    report_send_state_t *controls = &UsbSemaphore.controls;
    report_send_state_t *mouse = &UsbSemaphore.mouse;
    bool leftAlone = controls->retries == 0 && !controls->inFlight && mouse->retries == 0 && !mouse->inFlight;
    if (!leftAlone) {
        LOG_FAILURE("[TEST] FAIL: report sent to a down interface (inFlight/retries): controls=%d/%d mouse=%d/%d\n",
            controls->inFlight, controls->retries, mouse->inFlight, mouse->retries);
    }
    return leftAlone;
}

// A host in boot mode starts the keyboard interface only, so the mouse and controls reports of
// the held keys can't be delivered. Typing has to arrive anyway, over the unreliable transport.
static const test_action_t test_transport_boot_mode_typing[] = {
    TEST_SET_BOOL(&Hid_TestSinkBootMode, true),
    TEST_SET_BOOL(&UnreliableTransportTestMode, true),
    TEST_SET_ACTION("u", "mediaVolumeUp"),
    TEST_SET_ACTION("i", "mouseBtnLeft"),
    TEST_SET_ACTION("h", "h"),
    TEST_SET_ACTION("j", "j"),
    TEST_SET_ACTION("k", "k"),
    TEST_SET_ACTION("l", "l"),
    TEST_SET_ACTION("n", "n"),
    TEST_PRESS______("u"),
    TEST_PRESS______("i"),
    TEST_DELAY__(60),
    TEST_PRESS______("l"),
    TEST_DELAY__(10),
    TEST_EXPECT__________("l"),
    TEST_RELEASE__U("l"),
    TEST_DELAY__(50),
    TEST_EXPECT__________(""),
    TEST_PRESS______("h"),
    TEST_DELAY__(10),
    TEST_EXPECT__________("h"),
    TEST_RELEASE__U("h"),
    TEST_DELAY__(50),
    TEST_EXPECT__________(""),
    TEST_PRESS______("k"),
    TEST_DELAY__(10),
    TEST_EXPECT__________("k"),
    TEST_RELEASE__U("k"),
    TEST_DELAY__(50),
    TEST_EXPECT__________(""),
    TEST_PRESS______("n"),
    TEST_DELAY__(10),
    TEST_EXPECT__________("n"),
    TEST_RELEASE__U("n"),
    TEST_DELAY__(50),
    TEST_EXPECT__________(""),
    TEST_PRESS______("j"),
    TEST_DELAY__(10),
    TEST_EXPECT__________("j"),
    TEST_RELEASE__U("j"),
    TEST_DELAY__(50),
    TEST_EXPECT__________(""),
    TEST_CALL(downInterfacesLeftAlone),
    TEST_RELEASE__U("u"),
    TEST_RELEASE__U("i"),
    TEST_DELAY__(60),
    TEST_SET_BOOL(&UnreliableTransportTestMode, false),
    TEST_SET_BOOL(&Hid_TestSinkBootMode, false),
    TEST_END()
};

// Stalls the mouse sink. What the reset at the test's start set off settles first (released keys
// of a failed predecessor, a state resend), so that the report the sink holds is this test's.
#define STALL_MOUSE_SINK TEST_DELAY__(100), TEST_SET_BOOL(&Hid_TestSinkMouseStalled, true)

static uint16_t capturedBeforeResend;

static bool requestStateResend(void) {
    EventScheduler_Schedule(Timer_GetCurrentTime(), EventSchedulerEvent_ResendUsbState, "test");
    return true;
}

// Requests a state resend. Fails the test if no mouse report is being retried: then there is no
// resend pass that could lose the request.
static bool requestStateResendDuringRetries(void) {
    uint8_t retries = UsbSemaphore.mouse.retries;
    bool retrying = retries > 0 && retries < THROTTLE_MAX_RETRIES;
    if (!retrying) {
        LOG_FAILURE("[TEST] FAIL: the mouse report isn't being retried (retries=%d)\n", retries);
    }
    capturedBeforeResend = TestHooks_CapturedReports;
    requestStateResend();
    return retrying;
}

// Fails the test if the keyboard state went out while the mouse report was still being retried:
// a normal pass has to wait for the reports of earlier passes, to keep the reports in sequence.
static bool notResentDuringRetries(void) {
    uint8_t retries = UsbSemaphore.mouse.retries;
    bool retrying = retries < THROTTLE_MAX_RETRIES;
    bool waited = TestHooks_CapturedReports == capturedBeforeResend;
    if (!retrying) {
        LOG_FAILURE("[TEST] FAIL: the mouse retries are over already (retries=%d)\n", retries);
    } else if (!waited) {
        LOG_FAILURE("[TEST] FAIL: a keyboard report went out while the mouse report was being retried\n");
    }
    return retrying && waited;
}

// Fails the test if the mouse retries didn't end in a give-up, or the host got no keyboard report
// since the state resend was requested.
static bool stateWasResent(void) {
    bool givenUp = UsbSemaphore.mouse.retries >= THROTTLE_MAX_RETRIES;
    bool resent = TestHooks_CapturedReports != capturedBeforeResend;
    if (!givenUp) {
        LOG_FAILURE("[TEST] FAIL: the mouse report wasn't given up (retries=%d)\n", UsbSemaphore.mouse.retries);
    } else if (!resent) {
        LOG_FAILURE("[TEST] FAIL: the requested state resend didn't reach the host\n");
    }
    return givenUp && resent;
}

// A state resend is requested while the report of another kind is being retried, and those
// retries end in a give-up. The resend passes must not lose the request: the host has to get the
// keyboard state without any further input. No key is held meanwhile: a held key asks for a pass
// on its own.
static const test_action_t test_transport_resend_request_survives_retries[] = {
    TEST_SET_ACTION("j", "j"),
    TEST_SET_ACTION("i", "mouseBtnLeft"),
    STALL_MOUSE_SINK,
    TEST_PRESS______("j"),
    TEST_EXPECT__________("j"),
    TEST_DELAY__(60),
    TEST_RELEASE__U("j"),
    TEST_EXPECT__________(""),
    TEST_DELAY__(60),

    // The mouse report is held, retried for about 290 ms and given up.
    TEST_PRESS______("i"),
    TEST_DELAY__(60),
    TEST_RELEASE__U("i"),
    TEST_DELAY__(400),

    // The first state resend starts the mouse retries again; the second comes in their middle.
    TEST_CALL(requestStateResend),
    TEST_DELAY__(100),
    TEST_CALL(requestStateResendDuringRetries),

    // The second one restarts the retries; they end 255 to 290 ms from here.
    TEST_DELAY__(150),
    TEST_CALL(notResentDuringRetries),
    TEST_DELAY__(350),
    TEST_CALL(stateWasResent),
    TEST_END()
};

static uint16_t mouseSendsBefore;

// Fails the test if the mouse wasn't given up in this test: the tests below need a host that
// doesn't take mouse reports and a sender that knows it.
static bool mouseIsGivenUp(void) {
    bool givenUp = UsbSemaphore.mouse.retries >= THROTTLE_MAX_RETRIES && Hid_TestSinkMouseSends > 0;
    if (!givenUp) {
        LOG_FAILURE("[TEST] FAIL: the mouse report wasn't given up (retries=%d, sends=%d)\n", UsbSemaphore.mouse.retries, Hid_TestSinkMouseSends);
    }
    mouseSendsBefore = Hid_TestSinkMouseSends;
    return givenUp;
}

static bool noMouseSendSince(void) {
    uint16_t sends = Hid_TestSinkMouseSends - mouseSendsBefore;
    if (sends != 0) {
        LOG_FAILURE("[TEST] FAIL: %d mouse reports sent to a host that doesn't take them\n", sends);
    }
    return sends == 0;
}

// A host that never polls the mouse: its report is given up and dropped. After that nothing is
// sent to the mouse interface, neither by the passes of other kinds nor by a timer.
static const test_action_t test_transport_given_up_report_is_dropped[] = {
    TEST_SET_ACTION("j", "j"),
    TEST_SET_ACTION("i", "mouseBtnLeft"),
    STALL_MOUSE_SINK,

    // The mouse report is held, retried for about 290 ms and given up. The mouse key stays down,
    // so the report keeps differing from what the host has.
    TEST_PRESS______("i"),
    TEST_DELAY__(400),
    TEST_CALL(mouseIsGivenUp),
    TEST_PRESS______("j"),
    TEST_EXPECT__________("j"),
    TEST_DELAY__(60),
    TEST_RELEASE__U("j"),
    TEST_EXPECT__________(""),

    // Longer than the 1024 ms after which a given-up report used to be sent again.
    TEST_DELAY__(1200),
    TEST_CALL(noMouseSendSince),
    TEST_RELEASE__U("i"),
    TEST_DELAY__(60),
    TEST_END()
};

static const test_t transport_tests[] = {
    {
        .name = "unreliable_typing",
        .actions = test_transport_unreliable_typing,
        .repeatCount = TRANSPORT_RUNS,
        .spuriousFailureRate = TRANSPORT_RUN_FAILURE_RATE,
    },
    {
        .name = "host_stops_polling",
        .actions = test_transport_host_stops_polling,
    },
    {
        .name = "confirmation_races_timeout",
        .actions = test_transport_confirmation_races_timeout,
    },
    {
        .name = "confirmation_races_failed_send",
        .actions = test_transport_confirmation_races_failed_send,
    },
    {
        .name = "resend_request_survives_retries",
        .actions = test_transport_resend_request_survives_retries,
    },
    {
        .name = "given_up_report_is_dropped",
        .actions = test_transport_given_up_report_is_dropped,
    },
    {
        .name = "boot_mode_typing",
        .actions = test_transport_boot_mode_typing,
        .repeatCount = TRANSPORT_RUNS,

        // 10 keyboard reports in a run.
        .spuriousFailureRate = TRANSPORT_RUN_FAILURE_RATE * 10 / TRANSPORT_REPORTS_PER_RUN,
    },
};

const test_module_t TestModule_Transport = {
    .name = "Transport",
    .tests = transport_tests,
    .testCount = sizeof(transport_tests) / sizeof(transport_tests[0])
};
