#include <stdint.h>
#include <stdbool.h>
#include "usb_report_sender.h"
#include "usb_report_updater.h"
#include "usb_semaphore.h"
#include "usb_scheduler.h"
#include "hid/transport.h"
#include "event_scheduler.h"
#include "timer.h"
#include "power_mode.h"
#include "config_manager.h"
#include "host_connection.h"
#include "macro_recorder.h"
#include "led_manager.h"
#include "debug.h"
#include "logger.h"
#include "macros/key_timing.h"
#include "atomicity.h"
#include "trace.h"
#include "utils.h"
#include "slave_drivers/uhk_module_driver.h"
#include <errno.h>

#ifdef __ZEPHYR__
#include "keyboard/input_interceptor.h"
#include "bt_conn.h"
#include "connections.h"
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(UsbReportSender, LOG_LEVEL_INF);
#else
#include "stubs.h"
#endif

// Backoff sizing. MAX_RETRIES is sized so that the cumulative wait from
// GetResendThrottleDelay() is ~1024 ms (1+2+4+8+16+32 + 30*32 = 1023).
#define THROTTLE_MAX_RETRIES 36
#define THROTTLE_MAX_SHIFT 5 // 1 << 5 == 32 ms
#define THROTTLE_MAX_DELAY_MS 32

bool Resending = false;
static uint32_t lastBasicReportTime = 0;
static uint32_t retryThrottleTime = 0;

bool UsbReportSender_GivenUp = false;

// Should we resend, or give up?
bool UsbReportSender_ShouldGiveUp(int err, uint8_t* counter) {
    bool statusOk = err == 0;

    if (statusOk) {
        *counter = 0;
        UsbReportSender_GivenUp = false;
        return true;
    }

    if (UsbReportSender_GivenUp) {
        return true;
    }

    (*counter)++;
    if (*counter > THROTTLE_MAX_RETRIES) {
        *counter = 0;
        UsbReportSender_GivenUp = true;
        return true;
    }

    if (*counter == 1 && err != -EBUSY) {
        LOG_WRN("Send try failed, result: %d (%s). Will retry.\n", err, ErrToStr(err));
    }

    return false;
}

// Exponential backoff for retry throttling: 1, 2, 4, 8, 16, 32 ms, then
// capped at 32 ms. Keeps initial retries snappy (mouse keys don't stutter on
// transient USB-busy) while still backing off on sustained failure.
uint16_t UsbReportSender_ComputeResendDelay(uint8_t counter) {
    if (counter == 0) {
        return 0;
    }
    uint8_t shift = counter - 1;
    if (shift > THROTTLE_MAX_SHIFT) {
        shift = THROTTLE_MAX_SHIFT;
    }
    return (uint16_t)1 << shift;
}

static void handleUltimateFail(errno_t errorCode) {
    // In any case, make the keyboard wake up, compare the usb reports, and possibly send them (those that are up-to-date at that time)
    // This way, we loose some reports along the way, but at least don't produce stuck keys
    EventScheduler_Schedule(Timer_GetCurrentTime() + USB_RESEND_DELAY_MS, EventSchedulerEvent_SendUsbReports, "usb-resend");

#ifdef __ZEPHYR__
    if (ActiveHostConnectionId == ConnectionId_Invalid) {
        LOG_WRN("Send failed: no connection selected: %d (%s)\n", errorCode, ErrToStr(errorCode));
    } else {
        LOG_ERR("Send failed (gave up resending): %d (%s)\n", errorCode, ErrToStr(errorCode));
        if (Timer_GetCurrentTime() - Bt_LastConnectedTime > 10*1000) {
            // If we are failing to resend a report and it has been at least 10 seconds since the connection was established, try to reconnect.
            if (!WormCfg->devMode) {
                LOG_ERR("Send failed. Trying to reconnect.\n");
                HostConnections_Reconnect();
            }
        }
    }
#else
    LOG_ERR("Send failed: %d (%s)\n", errorCode, ErrToStr(errorCode));
#endif
}

bool UsbReportSender_ResendOrGiveUp(report_send_state_t* st, errno_t ret, bool withDelay) {
    if (!UsbReportSender_ShouldGiveUp(ret, &st->retries)) {
        st->needsResending = true;
        // The semaphore timeout path already waited out its own delay, so resend now.
        retryThrottleTime = withDelay ? Timer_GetCurrentTime() + UsbReportSender_ComputeResendDelay(st->retries) : Timer_GetCurrentTime();
        EventVector_Set(EventVector_ResendUsbReports);
        return true;
    } else {
        if (ret != 0) {
            handleUltimateFail(ret);
        }
        st->needsResending = false;
        return false;
    }
}

static void clearMouseMovement(void) {
    ActiveMouseReport->x = 0;
    ActiveMouseReport->y = 0;
    ActiveMouseReport->wheelX = 0;
    ActiveMouseReport->wheelY = 0;

    for (uhk_module_state_t *moduleState = UhkModuleStates; moduleState < UhkModuleStates + UHK_MODULE_MAX_SLOT_COUNT; moduleState++) {
        moduleState->pointerDelta.x = 0;
        moduleState->pointerDelta.y = 0;
    }
}

static bool mouseButtonsChanged(void) {
    return mouseReports[0].buttons != mouseReports[1].buttons;
}

static void sendActiveReports(bool resending) {
    bool usbReportsChangedByAction = false;
    bool usbReportsChangedByAnything = false;
    errno_t ret;

    // in case of usb error, this gets set back again
    EventVector_Unset(EventVector_SendUsbReports | EventVector_ResendUsbReports);

    if (KeyboardReport_HasChange(keyboardReports) && (!resending || UsbSemaphore.keyboard.needsResending)) {
#ifdef __ZEPHYR__
        if (InputInterceptor_RegisterReport(ActiveKeyboardReport)) {
            UsbReportUpdater_SwitchActiveKeyboardReport();
        } else
#endif
        {
            MacroRecorder_RecordBasicReport(ActiveKeyboardReport);

            KEY_TIMING(KeyTiming_RecordReport(ActiveKeyboardReport));

            if (RuntimeMacroRecordingBlind) {
                //just switch reports without sending the report
                UsbReportUpdater_SwitchActiveKeyboardReport();
            } else {
                //The semaphore has to be set before the call. Assume what happens if a bus reset happens asynchronously here. (Deadlock.)
                UsbSemaphore.keyboard.inFlight = true;
                ret = Hid_SendKeyboardReport(ActiveKeyboardReport);
                // On success the report is accepted by the transport, but NOT yet confirmed
                // delivered. We advance the baseline (switchActiveKeyboardReport) only once
                // delivery is confirmed: USB/BLE asynchronously via Hid_KeyboardReportSentCallback,
                // NUS/dongle synchronously inside Hid_SendKeyboardReport. If a USB/BLE confirmation
                // never arrives, the IsReadyForTransfers timeout re-arms the resend - the baseline
                // was not advanced, so HasChange stays true and we re-send the current state instead
                // of losing it (the stuck-key bug).
                if (ret != 0) {
                    UsbSemaphore.keyboard.inFlight = false;
                    UsbReportSender_ResendOrGiveUp(&UsbSemaphore.keyboard, ret, true);
                }
            }
            usbReportsChangedByAction = true;
            usbReportsChangedByAnything = true;
            lastBasicReportTime = Timer_GetCurrentTime();
            UsbReportUpdater_LastActivityTime = resending ? UsbReportUpdater_LastActivityTime : Timer_GetCurrentTime();
        }
    }

    if (ControlsReport_HasChanges(controlsReports) && (!resending || UsbSemaphore.controls.needsResending)) {
        UsbSemaphore.controls.inFlight = true;
        ret = Hid_SendControlsReport(ActiveControlsReport);
        if (ret != 0) {
            UsbSemaphore.controls.inFlight = false;
            UsbReportSender_ResendOrGiveUp(&UsbSemaphore.controls, ret, true);
        }
        UsbReportUpdater_LastActivityTime = resending ? UsbReportUpdater_LastActivityTime : Timer_GetCurrentTime();
        usbReportsChangedByAction = true;
        usbReportsChangedByAnything = true;
    }

    if (MouseReport_HasChanges(mouseReports, ActiveMouseReport) && (!resending || UsbSemaphore.mouse.needsResending)) {
        bool usbMouseButtonsChanged = mouseButtonsChanged();

        UsbSemaphore.mouse.inFlight = true;
        ret = Hid_SendMouseReport(ActiveMouseReport);
        if (ret != 0) {
            UsbSemaphore.mouse.inFlight = false;
            if(!UsbReportSender_ResendOrGiveUp(&UsbSemaphore.mouse, ret, true)) {
                clearMouseMovement(); // Don't make cursor jump if we have connection issues.
            }
        }

        Debug_RecordBleSendResult(ret);

        UsbReportUpdater_LastActivityTime = resending ? UsbReportUpdater_LastActivityTime : Timer_GetCurrentTime();
        UsbReportUpdater_LastMouseActivityTime = resending ? UsbReportUpdater_LastMouseActivityTime : Timer_GetCurrentTime();
        usbReportsChangedByAction |= usbMouseButtonsChanged;
        usbReportsChangedByAnything = true;
    }

    // If anything changed, trigger one more update to send zero reports
    // TODO: consider doing this depending on change of ReportsUsed mask(s) and actual module scans
    if (usbReportsChangedByAnything) {
        EventVector_Set(EventVector_SendUsbReports);
    }

    if (UsbSemaphore_AnyInFlight()) {
        EventScheduler_Schedule(UpdateUsbReports_LastUpdateTime + USB_SEMAPHORE_TIMEOUT, EventSchedulerEvent_Postponer, "usb-semaphore-timeout");
    }
}

static bool blockedByReportThrottle() {
    static uint32_t postponedMasks = 0;
    uint32_t currentTime = Timer_GetCurrentTime();
    uint32_t blockedUntil = 0;
    bool blocked = false;

    // UsbReportSemaphore - make sure:
    // - we are not sending reports until last report is sent
    // - we resend if sent is not confirmed within timeout
    if (!UsbSemaphore_RecalculateIsReady()) {
        blockedUntil = MAX(blockedUntil, currentTime + USB_SEMAPHORE_TIMEOUT);
        blocked = true;
    }

    // Configured delay - prevent firmware (macros and similar) from sending reports too fast -
    // some applications (esp. games) don't expect that.
    if (currentTime < lastBasicReportTime + Cfg.KeystrokeDelay) {
        blockedUntil = lastBasicReportTime + Cfg.KeystrokeDelay;
        blocked = true;
    }

    // If we are retrying too agressively, we may clog some USB hubs, so add a throttle in that case as well.
    if (currentTime < retryThrottleTime) {
        blockedUntil = MAX(retryThrottleTime, blockedUntil);
        blocked = true;
    }

    // To reduce mouse latency, don't construct report until we are close enough to transport window.
    if ((int32_t)(UsbReportWindowEstimate - currentTime) > USB_REPORT_WINDOW_LOOKAHEAD_MS) {
        uint32_t throttleUntil = UsbReportWindowEstimate - USB_REPORT_WINDOW_LOOKAHEAD_MS;
        blockedUntil = MAX(throttleUntil, blockedUntil);
        blocked = true;
    }

    if (blocked) {
        DISABLE_IRQ();
        postponedMasks |= EventScheduler_Vector & EventVector_MainTriggers;
        EventScheduler_Vector = (EventScheduler_Vector & ~EventVector_MainTriggers) | EventVector_KeystrokeDelayPostponing;
        ENABLE_IRQ();

        // Make sure to wake up postponer so that it can process the events.
        // Don't reschedule here - it is unsafe because of races.
        EventScheduler_Schedule(blockedUntil, EventSchedulerEvent_Postponer, "report throttle");
        return true;
    } else if (postponedMasks) {
        DISABLE_IRQ();
        EventScheduler_Vector = (EventScheduler_Vector & ~EventVector_KeystrokeDelayPostponing) | postponedMasks;
        postponedMasks = 0;
        ENABLE_IRQ();
    }
    return false;
}

// All paths have to call either UsbReportUpdater_UpdateActiveReports or justPreprocessInput(true).
void UsbReportSender_UpdateAndSendUsbReports(void)
{
    Trace_Printc("u1");
    if (blockedByReportThrottle()) {
        justPreprocessInput(true);
        return;
    }

#if __ZEPHYR__ && (DEBUG_POSTPONER || DEBUG_EVENTLOOP_SCHEDULE)
    printk("========== new UpdateAndSendUsbReports cycle ==========\n");
#endif

    UpdateUsbReports_LastUpdateTime = Timer_GetCurrentTime();
    UsbReportUpdateCounter++;

    bool resending = EventVector_IsSet(EventVector_ResendUsbReports);
    Resending = resending;

    if (resending) {
        justPreprocessInput(true); // don't allow resending to block input processing
    } else {
        Trace_Printc("u2");
        UsbReportUpdater_UpdateActiveReports();
    }

    bool sendingNew = EventVector_IsSet(EventVector_SendUsbReports);

    if (resending || sendingNew) {
        if (CurrentPowerMode < PowerMode_Lock) {
            if (!resending) {
                Trace_Printc("u3");
                UsbReportUpdater_MergeReports();
            }

            Trace_Printc("u6");
            sendActiveReports(resending);
        } else {
            EventVector_Unset(EventVector_SendUsbReports | EventVector_ResendUsbReports);
        }
    }

    Trace_Printc("u7");

    if (DisplaySleepModeActive || KeyBacklightSleepModeActive) {
        LedManager_UpdateSleepModes();
    }

    Trace_Printc("u8");
}
