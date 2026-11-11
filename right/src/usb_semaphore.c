#include <stdint.h>
#include <stdbool.h>
#include <errno.h>
#include "usb_semaphore.h"
#include "usb_report_sender.h"
#include "usb_report_updater.h"
#include "timer.h"
#include "power_mode.h"
#include "event_scheduler.h"
#include "logger.h"
#include "utils.h"

#ifndef __ZEPHYR__
#include "stubs.h"
#endif

#ifdef __ZEPHYR__
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(UsbSemaphore, LOG_LEVEL_INF);
#endif

report_send_states_t UsbSemaphore = {
    .keyboard = { .switchActiveReport = UsbReportUpdater_SwitchActiveKeyboardReport },
    .controls = { .switchActiveReport = UsbReportUpdater_SwitchActiveControlsReport },
    .mouse = { .switchActiveReport = UsbReportUpdater_SwitchActiveMouseReport },
};

bool UsbSemaphore_AnyInFlight(void) {
    return UsbSemaphore.keyboard.inFlight || UsbSemaphore.controls.inFlight || UsbSemaphore.mouse.inFlight;
}

// Reassemble / apply the legacy uint8 bitfield view of the in-flight flags.
uint8_t UsbSemaphore_Get(void) {
    uint8_t bits = 0;
    bits |= UsbSemaphore.keyboard.inFlight ? UsbReportUpdate_Keyboard : 0;
    bits |= UsbSemaphore.controls.inFlight ? UsbReportUpdate_Controls : 0;
    bits |= UsbSemaphore.mouse.inFlight ? UsbReportUpdate_Mouse : 0;
    return bits;
}

void UsbSemaphore_Set(uint8_t bits) {
    UsbSemaphore.keyboard.inFlight = bits & UsbReportUpdate_Keyboard;
    UsbSemaphore.controls.inFlight = bits & UsbReportUpdate_Controls;
    UsbSemaphore.mouse.inFlight = bits & UsbReportUpdate_Mouse;
}

void UsbSemaphore_Clear(void) {
    UsbSemaphore.keyboard.inFlight = false;
    UsbSemaphore.controls.inFlight = false;
    UsbSemaphore.mouse.inFlight = false;
}

static bool resendsOnTimeout(const report_send_state_t* st) {
    return st->sink != ReportSink_BleHid;
}

static void clearIf(report_send_state_t* st, bool clear) {
    if (clear) {
        st->inFlight = false;
    }
}

void UsbSemaphore_ClearSink(report_sink_t sink) {
    clearIf(&UsbSemaphore.keyboard, UsbSemaphore.keyboard.sink == sink);
    clearIf(&UsbSemaphore.controls, UsbSemaphore.controls.sink == sink);
    clearIf(&UsbSemaphore.mouse, UsbSemaphore.mouse.sink == sink);
}

void UsbSemaphore_ClearResendable(void) {
    clearIf(&UsbSemaphore.keyboard, resendsOnTimeout(&UsbSemaphore.keyboard));
    clearIf(&UsbSemaphore.controls, resendsOnTimeout(&UsbSemaphore.controls));
    clearIf(&UsbSemaphore.mouse, resendsOnTimeout(&UsbSemaphore.mouse));
}

void UsbSemaphore_NoteSendTarget(report_send_state_t* st, report_sink_t sink, uint8_t connectionId) {
    st->sink = sink;
    st->connectionId = connectionId;
    st->sentTime = Timer_GetCurrentTime();
}

static bool clearIfSentTo(report_send_state_t* st, uint8_t connectionId) {
    bool clear = st->inFlight && !resendsOnTimeout(st) && st->connectionId == connectionId;
    if (clear) {
        st->inFlight = false;
    }
    return clear;
}

void UsbSemaphore_ClearConnection(uint8_t connectionId) {
    bool cleared = clearIfSentTo(&UsbSemaphore.keyboard, connectionId);
    cleared |= clearIfSentTo(&UsbSemaphore.controls, connectionId);
    cleared |= clearIfSentTo(&UsbSemaphore.mouse, connectionId);
    if (cleared) {
        // The unconfirmed reports were not released, so the current state goes out again.
        EventVector_Set(EventVector_SendUsbReports);
        EventVector_WakeMain();
    }
}

void UsbSemaphore_Release(report_send_state_t* st) {
    if (st == &UsbSemaphore.keyboard) {
        DEBUG_KEY_LIFE(delivered);
    }

    st->switchActiveReport();
    st->retries = 0;
    st->inFlight = false;
    UsbReportSender_GivenUp = false;
    if (DEVICE_IS_MASTER) {
        EventScheduler_Schedule(Timer_GetCurrentTime(), EventSchedulerEvent_Postponer, "Usb semaphore released. Recalculate throttle delay.");
    }
}

// NOTE: if we retry too soon, we might get a double report confirmation, confirming this report and the next one, which would make us loose the next one if its transport failes. Low probability in practice.
// Returns true while the report still blocks sending.
static bool recalculateReport(report_send_state_t* st) {
    bool blocks = false;
    if (st->inFlight) {
        if (resendsOnTimeout(st)) {
            st->inFlight = false;
            UsbReportSender_ResendOrGiveUp(st, -ETIMEDOUT, false);
        } else if (Timer_GetElapsedTime(&st->sentTime) >= USB_SEMAPHORE_STALL_TIMEOUT) {
            st->inFlight = false;
            UsbReportSender_TearDownStalledLink(st);
        } else {
            blocks = true;
        }
    }
    return blocks;
}

bool UsbSemaphore_RecalculateIsReady(void) {
    bool ready = true;
    if (UsbSemaphore_AnyInFlight() && CurrentPowerMode <= PowerMode_LastAwake) {
        if (Timer_GetElapsedTime(&UpdateUsbReports_LastUpdateTime) < USB_SEMAPHORE_TIMEOUT) {
            ready = false;
        } else {
            bool keyboardBlocks = recalculateReport(&UsbSemaphore.keyboard);
            bool controlsBlocks = recalculateReport(&UsbSemaphore.controls);
            bool mouseBlocks = recalculateReport(&UsbSemaphore.mouse);
            ready = !keyboardBlocks && !controlsBlocks && !mouseBlocks;
        }
    }
    return ready;
}
