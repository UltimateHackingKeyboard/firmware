#include <stdint.h>
#include <stdbool.h>
#include <errno.h>
#include "usb_semaphore.h"
#include "usb_report_sender.h"
#include "usb_report_updater.h"
#include "timer.h"
#include "power_mode.h"
#include "event_scheduler.h"

#ifndef __ZEPHYR__
#include "stubs.h"
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

// Release a report's in-flight latch once it has been successfully sent (USB/BLE via the
// async sent callback, NUS/dongle synchronously on successful enqueue). This unblocks report
// processing: advance the baseline to the just-sent report and reset the resend state
// (ShouldResendReport(0, ...) clears the retry counter and the give-up latch). This means
// "sent", not "delivered" - for USB/BLE the delivery confirmation is a separate, later event.
void UsbSemaphore_Release(report_send_state_t* st) {
    st->switchActiveReport();
    st->retries = 0;
    st->inFlight = false;
    UsbReportSender_GivenUp = false;
    EventScheduler_Schedule(Timer_GetCurrentTime(), EventSchedulerEvent_Postponer, "Usb semaphore released. Recalculate throttle delay.");
}

// Recompute the in-flight gate. While a report is still in flight within its confirmation
// grace window we are not ready (return false). Once the window elapses, re-arm the resend
// for the still-pending reports (their baseline was never advanced, so the current state is
// re-sent rather than lost) and report ready.
bool UsbSemaphore_RecalculateIsReady(void) {
    if (UsbSemaphore_AnyInFlight() && CurrentPowerMode <= PowerMode_LastAwake) {
        if (Timer_GetElapsedTime(&UpdateUsbReports_LastUpdateTime) < USB_SEMAPHORE_TIMEOUT) {
            return false;
        } else {
            if (UsbSemaphore.keyboard.inFlight) {
                UsbSemaphore.keyboard.inFlight = false;
                UsbReportSender_ResendOrGiveUp(&UsbSemaphore.keyboard, -ETIMEDOUT, false);
            }
            if (UsbSemaphore.controls.inFlight) {
                UsbSemaphore.controls.inFlight = false;
                UsbReportSender_ResendOrGiveUp(&UsbSemaphore.controls, -ETIMEDOUT, false);
            }
            if (UsbSemaphore.mouse.inFlight) {
                UsbSemaphore.mouse.inFlight = false;
                UsbReportSender_ResendOrGiveUp(&UsbSemaphore.mouse, -ETIMEDOUT, false);
            }
        }
    }
    return true;
}
