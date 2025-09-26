# Ultimate Hacking Keyboard firmware

[![CI](https://github.com/UltimateHackingKeyboard/firmware/actions/workflows/ci.yml/badge.svg)](https://github.com/UltimateHackingKeyboard/firmware/actions/workflows/ci.yml)

This repository hosts the firmware of the [Ultimate Hacking Keyboard](https://ultimatehackingkeyboard.com/).

If you want to use the latest firmware version for your UHK, then instead of going through the pain of building the firmware, simply download the [latest release of Agent](https://github.com/UltimateHackingKeyboard/agent/releases/latest) and update to the latest firmware version within Agent with a click of a button.

## Macro documentation

- Agent smart macro pane - covers commands that configure your UHK. 
- [user guide](https://github.com/UltimateHackingKeyboard/firmware/blob/master/doc-dev/user-guide.md) tries to give a a basic understanding of how macro commands can be combined, and describe common usecases.
- [reference manual](https://github.com/UltimateHackingKeyboard/firmware/blob/master/doc-dev/reference-manual.md) is a dry and rather formal list of all the commands and features.

## Developing

If you're one of the brave few who wants to hack the firmware then read on.

### Directory structure:

- `uhk-workspace` - west workspace that manages both uhk firmware repository and third party components
  - `zephyr, c2usb, bootloader, nrf` - third party components
  - `firmware` - the actual uhk repository
    - `lib/agent` - Agent as a git submodule
      - `.nvmrc` - expected Node.js version
      - `packages/usb` various development scripts to communicate with the UHK over USB
    - `scripts`
      - `make-release.mjs` - script to build a full release tarball

### Dependencies (for both UHK60 and UHK80):

- git
- pip3
- ninja
- cmake
- nodejs (and optionally nvm for version management, then (e.g.) `nvm install 22; nvm use 22`)
- west and some other python packages (`pip3 install -r scripts/requirements.txt` after you have cloned the repository)
- (UHK60 and modules) gcc-arm-none-eabi (sometimes also called arm-none-eabi-gcc) toolchain:
  after it is installed, set an environment variable in the default shell, e.g.
  - `export ARM_GCC_DIR="/usr"` for Linux or WSL in ~/.bashrc
  - `export ARM_GCC_DIR="/opt/homebrew"` for macOS in ~/.zshrc
- (UHK80) nrfutil and nrf commandline tools:
  - https://www.nordicsemi.com/Products/Development-tools/nRF-Util/Download
  - https://www.nordicsemi.com/Products/Development-tools/nrf-command-line-tools/download
- (release, might work without it)
  - https://docs.zephyrproject.org/latest/develop/toolchains/zephyr_sdk.html
- (flashing over USB) Agent built in `lib/agent` (see bellow also see `lib/agent/README.md`): 
- (optional, used by build.sh) jq, tmux

### Quick script-aided cheatsheet

Following uses the `build.sh` helper script. I use it with Ubuntu linux. If they don't work for you, see the manual instructions below.

Initial setup:

```
mkdir uhk-workspace
cd uhk-workspace
git clone --recurse-submodules git@github.com:UltimateHackingKeyboard/firmware.git
cd firmware
./build.sh setup
```

(Optional) Update and build Agent (we don't update the submodule reference as often as we should):
```
cd lib/agent
git fetch origin && git checkout origin/master
nvm use 22 && npm ci && npm run build
cd ../..
```

Pick UHK60 environment:

```
./build.sh switchMcux 
```

Or UHK80 environment:

```
./build.sh switchZephyr
```

Full build and flash of UHK80:

```
./build.sh right left dongle build flashUsb
```

Full build and flash of UHK60v2:

```
./build.sh rightv2 build flashUsb
```

Valid targets:
- UHK80: `right`, `left`, `dongle`
- UHK60: `rightv1`, `rightv2`, `left`, `keycluster`, `trackball`, `trackpoint`

Basic actions (see help for more):
- `build` - full pristine build
- `make` - incremental build
- `flash` - flash via debug probe, consider setting up `.devices` file. See `./build.sh help`.
- `flashUsb` - flash via USB
- `release` - build full release tarball

Release:

```
./build.sh release
```

### Manual workspace setup

_Note: this and following sections are redundant If you have successfully completed above build.sh procedure._

Unlike most common workflows, where the git repository is the top level directory,
this firmware uses the [west workspace](https://docs.zephyrproject.org/latest/develop/west/workspaces.html#t2-star-topology-application-is-the-manifest-repository) structure. This means that you should first
create a wrapping directory, which will store the firmware git repository, and the *west workspace*
with the third-party SW components.

Here is the initial checkout and installation of required Python packages:
```bash
mkdir uhk-workspace
cd uhk-workspace
git clone --recurse-submodules git@github.com:UltimateHackingKeyboard/firmware.git
west init -l firmware --mf west_nrfsdk.yml
west config build.cmake-args -- "-Wno-dev"
cd firmware
```
Note that there are two parallel development paths, that exist side by side.
UHK80 left, right and dongle use nRF Connect SDK, while the UHK60 and the modules use McuXpresso SDK.
If you intend to develop on the latter, use `west init -l firmware --mf west_mcuxsdk.yml` command instead.
(You can also switch later on with `west config manifest.file west_mcuxsdk.yml`.)
For the rest of the command line instructions, we assume the `pwd` to be `firmware`, the git repo root directory.

### Fetch external software components

The nRF Connect SDK or McuXpresso SDK (depending on the manifest file selection) and additional
third-party libraries must be fetched to their up-to-date state with the following command:
```bash
west update && west patch
```
This must be performed for each SDK independently, after manifest file selection.
While the setup must only be done once, the external software components change during development,
so this command is highly recommended to execute after checking out a new branch.



### Nrf shell environment

For UHK80 development, it is recommended to use Nordic's nrf shell environment:

1. (Install [nRF Util](https://www.nordicsemi.com/Products/Development-tools/nRF-Util) if you haven't done yet).
2. Install the nRF Connect Toolchain Manager with `nrfutil install toolchain-manager`
3. Enter the Toolchain Manager shell with `nrfutil toolchain-manager launch --shell --ncs-version v2.8.0`

### Build a device firmware

Before you start a build, ensure that you have the correct west manifest selected (check with `west config manifest.file`),
and that the external software components are available.

For UHK80 device targets (`uhk-80-right`, `uhk-80-left`, or `uhk-dongle`), the basic command is this:
```bash
DEVICE=uhk-80-right; west build --build-dir device/build/$DEVICE device -- --preset $DEVICE
```
A debug build can be produced by appending DEVICE name with `-debug`, e.g. `DEVICE=uhk-80-right-debug`.

For UHK60 device and module targets (`right`, `left`, `keycluster`, `trackball` or `trackpoint`), the basic command is this:
(Note that only `right` has `v1` and `v2` options, all other devices are simply `release` presets.)
```bash
DEVICE=right; west build -f --build-dir $DEVICE/build/v2-release $DEVICE -- --preset v2-release
```
A debug build can be produced by replacing `release` with `debug` in the preset.

The empty `--` separates the west command line arguments from the arguments that are forwarded to cmake.
A full, clean rebuild can be performed by adding `-p` or `--pristine` before this separator.

### Flashing a built firmware

Using the `--build-dir` parameter of the build (e.g. `device/build/uhk-80-right` or `right/build/v2-release`),
the flashing command is as follows:
```bash
BUILD_DIR=device/build/uhk-80-right; west flash --build-dir $BUILD_DIR
```

### Flashing a built firmware over USB

- (Install node.js and build Agent `cd lib/agent && npm ci && npm run build` or see see `lib/agent/README.md`.)

- Use the `west agent` command just like `west flash`, with the `--build-dir` parameter to flash the new firmware over USB.

### Development with VS Code

It is recommended to start development in the IDE once a successful build is available, as the build parameters
aren't trivial to pass to the IDE, but it does pick up existing build configurations.
To get started, choose *Open Workspace from File...*, then select the `firmware.code-workspace` file.
Install the recommended extensions or pick the one for your single device depending on the SDK.

> Note that using *MCUXpresso for VS Code* extension currently overwrites the `mcux_includes.json` file,
these modifications shall not be committed into the git repository!

### Build troubleshooting

If you encounter any issues, let us know via a github ticket and we will try to help asap. 

Places to reference if build fails:
- `scripts/make-release.mjs` - script that handles building the release, thus should contain correct build commands
- `.github/workflows/ci.yml` - github actions script that builds the firmware on every push, so contains a working dev environment setup
- `build.sh` - a linux helper script that tries to automate above steps 

### Debugging with VS Code

For UHK60 and modules, the McuXpresso SDK extention is the starting point for a debugging session.
On a first try, this error might manifest:

> Could not start GDB. Check that the file exists, and it can be manually started.
Error: Error: spawn $env{ARM_GCC_DIR}/bin/arm-none-eabi-gdb ENOENT

There are two problems to solve:
1. The arm-none-eabi package doesn't ship with gdb by default.
You can follow [this guide](https://interrupt.memfault.com/blog/installing-gdb#binaries-from-arm)
to get a full toolchain installed.
2. The extension doesn't expand the environment variable, so you'll need to modify the `.vscode/mcuxpresso-tools.json` file,
to have a hardcoded `toolchainPath` variable. (Don't push this change into the repository, obviously.)

### Releasing

Release pack can be built either from system shell or from Nordic's nrf shell. As a rule of thumb, if one fails, try the other.

To build a full firmware tarball:

1. Run `npm install` in `scripts`.
2. Either:
  - To build from system shell, make sure you have zephyr sdk installed: https://docs.zephyrproject.org/latest/develop/toolchains/zephyr_sdk.html
  - To build from nrf shell, etner `nrfutil toolchain-manager launch --shell --ncs-version v2.8.0`.
2. Run `scripts/make-release.mjs`. (Or `scripts/make-release.mjs --allowSha` for development purposes.)
3. Now, the created tarball `scripts/uhk-firmware-VERSION.tar.gz` can be flashed with UHK Agent.

## Contributing

Want to contribute? Let us show you [how](/CONTRIBUTING.md).
