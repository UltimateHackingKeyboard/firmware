# USB Left relay host tests

Run from the repository root on Linux/WSL with Python 3, a C compiler and sanitizer runtime:

```bash
bash tests/usb_left_relay/run.sh
```

The script compiles temporary C11 executables with warnings as errors,
AddressSanitizer and UndefinedBehaviorSanitizer, runs them and removes them.
It does not require Zephyr or a keyboard.

`test_relay.c` exercises the real protocol codecs and both relay roles through
deterministic fake UART/USB endpoints: malformed/truncated messages and codec
fuzzing; lost reports/acks; immutable retries; deduplication of relative input;
stale tokens, sequences, USB generations and state revisions; neutralization;
leases; sleeping-host cleanup; deadlines; and peer reboot recovery. It also
checks closure of a lost BEGIN and rejection of delayed activation, and retains
close acknowledgements across sleeping-host neutralization on resume. It
checks transfer completion identity, queued ownership, cancellation failures
and suppression of independent physical sources with the same HID usage.

`test_host_route.c` compiles the actual coordinator with mocked platform hooks.
It covers drain/release/preparation ordering, held-source gating, offline
selection and recovery, preparation timeout, request coalescing, generation
changes, indefinitely unproven drains, deferred sleeping-host neutral reports
and configuration replacement/removal of the selected slot.
It also compiles the actual USB-generation observation block from the UHK
adapter and checks that reconnect/protocol replacement drains old transfers and
confirms all-up delivery before reopening input. A delayed all-up completion
cannot be bypassed by elapsed time, and the drain is not rerun over pending
neutral reports.

With the pinned and patched west SDK present, also run:

```bash
bash tests/usb_left_relay/run_c2usb_completion.sh
```

This C++20 sanitizer test compiles the actual c2usb `process_ep_event` method
and transfer class, with only event/platform plumbing mocked. It covers Nordic
IN buffers consumed to zero remaining bytes, original buffer identity and
submitted length, failed/cancelled transfers, and OUT receive lengths. It also
passes those completions through the actual relay transfer-identity helper.
Before the c2usb completion patch, the consumed IN case fails; after the patch,
successful sends retire their tickets while cancellations still fail delivery.

The timer integration regression needs Python 3 and the same C sanitizer runtime:

```bash
bash tests/usb_left_relay/run_relay_poll.sh
```

It compiles the actual relay poll event handler, reuses the existing protocol
platform fakes, and models the scheduler's rule that a later request cannot
postpone an already-due timer. With no external wakeups, both relay roles must
remain active for five lease periods. Before rearming the popped event in its
handler, the left lease expires; with the rearm, heartbeats maintain the session.

The normal keyboard completion regression also needs a C++20 compiler:

```bash
bash tests/usb_left_relay/run_normal_keyboard_completion.sh
```

It compiles the actual adapter queue, tracked submission and completion methods
with mocked platform hooks. It reproduces a key-down completion arriving after
the semaphore times out and the desired report is rebuilt as key-up. The delivered
baseline must retain the immutable submitted state, so the release remains a
change requiring delivery. It covers completion ownership before main-thread
drain, cancelled reports, stale route/USB generations and selected BLE delivery.
The original completion path fails this regression.

These tests do not execute the full Zephyr/c2usb C++ adapter, Nordic UDC driver,
physical key scanner or native Windows updater. Firmware builds check adapter
integration; the hardware matrix in the
[development guide](../../doc-dev/technical/uhk80-dual-usb-hosts-development.md)
is required to establish runtime behavior and performance.
