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

### Global prerequisities (for both UHK60 and UHK80):

- git
- pip3
- nodejs (and optionally nvm for version management, then (e.g.) `nvm install 20; nvm use 20`)
- west (`pip3 install west`)
- (UHK60 and modules) gcc-arm-none-eabi toolchain:
  after it is installed, set an environment variable in the default shell, e.g.
  - `export ARM_GCC_DIR="/usr"` for Linux or WSL in ~/.bashrc
  - `export ARM_GCC_DIR="/opt/homebrew"` for macOS in ~/.zshrc
- (UHK80) nrfutil and nrf commandline tools:
  - https://www.nordicsemi.com/Products/Development-tools/nRF-Util/Download
  - https://www.nordicsemi.com/Products/Development-tools/nrf-command-line-tools/download

### Setup workspace

Unlike most common workflows, where the git repository is the top level directory,
this firmware uses the [west workspace](https://docs.zephyrproject.org/latest/develop/west/workspaces.html#t2-star-topology-application-is-the-manifest-repository) structure. This means that you should first
create a wrapping directory, which will store the firmware git repository, and the *west workspace*
with the third-party SW components.

Here is the initial checkout:
```bash
mkdir uhk-workspace
cd uhk-workspace
git clone --recurse-submodules git@github.com:UltimateHackingKeyboard/firmware.git
west init -l firmware --mf west.yml
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
third-party libraries can be fetched to their up-to-date state with the following command:
```bash
west update && west patch
```
While the setup must only be done once, the external software components change during development,
so this command is highly recommended to execute after checking out a new branch.

### Build a device firmware

Before you start a build, ensure that you have the correct west manifest selected (check with `west config manifest.file`),
and that the external software components are available.

For UHK80 device targets (`uhk-80-right`, `uhk-80-left`, or `uhk-dongle`), the basic command is this:
```bash
DEVICE=uhk-80-right; west build --build-dir device/build/$DEVICE device --no-sysbuild -- --preset $DEVICE
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

### Development with VS Code

It is recommended to start development in the IDE once a successful build is available, as the build parameters
aren't trivial to pass to the IDE, but it does pick up existing build configurations.
To get started, choose *Open Workspace from File...*, then select the `firmware.code-workspace` file.
Install the recommended extensions or pick the one for your single device depending on the SDK.

> Note that using *MCUXpresso for VS Code* extension currently overwrites the `mcux_includes.json` file,
these modifications shall not be committed into the git repository!

### UHK80 Minimal development setup

- Install commandline stuff from [nRF Connect SDK](https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/installation/install_ncs.html)
- You can use `./build.sh` script that basically just packs the following snippets, but should be more up to date:

    - e.g. `./build.sh uhk-80-left build make flash`, which will perform the three actions below

- If the `build.sh` doesn't suit you, then launch the nrfutil shell:
    ```bash
    nrfutil toolchain-manager launch --shell --ncs-version v2.8.0
    ```
- In the shell, you can build (e.g.) uhk-80-left as follows:

  - full build including cmake steps, as extracted from VS Code:
    ```bash
    export DEVICE=uhk-80-left
    export PWD=`pwd`
    west build --build-dir $PWD/device/build/$DEVICE $PWD/device --pristine --board $DEVICE --no-sysbuild -- -DNCS_TOOLCHAIN_VERSION=NONE -DCONF_FILE=$PWD/device/prj.conf -DOVERLAY_CONFIG=$PWD/device/prj.conf.overlays/$DEVICE.prj.conf -DBOARD_ROOT=$PWD
    ```

  - quick rebuild:
    ```bash
    export DEVICE=uhk-80-left
    export PWD=`pwd`
    west build --build-dir $PWD/device/build/$DEVICE $PWD/device
    ```

  - flash:
    ```bash
    export DEVICE=uhk-80-left
    export PWD=`pwd`
    west flash -d $PWD/device/build/$DEVICE
    ```

In case of problems, please refer to `scripts/make-release.mjs`

### UHK60 Minimal development setup

1. Install Node.js. You find the expected Node.js version in `lib/agent/.nvmrc` file. Use your OS package manager to install it. [Check the NodeJS site for more info.](https://nodejs.org/en/download/package-manager/ "Installing Node.js via package manager") Mac OS users can simply `brew install node` to get both. Should you need multiple Node.js versions on the same computer, use Node Version Manager for [Mac/Linux](https://github.com/creationix/nvm) or for [Windows](https://github.com/coreybutler/nvm-windows)

2. Build UHK Agent. `cd lib/agent && npm ci && npm run build`.

3. Still inside the Agent submodule, compile flashing util scripts. `cd packages/usb && npx tsc`.

4. Use the `west agent` command just like `west flash`, with the `--build-dir` parameter
to flash the new firmware over USB.

### Releasing

To build a full firmware tarball:

1. Run `npm install` in `scripts`.
2. Enter nrf shell `nrfutil toolchain-manager launch --shell --ncs-version v2.8.0`
2. Run `scripts/make-release.mjs`. (Or `scripts/make-release.mjs --allowSha` for development purposes.)
3. Now, the created tarball `scripts/uhk-firmware-VERSION.tar.gz` can be flashed with UHK Agent.

If `make-release.mjs` fails with a build error, it'll probably succeed in Nordic's shell environment.

1. Install [nRF Util](https://www.nordicsemi.com/Products/Development-tools/nRF-Util).
2. Install the nRF Connect Toolchain Manager with `nrfutil install toolchain-manager`
3. Enter the Toolchain Manager shell with `nrfutil toolchain-manager launch --shell`
4. Within the shell, run `make-release.mjs` according to the above.

## Contributing

Want to contribute? Let us show you [how](/CONTRIBUTING.md).
