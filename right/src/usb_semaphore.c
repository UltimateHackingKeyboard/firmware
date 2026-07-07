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

void UsbSemaphore_Release(report_send_state_t* st) {
    if (st == &UsbSemaphore.keyboard) {
        DEBUG_KEY_LIFE(LOG_INF("      '%s' delivered\n", Utils_GetUsbReportString(ActiveKeyboardReport)));
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
