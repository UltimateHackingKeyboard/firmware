# USB Left relay host tests

Run from the repository root on Linux/WSL with a C compiler and sanitizer runtime:

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

These tests do not execute the Zephyr/c2usb C++ adapter, Nordic UDC driver,
physical key scanner or native Windows updater. Firmware builds check adapter
integration; the hardware matrix in the
[development guide](../../doc-dev/technical/uhk80-dual-usb-hosts-development.md)
is required to establish runtime behavior and performance.
