# UHK80 dual USB: implementation and development

The experimental firmware routes the complete keyboard to either half's USB-C
port. The right half runs keymaps, layers and macros; the left encodes and submits
relayed keyboard, mouse and consumer/system-control reports to its own computer.
Both USB command interfaces remain available while input selection changes.

This implements the [technical design](uhk80-dual-usb-hosts-design.md) behind
`CONFIG_UHK_USB_LEFT_RELAY`, which defaults to **off**. The experimental presets
enable it on both halves and enable a temporary host slot on the right. Standard
presets retain the existing feature selection. No Agent, descriptor, VID/PID,
configuration-version or bootloader change is included.

**Hardware acceptance is pending.** Cloud checks compile the actual firmware
and exercise the portable protocol and route coordinator; they cannot establish
USB cancellation timing, wakeup behavior, runtime stack headroom or two-computer
operation. The original cloud implementation did not flash a keyboard.

Initial local Windows testing on 2026-10-09 flashed both halves, established
UART relay negotiation, and exposed blocked switching and repeated keys. The
completion boundary had reported the Nordic driver's consumed IN buffer
(`data` advanced, `len` zero) as the sent report. This made successful delivery
look like failure to the relay adapter. The repository's second c2usb patch
preserves the submitted IN buffer and length; OUT callbacks retain received
lengths, and errors/cancellations still report zero transferred bytes. The
SDK-backed completion regression fails before that patch and passes after it.
Fresh firmware builds and signed-image verification pass with the patch. Both
corrected applications were flashed, and a control-path check activated left
USB and returned to right USB without leaving the route blocked. Relay
rejection/lease counters increased during that check; physical key-release
testing and the complete hardware acceptance matrix remain pending.

The user reported improved typing through right USB after that correction.
Left USB still expired its idle lease. Its poll event was popped after the main
loop's next-poll request had been ignored by the scheduler's earlier-deadline
rule, leaving no relay wakeup queued. The relay poll event handler now rearms
the next poll. An integration regression using that actual handler reproduces
lease expiry before the fix and maintains an idle session for five lease periods
after it. The completion, relay and coordinator tests and both signed builds
pass with both corrections. On hardware, corrected left USB stayed active for
20 seconds without vendor commands, with zero faults, retries or rejected
packets. The user reported that typing seemed to work through left USB, and
returning to right USB completed without faults. Two-computer input isolation,
held-key switching and the complete acceptance matrix remain pending.

A subsequent direct USB-C test on the right half produced repeating after
physical release; resetting that half made the same Mac/cable connection work.
The post-reset capture does not retain the failed transfer state. Code review
and an integration regression identified that a local USB-generation change
cleared the report baseline without requiring host-side all-up delivery. The
coordinator now holds input behind a drain and all-up completion barrier for an
awake, selected right USB session. The regression fails before this change and
passes after it. This correction is a candidate for the reported reconnect issue;
that Mac hot-plug scenario still needs a hardware retest.

A later Windows failure was captured before resetting: the physical/logical
keys were released, but the last completed right USB keyboard report still
asserted Backspace while both canonical comparison buffers were empty. A
filtered Windows Raw Input capture attributed down events without release to
the right USB interface. Successful normal keyboard completions could be
discarded after the semaphore timed out, or advance the baseline using a
working report already rebuilt after release. Normal USB/BLE submissions now
retain an immutable canonical keyboard snapshot; completion records that
delivered state even after timeout, leaving key-up distinct and requiring
delivery. An undrained keyboard completion prevents a new submission from
replacing its snapshot. Cancelled and stale route/session completions remain
excluded. The actual adapter queue/completion regression fails before this
correction and passes after it.

Both experimental release builds and signed-image checks passed. The corrected
right application was flashed, and the user confirmed key release from both
halves, including Backspace, after flashing and after a cable-only Windows USB
reconnect without reset. The existing next-connection key also successfully
selected the ready left USB host. These checks establish those scenarios;
direct Mac USB-C reconnect behavior, held-key switching and the remaining
acceptance matrix still need coverage.

## Behavior and limits

- Connect the inter-half bridge cable, Computer A to right USB-C and Computer B
  to left USB-C. Both halves must run these experimental images. The relay uses
  **UART only**; loss of the bridge does not send relay traffic over BLE.
- The optional development slot is `USB_Left_Dev`, allocated only in an empty
  serialized host slot. It never overwrites a paired/reserved slot. An existing
  unique USB Left slot is reused. Duplicate slots or a full table prevent
  provisioning. The development slot lives in RAM and is recreated after
  configuration reload; it is not saved to the user's configuration.
- On a switch involving USB Left, stop old user transfers, send neutral reports
  to the old host, prepare the new left session if needed, then commit the route.
  A suspended old USB host retains a neutral-report debt for resume after its
  in-flight transfers have been proven drained.
- Keys and modifiers physically held at the switch are suppressed by their
  source positions until release and a fresh press. This includes the physical
  key that invokes switching. Internal layer/keymap state is preserved. Macro
  output waits at the bounded transition barrier; later macro output is fresh
  input. Reports produced while the selected host is unavailable are discarded.
- Report retries retain the exact bytes and sequence number. The left acknowledges
  delivery after a successful USB completion, not UART acceptance. Duplicate
  relative mouse reports are acknowledged without submitting the movement again.
- A missing drain/cancellation proof blocks the transition. Elapsed time does
  not grant permission to reuse transfer storage or open another destination.
  Restore the bridge/host and inspect status; `retry` restarts negotiation but
  does not bypass this protection.
- Local USB submissions and cancellation run on the c2usb worker. Queued
  reservations remain owned through submission. Lifecycle fences cancel old
  transfers and drain their callbacks before allowing buffer reuse. Relay
  transmission uses a bounded four-packet UART worker queue.

Timing constants are provisional: report retry 32 ms, report deadline 128 ms,
heartbeat 200 ms, left lease 700 ms, state age limit 1,000 ms, and a 100 ms
macro/preparation barrier. A failed drain can outlast that barrier while scans
and switch-key processing continue. Measure healthy switching and report latency
on hardware before treating the PRD's performance targets as met.

## Build the experimental images

For a fresh Windows/WSL workspace, follow
[the setup instructions](uhk80-windows-wsl-setup.md) first. In this cloud
workspace, source `/workspace/.uhk-env/activate.sh` and enter the firmware repo.
For a local workspace, source its `activate-uhk80.sh` instead.

From the firmware repository root in WSL:

```bash
west build --sysbuild --build-dir device/build/uhk-80-right-usb-left device -- --preset uhk-80-right-usb-left
west build --sysbuild --build-dir device/build/uhk-80-left-usb-left device -- --preset uhk-80-left-usb-left
```

For logging/debug builds, use separate directories:

```bash
west build --sysbuild --build-dir device/build/uhk-80-right-usb-left-debug device -- --preset uhk-80-right-usb-left-debug
west build --sysbuild --build-dir device/build/uhk-80-left-usb-left-debug device -- --preset uhk-80-left-usb-left-debug
```

Incremental builds do not need another SDK update or patch operation:

```bash
west build --build-dir device/build/uhk-80-right-usb-left
west build --build-dir device/build/uhk-80-left-usb-left
```

Each directory contains:

| File | Use |
| --- | --- |
| `device/zephyr/zephyr.signed.bin` | Signed application image for USB updating that half. |
| `device/zephyr/zephyr.elf` | Symbols for that exact image; retain it for crash analysis. |
| `merged.hex` | Full programming image including MCUboot, for probe workflows. Do not pass it to the USB application updater. |

Verify the application signature against this workspace's build key:

```bash
python ../bootloader/mcuboot/scripts/imgtool.py verify \
  -k ../bootloader/mcuboot/root-ec-p256.pem \
  device/build/uhk-80-right-usb-left/device/zephyr/zephyr.signed.bin
```

Repeat for the left image. This checks the generated image and build key; it
does not establish which key your installed bootloader trusts. Use the existing
bootloader and matching application image for your unit.

## USB flashing from native Windows

Use native Windows tools for USB, with a separate Agent checkout; WSL's
`node_modules` cannot be reused because HID/serial dependencies contain native
code. The firmware pins Agent commit
`a10a251fc812df5011f706b0d150b12526da9b83`, whose engines require Node
`>=24.16.0 <25` and npm `>=11.13.0 <12`. In PowerShell:

```powershell
New-Item -ItemType Directory -Force C:\uhk-dev | Out-Null
Set-Location C:\uhk-dev
git clone https://github.com/UltimateHackingKeyboard/agent.git agent
Set-Location agent
git checkout a10a251fc812df5011f706b0d150b12526da9b83
node --version
npm.cmd --version
npm.cmd ci
npm.cmd run build
```

Export the experimental applications from WSL:

```bash
cp device/build/uhk-80-left-usb-left/device/zephyr/zephyr.signed.bin /mnt/c/uhk-dev/uhk-80-left.bin
cp device/build/uhk-80-right-usb-left/device/zephyr/zephyr.signed.bin /mnt/c/uhk-dev/uhk-80-right.bin
```

Save your configuration and keep a working firmware release and another
keyboard available for the first hardware test. This checkout's MCUboot build
uses a single application slot; normal USB updating depends on a working
application command interface. Confirm your unit's recovery procedure first.
Close desktop Agent, connect the target half directly to Windows, and run the
matching command from `C:\uhk-dev\agent`:

```powershell
.\node_modules\.bin\tsx.cmd .\packages\usb\update-device-firmware.ts --vid=14248 --pid=7 C:\uhk-dev\uhk-80-left.bin
.\node_modules\.bin\tsx.cmd .\packages\usb\update-device-firmware.ts --vid=14248 --pid=9 C:\uhk-dev\uhk-80-right.bin
```

These commands flash hardware. VID is `0x37A8`, left application PID `0x0007`,
right PID `0x0009`. The updater requests MCUboot, uploads over its serial USB
interface and resets the device. Disconnect other matching UHKs or use the
updater's `--serial-number` selector. Native Windows installation, flashing and
physical bootloader recovery have not been tested in this cloud environment.

## Test without changing Agent

No configuration serialization is required. With both experimental images,
the bridge connected and right USB attached to the Windows machine running
these commands, use the existing execute-macro interface:

```powershell
.\node_modules\.bin\tsx.cmd .\packages\usb\exec-macro-command.ts --vid=14248 --pid=9 'zephyr uhk usbLeft status'
.\node_modules\.bin\tsx.cmd .\packages\usb\exec-macro-command.ts --vid=14248 --pid=9 'zephyr uhk usbLeft select left'
.\node_modules\.bin\tsx.cmd .\packages\usb\exec-macro-command.ts --vid=14248 --pid=9 'zephyr uhk usbLeft select right'
```

The commands keep working through right USB even when keyboard input goes to
the left computer. Development controls are asynchronous: read status again
after a request to see the resulting state. `zephyr uhk usbLeft provision`
requests RAM provisioning; `zephyr uhk usbLeft retry` restarts negotiation.
When the newly allocated slot has its default name, this existing macro also
selects it:

```powershell
.\node_modules\.bin\tsx.cmd .\packages\usb\exec-macro-command.ts --vid=14248 --pid=9 'switchHost USB_Left_Dev'
```

An existing configured USB Left slot keeps its name. `switchHost nextActive`
and other existing selection commands can select the ready RAM slot. For a
physical switching-key test, use a compatible existing Agent to assign the
macro, then confirm the RAM slot was recreated after configuration application.
The development commands bypass the need for Agent UI support for adding a
left USB slot.

The pinned Agent declares configuration version 14/protocol 4.17 while this
firmware declares 15/4.20. The existing low-level update/execute-macro paths do
not serialize a configuration. Do not infer that the pinned GUI can safely
rewrite a version-15 configuration. Agent configuration/UI compatibility is a
separate rollout task from this firmware experiment.

With compatible Agent logging or its Zephyr shell, inspect:

```text
uhk usbLeft status
uhk connections
uhk uartStats
uhk threads
uhk log usbSink 1
```

Status includes negotiation, configuration, activity, quiescence, pending reports,
USB generation, rejected/duplicate/retry/fault counters, numeric fault reason,
current/requested host and input blocking. `provisionResult` is the connection
ID on success, `-ENOSPC` for a full table or `-EEXIST` for duplicate left slots.
Fault reason values are defined in `device/src/usb_left_relay_protocol.h`.

For crash symbols without a debug probe:

```bash
"$ZEPHYR_SDK_INSTALL_DIR/arm-zephyr-eabi/bin/arm-zephyr-eabi-addr2line" \
  -e device/build/uhk-80-right-usb-left-debug/device/zephyr/zephyr.elf -f -C 0xADDRESS
```

Use the ELF from the image actually flashed. USB logs and offline symbols work
without a probe; live breakpoints and single stepping generally require SWD.

## Checks and hardware acceptance

Run the host tests under WSL:

```bash
bash tests/usb_left_relay/run.sh
bash tests/usb_left_relay/run_c2usb_completion.sh
bash tests/usb_left_relay/run_relay_poll.sh
bash tests/usb_left_relay/run_normal_keyboard_completion.sh
```

See [test coverage and boundaries](../../tests/usb_left_relay/README.md).
Release and debug experimental builds for both halves, standard release builds
for both halves and the dongle, and signed-image verification were exercised
in the cloud. Cloud linker sizes before the local corrections were:

| Target | Flash / 900,608 bytes | RAM / 262,143 bytes |
| --- | --- | --- |
| Right release | 855,188 (94.96%) | 242,848 (92.64%) |
| Left release | 654,568 (72.68%) | 202,072 (77.08%) |
| Right debug | 870,976 (96.71%) | 242,720 (92.59%) |
| Left debug | 668,808 (74.26%) | 202,136 (77.11%) |

These report static allocations, not runtime stack/heap headroom. The right
half's flash budget leaves little room for further debug instrumentation.
Hardware acceptance still needs:

1. Verify two-computer enumeration, full keyboard/mouse/controls input, inactive
   host isolation and vendor commands on both ports.
2. Hold a key/modifier/mouse button while switching with a separate key. Verify
   old-host all-up and new-host silence until each held source is released and
   pressed again; cover both halves and modules.
3. Verify active-host LEDs, boot/6KRO/NKRO keyboard protocols, scroll resolution,
   Windows scroll behavior and macro continuations.
4. Disconnect/reconnect either USB cable or bridge during transfers and switches;
   reboot either half; verify no replay, duplicate relative movement or false
   completion. A blocked drain must remain visibly blocked.
5. Test sleep/resume and permitted/denied remote wakeup, including a suspended
   old host that resumes after switching away.
6. Stress UART loss/retries, endpoint cancellation, USB reconfiguration and
   configuration reload. Measure latency and queue/stack headroom with logs both
   enabled and disabled. Do not enable the feature by default before these pass.
