#!/usr/bin/env bash
set -euo pipefail
repo_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)"
test_dir="$(mktemp -d)"
trap 'rm -rf "$test_dir"' EXIT
python3 - "$repo_root" "$test_dir" <<'PY'
import pathlib, sys
repo_root, test_dir = map(pathlib.Path, sys.argv[1:])
source = (repo_root / 'right/src/event_scheduler.c').read_text()
start = source.index('case EventSchedulerEvent_UsbLeftRelay:')
end = source.index('case ', start + 5)
(test_dir / 'relay_poll_event_under_test.inc').write_text(source[start:end])
helpers = (repo_root / 'tests/usb_left_relay/test_relay.c').read_text()
(test_dir / 'relay_fake_under_test.inc').write_text(helpers[:helpers.index('static void codecTests(void)')])
PY
"${CC:-cc}" -std=c11 -Wall -Wextra -Werror -g -fsanitize=address,undefined -DCONFIG_UHK_USB_LEFT_RELAY=1 \
  -I "$test_dir" -I "$repo_root/device/src" -I "$repo_root/right/src" \
  "$repo_root/device/src/usb_left_relay_protocol.c" "$repo_root/device/src/usb_left_relay.c" \
  "$repo_root/tests/usb_left_relay/test_relay_poll.c" -o "$test_dir/test-relay-poll"
"$test_dir/test-relay-poll"
