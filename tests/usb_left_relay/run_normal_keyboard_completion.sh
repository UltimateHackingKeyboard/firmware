#!/usr/bin/env bash
set -euo pipefail
repo_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)"
source_root="${NORMAL_COMPLETION_SOURCE_DIR:-$repo_root}"
test_dir="$(mktemp -d)"
trap 'rm -rf "$test_dir"' EXIT
python3 - "$source_root" "$test_dir/normal_completion_under_test.inc" <<'PY'
import pathlib, sys
root = pathlib.Path(sys.argv[1])
source = (root / 'right/src/hid/usb_left_adapter.cpp').read_text()
def function(text, marker):
    start = text.index(marker)
    opening = text.index('{', start)
    depth, end = 1, opening + 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[start:end]
pieces = [function(source, marker) for marker in [
    'extern "C" void Hid_LocalUsbComplete(',
    'extern "C" void Hid_LocalUsbDrainCompletions(void)',
]]
if 'const hid_keyboard_report_t *keyboard = nullptr' in source:
    pieces = ['#define NORMAL_SNAPSHOT_SUPPORT 1',
              function(source, 'static int trackedSend('),
              function(source, 'static int queueUsb(')] + pieces
pathlib.Path(sys.argv[2]).write_text('\n'.join(pieces) + '\n')
PY
"${CXX:-c++}" -std=c++20 -Wall -Wextra -Werror -Wno-missing-field-initializers -g -fsanitize=address,undefined \
  -I "$test_dir" -I "$repo_root/device/src" \
  "$repo_root/tests/usb_left_relay/test_normal_keyboard_completion.cpp" -o "$test_dir/test-normal-completion"
"$test_dir/test-normal-completion"
