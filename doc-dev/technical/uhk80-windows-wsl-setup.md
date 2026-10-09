# UHK80 firmware toolchain on Windows with WSL

This recipe uses WSL2 Ubuntu 24.04 for builds and native Windows for USB
flashing, without a debug probe. It records the toolchain used for the cloud
builds on 2026-10-09. Use a firmware checkout containing the experimental
relay presets to follow the [dual USB development guide](uhk80-dual-usb-hosts-development.md).

## Local setup: WSL2 Ubuntu 24.04, x86-64

Build inside WSL's Linux filesystem, such as `~/uhk-workspace`, using VS Code's
WSL extension. Use native Windows USB flashing so you do not need USB passthrough
or have to manage forwarding a device that changes from HID to a serial
bootloader. A probe is not required for builds or normal USB updates.

The verified toolchain is Nordic **nRF Connect SDK v3.3.0** plus **Zephyr SDK
0.17.4**, Python 3.12, west 1.5.0, CMake 4.4.4, and Ninja 1.13.2. The SDK version
comes from `west_nrfsdk.yml` and `zephyr/SDK_VERSION`. `build.sh` still references
NCS v2.8.0 in its installation/launch helper, so do not use that helper for this
baseline. You do not need the separate MCUX/UHK60 toolchain to work on UHK80.

The companion [Python requirements](uhk80-python-requirements.txt) records the
Python packages used by the successful cloud builds. The commands
below are a fresh-workspace recipe; do not run initialization/update over an
existing SDK checkout containing local changes.

In WSL:

```bash
sudo apt-get update
sudo apt-get install -y git python3-venv python3-dev build-essential \
  gperf device-tree-compiler curl wget xz-utils unzip file libmagic1

mkdir -p ~/uhk-workspace
cd ~/uhk-workspace
git clone https://github.com/leadZERO/UHK-Firmware.git firmware
# Before proceeding, check out the branch containing the experimental relay.
# If it is unpublished, copy that firmware checkout into this directory instead.
python3 -m venv .venv
source .venv/bin/activate
python -m pip install -r firmware/doc-dev/technical/uhk80-python-requirements.txt
cd firmware
git submodule update --init --recursive
cd ..
west init -l firmware --mf west_nrfsdk.yml
west update
cd firmware

# Apply the firmware repository's SDK patches once, after the fresh west update.
# Calling its helper directly avoids a conflicting SDK-provided "west patch".
GIT_COMMITTER_NAME='UHK SDK Setup' GIT_COMMITTER_EMAIL='setup@localhost' \
python - <<'PY'
import sys
from pathlib import Path
sys.path.insert(0, str(Path.cwd() / 'scripts'))
from west_patch import WestPatch
WestPatch().apply_patches()
PY
```

Install the Linux SDK and ARM compiler in the workspace:

```bash
cd ~/uhk-workspace
mkdir -p .tools/downloads
cd .tools/downloads
curl --fail --location --remote-name \
  https://github.com/zephyrproject-rtos/sdk-ng/releases/download/v0.17.4/zephyr-sdk-0.17.4_linux-x86_64_minimal.tar.xz
curl --fail --location --remote-name \
  https://github.com/zephyrproject-rtos/sdk-ng/releases/download/v0.17.4/toolchain_linux-x86_64_arm-zephyr-eabi.tar.xz
cat > sdk.sha256 <<'HASHES'
a7258bf50c892c4669712b2aacdfcd5e303245bcf36cb458a296039a664594f5  zephyr-sdk-0.17.4_linux-x86_64_minimal.tar.xz
1b61084fe12076a3639aa2d028b5258f41eb3d0f7d46a16264949e1d213f1915  toolchain_linux-x86_64_arm-zephyr-eabi.tar.xz
HASHES
sha256sum --check sdk.sha256
tar -xf zephyr-sdk-0.17.4_linux-x86_64_minimal.tar.xz -C ..
tar -xf toolchain_linux-x86_64_arm-zephyr-eabi.tar.xz -C ../zephyr-sdk-0.17.4
cd ../zephyr-sdk-0.17.4
./setup.sh -t arm-zephyr-eabi -h
```

Create a reusable environment activation file:

```bash
cd ~/uhk-workspace
cat > activate-uhk80.sh <<'ENV'
# Source from bash; paths follow the location of this file.
uhk_workspace="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
source "$uhk_workspace/.venv/bin/activate"
export ZEPHYR_BASE="$uhk_workspace/zephyr"
export ZEPHYR_SDK_INSTALL_DIR="$uhk_workspace/.tools/zephyr-sdk-0.17.4"
export ZEPHYR_TOOLCHAIN_VARIANT=zephyr
export CMAKE_BUILD_PARALLEL_LEVEL=5
ENV
source ./activate-uhk80.sh
cd firmware
west config manifest.file west_nrfsdk.yml
west config build.cmake-args -- -Wno-dev
python -m pip check
```

For each later terminal, source `~/uhk-workspace/activate-uhk80.sh` and enter
`~/uhk-workspace/firmware`. Do not rerun `west update` or reapply patches as part
of ordinary builds. An intentional SDK update resets vendor checkouts to their
manifest revisions and requires patches to be reapplied; save any vendor edits
first. Run the patch helper only against fresh, unpatched revisions.
