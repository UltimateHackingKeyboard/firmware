# Interpreting UHK80 USB diagnostics

This guide explains the `c2usb diag`, `hid state`, and `Trace` sections printed by the UHK80 USB recovery diagnostic. The existing [USB log guide](logs_c2usb.md) explains how to enable and collect Zephyr logs; it does not interpret these diagnostic sections.

The recovery diagnostic prints c2usb state, HID transport state, then the trace buffer. The `zephyr uhk usbDiag` shell command prints only c2usb state. These reports describe a snapshot, not a continuous capture, so run the recovery diagnostic as soon as the problem is visible and preserve the complete output.

## c2usb diag

- `t=... ms` is firmware uptime in milliseconds.
- `power` is the USB bus power state: `L0-on`, `L1-sleep`, `L2-suspend`, or `L3-off`.
- `configured=1` means the host selected a valid USB configuration. `active=1` means the USB MAC is started. It can be active but not configured while enumeration has not completed.
- `evt queue free` is the current free space in the UDC event queue; `min` is the lowest free-space value seen since boot. Low free space can indicate event pressure. A dropped-event counter, if present, is stronger evidence that events were lost.
- `last ok transfer` is the most recent successful data-endpoint completion: endpoint address, age, and successful-transfer count. It is not proof that the host application consumed the report. `no successful data EP transfer yet` means none has completed since diagnostics were initialized/reset.

Anomaly lines are omitted when their `total` is zero. For each printed category, `total` counts occurrences since boot/reset, `sinceOk` counts occurrences since the last successful data-endpoint transfer, and `last` identifies the most recent endpoint, raw detail value, and age. These counters are global across endpoints: a successful transfer on any endpoint resets every category's `sinceOk` count. Thus a recent success on EP 83 does not prove that keyboard endpoint EP 81 succeeded.

- `enqueue fail`: the controller rejected a transfer when firmware queued it.
- `EP error`: a data endpoint completed with an error or was cancelled.
- `ctrl error`: an error on the control endpoint.
- `ctrl stall`: firmware stalled the control pipeline. `err=0` can mean the stall was requested without a specific error code; it does not by itself mean the controller reported error zero.
- `evt dropped`: a controller event could not be added to the worker queue. Its `err` value is the UDC event type, not an errno.
- `EP busy reject`: firmware refused a second transfer while that endpoint was busy. Its `err` value is not an errno. A queued transfer keeps the endpoint busy until its completion event is processed or it is explicitly cancelled; under suspend, the host may not poll it to completion. Repeated rejects after resume are more concerning than rejects confined to a suspended interval.
- `no-power reject`: firmware refused an IN transfer because the bus was not in `L0-on`. On the UHK80 port, `err` is the power-state enum value, not an errno. In these reports it appears alongside `power=L2-suspend`.

For other categories, `err` is a raw value from the call site and may be an errno-style result. Do not interpret every `err` as an errno without checking the relevant port code. Endpoint addresses are hexadecimal USB addresses; bit 7 indicates IN, and the low four bits are the endpoint number. For example, `EP 81` is endpoint 1 IN.

### Error codes on the UHK80 USB path

These names describe negative errno results (`-MACRO`). They apply to error-return fields such as the `z12` trace value, not to every `err=` field in `c2usb diag`. The active UHK80 build uses Newlib; in that build `ENETDOWN` is 115, `ECONNRESET` is 104, and `EINPROGRESS` is 119.

| Code | Meaning and source |
| --- | --- |
| `-ENETDOWN` | Network/transport down. The c2usb UHK80 MAC returns `result::network_down` for an IN transfer attempted outside USB `L0-on`. This is the source of `z12`'s `-ENETDOWN` in reports 2 and 3; it is returned before calling the Nordic UDC enqueue function. |
| `-ECONNRESET` | Connection reset. The HID send path starts with this result when the USB sink has no open USB session; a detached c2usb HID transport can also return it. The generic UDC layer uses it to complete obsolete control transfers after a new SETUP packet or host timeout. |
| `-EAGAIN` | Try again. `udc_nrf` returns this when the Nordic peripheral rejects a remote-wakeup request. |
| `-EIO` | I/O error. `udc_nrf` maps failure to enable or disable a control endpoint to this code. |
| `-EPERM` | Operation not permitted. The generic UDC wrapper returns this when enqueue is attempted while the controller is disabled. |
| `-ENODEV` | No such device. The generic UDC wrapper returns this when the requested endpoint is missing or not enabled. |
| `-ECONNABORTED` | Connection aborted. The generic UDC layer completes queued transfers with this status when an endpoint queue is cancelled/dequeued. |
| `-ENOMSG` | No message of the requested type. In the UHK integration, this can be propagated from the event callback if a control SETUP event is delivered inline while the event queue is full; it is not the normal `udc_nrf` endpoint-enqueue result. |

In the checked-out `udc_nrf` driver, the endpoint enqueue callback itself queues the buffer and returns `0`. The generic UDC wrapper performs enabled/endpoint checks before calling it. The driver also propagates errors from HFXO clock-manager requests/releases and endpoint registration during initialization; those depend on the clock-manager/registration result rather than a fixed USB transfer status. `-EINPROGRESS` is not emitted by the Nordic endpoint-enqueue path, and it is not the interpretation of report 2's `-ENETDOWN`.

`bus event` lines show a ring of up to 16 recent event runs. Only adjacent events of the same type are combined into `xN`; these are not lifetime totals. The age is measured from the last event in that run. Entries are printed oldest-to-newest in the order the MAC processes them, not timestamped directly at the physical bus edge. In the nRF UDC, suspend and resume are pending event bits; if both are pending in one handler pass, the driver processes suspend before resume, and repeated occurrences of the same bit can coalesce. Treat the log as approximate driver-observed order, not a lossless USB wire trace. The MAC also records a bus event before flushing late events received after VBUS removal, so a reset/resume line after `vbus-off` may be logged but ignored for state changes.

## hid state

- `sessions` counts whether the keyboard, mouse, controls, and command USB interfaces currently have sessions open. Zeroes commonly accompany an unconfigured device.
- `powerMode` is the firmware's own power mode, separate from USB `power`: `0` Awake, `1` Powersaving, `2` LightSleep, `3` Lock, `4` DeepSleep.
- `usbUp` reports whether the USB transport is up; `usbSuspended` reports the host-suspended flag.
- `key life` gives the age of the last observed key-pipeline activity at each stage. `scan(N)` is the last scanned switch state (`1` press, `0` release); `queued` and `forceQueued` are key changes sent through the postponer, `applied` is an immediately applied key change, `action` is when the key action was processed, and `delivered` is when a keyboard report was acknowledged by the transport. These are last-seen timestamps, not durations or counts. A large age can simply mean no key activity occurred.
- `inactive kb report` is the keyboard report buffer not currently active for sending; an empty report is a valid all-keys-released state.
- Each `semaphore` triplet is `inFlight/retries/needsResend` for keyboard, controls, or mouse reports. `givenUp=1` means the sender exhausted its retry budget for a failed report. It schedules a later report attempt; it does not permanently disable sending.
- `throttle` shows the age of the last report-throttle check, whether it blocked processing, its reason bitmask, the time remaining until unblock, and postponed event bits. Reason bits are `0x1` semaphore, `0x2` configured key delay, `0x4` retry backoff, and `0x8` USB report-window timing. A negative `until` value means that deadline is already in the past.

## Trace

The trace is a persistent 256-character circular buffer. It is printed oldest-to-newest from the current write position; older characters have already been overwritten. There are no timestamps or separators between most markers, so decode the strings using source from the exact firmware tag shown in `Tag`, not just the current branch. `###` marks trace initialization when it remains in the buffer.

- `ID` identifies the device; `Tag` identifies the firmware build associated with the trace.
- `EV` is the decimal event-vector snapshot last recorded by a trace write. It is not a history of pending events. For example, `EV: 512` is bit 9 (`EventVector_EventScheduler`) in the current `event_scheduler.h`.
- `Uptime` is firmware uptime. `Last main` is how long ago the UHK80 main loop last updated its heartbeat. The main loop sleeps while idle, so a large age alone is not evidence of a hang; interpret it with pending work, thread state, and other activity.
- `<Fnumber>` marks event-scheduler processing. In the current enum, `<F9>` is `NativeActions`, `<F30>` is `SendUsbReports`, and `<F33>` is `RollTargetEllipsis`. Numeric IDs can change between firmware versions.
- `<Unumber>` marks a USB protocol command. `<U9>` is `GetDeviceState` in the current command enum.
- `d1`/`d2` bracket message-queue processing in the UHK80-right main loop; `d3` is after user-logic processing; `d4` is after scheduling; `d5` is after the spinning-loop check; `d6`/`d7` bracket event-scheduler processing. `(` and `)` mark user-logic/loop scheduling boundaries.
- `l6` enters USB report updating. `u1` starts a report-update cycle; `u2` updates reports; `u3` merges reports; `u6` submits reports; `u7` and `u8` mark later completion steps. A cycle can skip some markers depending on throttle state, resend mode, or whether reports changed.
- `x1`/`x2` mark a key-state commit; `x3`/`x4` mark debounce scheduling. `w0`-`w6` and `v1`-`v5` mark action-state, macro, layer, and mouse processing stages in `usb_report_updater.c`.
- `z11`/`z12`, `z21`/`z22`, and `z31`/`z32` bracket keyboard, mouse, and controls report sends. The first number is the selected sink enum; currently `1` is USB, `2` BLE HID, and `3` dongle. The second value is the transport return code (`0` is success; negative values are errors). `y1.N` records a remote-wakeup attempt with firmware power mode `N`; `y4` follows the call.

### Reading the attached reports

- Reports 2, 3, and 7 show `L2-suspend`, alternating resume/suspend events, `no-power reject` on `EP 81`, and `givenUp=1`. Together these say the firmware observed bus suspend/resume transitions, rejected IN sends while suspended, and exhausted a report's retry budget. They do not show why the host/controller is toggling the bus state.
- Reports 4 and 5 show `configured=0`, no USB sessions, and no completed data-endpoint transfer. At the time of those snapshots, the host had not configured the USB device; the key pipeline alone cannot establish whether enumeration or a later report send failed.
- Reports 1 and 6 were captured with an active/configured device and a recent successful transfer on EP 83. Since success counters are global, this does not establish that keyboard EP 81 was working at dump time; both captures may have missed the transient failure.
- In reports 2 and 3, `z11,1z12,-ENETDOWN` means the keyboard report selected the USB sink and c2usb rejected the IN send because its bus power state was not `L0-on`. The trace alone does not explain why the bus was suspended.

When escalating, include the complete dump, firmware version and trace `Tag`, host model/OS, USB-C hub or adapter, other devices connected at the time, and how long the issue had been present before the dump. The `Tag` in these supplied reports is `v18.0.3` even though the introductory firmware version says `v18.0.4`; use the trace tag when matching trace markers to source.