#!/usr/bin/env bash
set -euo pipefail
repo_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)"
workspace="$(cd -- "$repo_root/.." && pwd)"
c2usb_root="${C2USB_SOURCE_DIR:-$workspace/c2usb}"
test_dir="$(mktemp -d)"
trap 'rm -rf "$test_dir"' EXIT
python3 - "$c2usb_root/c2usb/port/zephyr/udc_mac.cpp" "$test_dir/c2usb_completion_under_test.inc" <<'PY'
import pathlib, sys
source = pathlib.Path(sys.argv[1]).read_text()
start = source.index('void udc_mac::process_ep_event(net_buf* buf)')
opening = source.index('{', start)
depth = 1
end = opening + 1
while depth:
    depth += (source[end] == '{') - (source[end] == '}')
    end += 1
pathlib.Path(sys.argv[2]).write_text(source[start:end] + '\n')
PY
"${CXX:-c++}" -std=c++20 -Wall -Wextra -Werror -g -fsanitize=address,undefined \
  -I "$test_dir" -I "$c2usb_root/c2usb" \
  -I "$workspace/modules/lib/bitfilled/bitfilled" -I "$repo_root/device/src" \
  "$repo_root/tests/usb_left_relay/test_c2usb_completion.cpp" -o "$test_dir/test-c2usb-completion"
"$test_dir/test-c2usb-completion"
