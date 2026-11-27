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

// Backoff sizing: 1 << 5 == 32 ms. THROTTLE_MAX_RETRIES is in the header.
#define THROTTLE_MAX_SHIFT 5
#define THROTTLE_MAX_DELAY_MS 32

bool Resending = false;
bool UsbReportSender_TestConfirmsInFail = false;
static uint32_t lastBasicReportTime = 0;
static uint32_t retryThrottleTime = 0;

// Reports sent while the bus is suspended are dropped, see dropIfBusDown in transport.cpp. Once the
// bus is awake, send the current state instead. Main thread only; other threads schedule
// EventSchedulerEvent_ResendUsbState.
void UsbReportSender_RequestStateResend(void) {
    if (DEVICE_IS_MASTER) {
        UsbSemaphore.keyboard.retries = 0;
        UsbSemaphore.controls.retries = 0;
        UsbSemaphore.mouse.retries = 0;

        uint8_t* keyboard = (uint8_t*)(ActiveKeyboardReport == &keyboardReports[0] ? &keyboardReports[1] : &keyboardReports[0]);
        uint8_t* controls = (uint8_t*)(ActiveControlsReport == &controlsReports[0] ? &controlsReports[1] : &controlsReports[0]);
        uint8_t* mouse = (uint8_t*)(ActiveMouseReport == &mouseReports[0] ? &mouseReports[1] : &mouseReports[0]);

        // A first byte no real report is likely to have: all modifiers, the first eight mouse
        // buttons. The baselines then differ from the current state, whatever it is by the next
        // pass. Not derived from the active reports: between a delivery and the next pass they are
        // stale. A state that does have this first byte isn't resent.
        keyboard[0] = 0xff;
        controls[0] = 0xff;
        mouse[0] = 0xff;

        EventVector_Set(EventVector_SendUsbReports);
    }
}

// No host takes the report and retrying won't change it; the current state is sent once a host is
// up, see UsbReportSender_RequestStateResend.
static bool isHostGone(int err) {
    return err == -ENETDOWN || err == -ECONNRESET || err == -ENOTCONN || err == -EPIPE || err == -EHOSTUNREACH;
}

// Should we resend, or give up?
bool UsbReportSender_ShouldGiveUp(int err, uint8_t* counter) {
    bool statusOk = err == 0;

    if (statusOk) {
        *counter = 0;
        return true;
    }

    (*counter)++;
    if (*counter > THROTTLE_MAX_RETRIES || isHostGone(err)) {
        // Stays exhausted until a send succeeds or a host is up, so that the following reports of
        // this kind get a single attempt each.
        *counter = THROTTLE_MAX_RETRIES;
        return true;
    }

    if (*counter == 1 && err != -EBUSY) {
        LOG_WRN("Send try failed, result: %d (%s). Will retry.", err, ErrToStr(err));
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
#ifdef __ZEPHYR__
    if (CurrentHostConnectionId == ConnectionId_Invalid) {
        WATCH(LOG_WRN("Send failed: no connection selected: %d (%s)", errorCode, ErrToStr(errorCode)));
    } else {
        WATCH(LOG_ERR("Send failed (gave up resending): %d (%s)", errorCode, ErrToStr(errorCode)));
        if (Timer_GetCurrentTime() - Bt_LastConnectedTime > 10*1000) {
            // If we are failing to resend a report and it has been at least 10 seconds since the connection was established, try to reconnect.
            if (!WormCfg->devMode && Connections_Type(CurrentHostConnectionId) != ConnectionType_UsbHidRight) {
                WATCH(LOG_ERR("Send failed. Trying to reconnect."));
                HostConnections_Reconnect();
            }
        }
    }
#else
    WATCH(LOG_ERR("Send failed: %d (%s)", errorCode, ErrToStr(errorCode)));
#endif
}

// The report was accepted but never confirmed: the link stopped taking data while staying up.
// Its queued reports hold shared BLE buffers until the link goes down, so take it down.
void UsbReportSender_TearDownStalledLink(report_send_state_t* st) {
#ifdef __ZEPHYR__
    connection_id_t connectionId = st->connectionId;
    WATCH(LOG_ERR("Report unconfirmed for %d ms, disconnecting %s.", USB_SEMAPHORE_STALL_TIMEOUT, GetPeerStringByConnId(connectionId)));
    if (connectionId != ConnectionId_Invalid && Connections_IsReady(connectionId)) {
        BtConn_DisconnectOne(connectionId);
    }
#endif
    st->retries = 0;
    st->needsResending = false;
    // The report wasn't released, so the current state goes out again once a link is up.
    EventScheduler_Schedule(Timer_GetCurrentTime() + USB_GIVEUP_RESEND_DELAY_MS, EventSchedulerEvent_SendUsbReports, "stalled-link");
}

static bool resendOrGiveUp(report_send_state_t* st, errno_t ret, bool withDelay) {
    if (!UsbReportSender_ShouldGiveUp(ret, &st->retries)) {
        st->needsResending = true;
        // The semaphore timeout path already waited out its own delay, so resend now.
        retryThrottleTime = withDelay ? Timer_GetCurrentTime() + UsbReportSender_ComputeResendDelay(st->retries) : Timer_GetCurrentTime();
        EventVector_Set(EventVector_ResendUsbReports);
        return true;
    } else {
        if (ret != 0) {
            // The report is dropped: the baseline advances as if the host had it. The host's state
            // is put right by the next report of this kind, or without one:
            // - host gone: by the state resend once a host is up.
            // - the transport still holds an earlier report (e.g. USB stopped polling the
            //   endpoint): by the state resend on its late confirmation, see reportSentCallback.
            // - neither (the link rejects reports while holding none): by nothing. A dropped key
            //   release then leaves the key pressed until the next key event.
            st->switchActiveReport();
            handleUltimateFail(ret);
        }
        st->needsResending = false;
        return false;
    }
}

// The report wasn't sent or delivered: resend it, or give up. Returns false if it was given up.
// withDelay: back off before the resend.
bool UsbReportSender_Fail(report_send_state_t* st, errno_t result, bool withDelay) {
    bool pending = true;

    // Test only: the test sink confirms its held report right here, the way a callback preempts
    // the main thread between a failure and its handling.
    if (UsbReportSender_TestConfirmsInFail && st == &UsbSemaphore.keyboard) {
        UsbReportSender_TestConfirmsInFail = false;
        Hid_ResumeTestSink();
    }

    if (UsbSemaphore_Release(st, SemaphoreRelease_Undelivered)) {
        // Not released: the baseline stays, so the same report goes out again, unless it is given up.
        pending = resendOrGiveUp(st, result, withDelay);
    } else if (DEVICE_IS_MASTER) {
        // Something released or cleared the report first (a confirmation, a bus reset), so the
        // active report is no longer the one that failed. A confirmation may have been that of
        // an older report though, so send the current state.
        LOG_WRN("Report resolved during a failure (%d)", result);
        EventScheduler_Schedule(Timer_GetCurrentTime(), EventSchedulerEvent_ResendUsbState, "confirmation raced a failure");
    } else {
        // The dongle relays one message at a time and has no state to send: send it again.
        LOG_WRN("Report resolved during a failure (%d)", result);
        st->needsResending = true;
    }
    return pending;
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

static void sendActiveReports(bool resending) {
    bool usbReportsChangedByAnything = false;
    errno_t ret;

    // In case of usb error, the resend request gets set back again. A resend pass leaves the
    // request for a normal pass alone: it doesn't do that pass's work.
    EventVector_Unset(resending ? EventVector_ResendUsbReports : EventVector_SendUsbReports | EventVector_ResendUsbReports);

    if (KeyboardReport_HasChange(keyboardReports) && (!resending || UsbSemaphore.keyboard.needsResending)) {
        bool givenUp = false;
        ret = 0;
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
                // The resend is happening. Left set, a resend pass of another kind would send
                // this kind's active report again, which after a release is the previous state.
                UsbSemaphore.keyboard.needsResending = false;

                // Hid_SendKeyboardReport marks the report in flight (UsbSemaphore_Take).
                ret = Hid_SendKeyboardReport(ActiveKeyboardReport);
                // On success the report is accepted by the transport, but NOT yet confirmed
                // delivered. We advance the baseline (switchActiveKeyboardReport) only once
                // delivery is confirmed: USB/BLE asynchronously via the sent callback in transport.cpp,
                // NUS/dongle synchronously inside Hid_SendKeyboardReport. If a USB/BLE confirmation
                // never arrives, the IsReadyForTransfers timeout re-arms the resend - the baseline
                // was not advanced, so HasChange stays true and we re-send the current state instead
                // of losing it (the stuck-key bug).
                if (ret != 0) {
                    givenUp = !UsbReportSender_Fail(&UsbSemaphore.keyboard, ret, true);
                }
            }
            usbReportsChangedByAnything |= !givenUp;
            lastBasicReportTime = Timer_GetCurrentTime();
            if (!resending && ret == 0) {
                UsbReportUpdater_LastActivityTime = Timer_GetCurrentTime();
            }
        }
    }

    // A report whose interface is down isn't sent. It goes out with the first pass after the interface starts.
    if (ControlsReport_HasChanges(controlsReports) && (!resending || UsbSemaphore.controls.needsResending) && Hid_IsControlsInterfaceUp()) {
        bool givenUp = false;
        UsbSemaphore.controls.needsResending = false;
        ret = Hid_SendControlsReport(ActiveControlsReport);
        if (ret != 0) {
            givenUp = !UsbReportSender_Fail(&UsbSemaphore.controls, ret, true);
        }
        if (!resending && ret == 0) {
            UsbReportUpdater_LastActivityTime = Timer_GetCurrentTime();
        }
        usbReportsChangedByAnything |= !givenUp;
    }

    if (MouseReport_HasChanges(mouseReports, ActiveMouseReport) && (!resending || UsbSemaphore.mouse.needsResending) && Hid_IsMouseInterfaceUp()) {
        bool givenUp = false;
        UsbSemaphore.mouse.needsResending = false;
        ret = Hid_SendMouseReport(ActiveMouseReport);
        if (ret != 0) {
            givenUp = !UsbReportSender_Fail(&UsbSemaphore.mouse, ret, true);
        }
        if (givenUp) {
            clearMouseMovement(); // Don't make cursor jump if we have connection issues.
        }

        Debug_RecordBleSendResult(ret);

        if (!resending && ret == 0) {
            UsbReportUpdater_LastActivityTime = Timer_GetCurrentTime();
            UsbReportUpdater_LastMouseActivityTime = Timer_GetCurrentTime();
        }
        usbReportsChangedByAnything |= !givenUp;
    }

    // If anything changed, trigger one more update to send zero reports. A report that was given up
    // doesn't count: the sends of its kind fail at once instead of waiting for the host, so its
    // passes would follow each other without a pause.
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
    uint8_t blockReasons = 0;

    // UsbReportSemaphore - make sure:
    // - we are not sending reports until last report is sent
    // - we resend if sent is not confirmed within timeout
    if (!UsbSemaphore_RecalculateIsReady()) {
        blockedUntil = MAX(blockedUntil, currentTime + USB_SEMAPHORE_TIMEOUT);
        blocked = true;
        blockReasons |= THROTTLE_REASON_SEMAPHORE;
    }

    // Configured delay - prevent firmware (macros and similar) from sending reports too fast -
    // some applications (esp. games) don't expect that.
    if (currentTime < lastBasicReportTime + Cfg.KeystrokeDelay) {
        blockedUntil = lastBasicReportTime + Cfg.KeystrokeDelay;
        blocked = true;
        blockReasons |= THROTTLE_REASON_KEY_DELAY;
    }

    // If we are retrying too agressively, we may clog some USB hubs, so add a throttle in that case as well.
    if (currentTime < retryThrottleTime) {
        blockedUntil = MAX(retryThrottleTime, blockedUntil);
        blocked = true;
        blockReasons |= THROTTLE_REASON_RETRY;
    }

    // To reduce mouse latency, don't construct report until we are close enough to transport window.
    if ((int32_t)(UsbReportWindowEstimate - currentTime) > USB_REPORT_WINDOW_LOOKAHEAD_MS) {
        uint32_t throttleUntil = UsbReportWindowEstimate - USB_REPORT_WINDOW_LOOKAHEAD_MS;
        blockedUntil = MAX(throttleUntil, blockedUntil);
        blocked = true;
        blockReasons |= THROTTLE_REASON_WINDOW;
    }

    MainLifeTimes.throttleTime = currentTime;
    MainLifeTimes.throttleBlocked = blocked;
    MainLifeTimes.throttleBlockReasons = blockReasons;
    MainLifeTimes.throttleBlockedUntil = blockedUntil;

    if (blocked) {
        DISABLE_IRQ();
        postponedMasks |= EventScheduler_Vector & EventVector_MainTriggers;
        EventScheduler_Vector = (EventScheduler_Vector & ~EventVector_MainTriggers) | EventVector_KeystrokeDelayPostponing;
        ENABLE_IRQ();

        MainLifeTimes.throttlePostponedMasks = postponedMasks;

        // Make sure to wake up postponer so that it can process the events.
        // Don't reschedule here - it is unsafe because of races.
        EventScheduler_Schedule(blockedUntil, EventSchedulerEvent_Postponer, "report throttle");
        return true;
    } else if (postponedMasks) {
        DISABLE_IRQ();
        EventScheduler_Vector = (EventScheduler_Vector & ~EventVector_KeystrokeDelayPostponing) | postponedMasks;
        postponedMasks = 0;
        ENABLE_IRQ();
        MainLifeTimes.throttlePostponedMasks = 0;
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
