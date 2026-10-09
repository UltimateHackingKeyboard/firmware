#!/usr/bin/env bash
set -euo pipefail
repo_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)"
test_dir="$(mktemp -d)"
trap 'rm -rf "$test_dir"' EXIT
"${CC:-cc}" -std=c11 -Wall -Wextra -Werror -g -fsanitize=address,undefined \
  -I "$repo_root/device/src" -I "$repo_root/right/src" \
  "$repo_root/device/src/usb_left_relay_protocol.c" \
  "$repo_root/device/src/usb_left_relay.c" \
  "$repo_root/tests/usb_left_relay/test_relay.c" -o "$test_dir/test-relay"
"$test_dir/test-relay"
"${CC:-cc}" -std=c11 -Wall -Wextra -Werror -g -fsanitize=address,undefined \
  -I "$repo_root/device/src" -I "$repo_root/right/src" -I "$repo_root/shared" \
  "$repo_root/tests/usb_left_relay/test_host_route.c" -o "$test_dir/test-host-route"
"$test_dir/test-host-route"
