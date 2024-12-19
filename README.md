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

### UHK80 quick dev setup

```
mkdir firmware
cd firmware
git clone --recurse-submodules git@github.com:UltimateHackingKeyboard/firmware.git
cd firmware
./build.sh setup
./build.sh all build make flash
```

In case the above doesn't work, please see (or create a ticket):
- further sections of this readme 
- content of `scripts/make-release.mjs` (this one is directive for build command form)
- content of `build.sh` (this is an auxiliary script; it works for me)

### Fetching the codebase

Note that these commands will create a [west workspace](https://docs.zephyrproject.org/latest/develop/west/workspaces.html#t2-star-topology-application-is-the-manifest-repository) in your current directory.

```bash
mkdir firmware
cd firmware
git clone --recurse-submodules git@github.com:UltimateHackingKeyboard/firmware.git
west init -l firmware
west update
west patch
west config --local build.cmake-args -- "-Wno-dev"
cd firmware/scripts
npm i
./generate-versions.mjs
```

Then, depending whether you want a full IDE experience or just minimal tools for building and flashing firmware, read *VS Code setup* or *Minimal development setup* (if you prefer a text editor + command line).

### VS Code setup

- Install [nRF Connect SDK](https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/installation/install_ncs.html) including VS Code extensions.
- In VS Code, click nRF connect icon in the left pane, then `Applications -> Create new build configuration` and select the relevant CMake preset. Now hit Build. This executes cmake steps.
- Now you can rebuild or flash using the Build and Flash actions.

### Minimal development setup

- Install commandline stuff from [nRF Connect SDK](https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/installation/install_ncs.html)
- You can use `./build.sh` script that basically just packs the following snippets, but should be more up to date:

    - e.g. `./build.sh uhk-80-left build make flash`, which will perform the three actions below

- If the `build.sh` doesn't suit you, then launch the nrfutil shell:
    ```
    nrfutil toolchain-manager launch --shell --ncs-version v2.8.0
    ```
- In the shell, you can build (e.g.) uhk-80-left as follows:

  - full build including cmake steps, as extracted from VS Code:
    ```
    export DEVICE=uhk-80-left
    export PWD=`pwd`
    west build --build-dir $PWD/device/build/$DEVICE $PWD/device --pristine --board $DEVICE --no-sysbuild -- -DNCS_TOOLCHAIN_VERSION=NONE -DCONF_FILE=$PWD/device/prj.conf -DOVERLAY_CONFIG=$PWD/device/prj.conf.overlays/$DEVICE.prj.conf -DBOARD_ROOT=$PWD
    ```

  - quick rebuild:
    ```
    export DEVICE=uhk-80-left
    export PWD=`pwd`
    west build --build-dir $PWD/device/build/$DEVICE $PWD/device
    ```

  - flash:
    ```
    export DEVICE=uhk-80-left
    export PWD=`pwd`
    west flash -d $PWD/device/build/$DEVICE
    ```

In case of problems, please refer to scripts/make-release.mjs

### Recommended tweaks

You may find this `.git/hooks/post-checkout` git hook useful:

```bash
#!/bin/bash

# Update the submodule in lib/c2usb to the commit recorded in the checked-out commit
git submodule update --init --recursive lib/c2usb
# Refresh versions.c, so that Agent always shows what commit you are on (although it doesn't indicate unstaged changes)
scripts/generate-versions.mjs
```

### Old IDE setup

2. Download and install MCUXpresso IDE for [Linux](https://ultimatehackingkeyboard.com/mcuxpressoide/mcuxpressoide-11.2.0_4120.x86_64.deb.bin), [Mac](https://ultimatehackingkeyboard.com/mcuxpressoide/MCUXpressoIDE_11.2.0_4120.pkg), or [Windows](https://ultimatehackingkeyboard.com/mcuxpressoide/MCUXpressoIDE_11.2.0_4120.exe).

3. Install the GNU ARM Eclipse Plugins for in McuXpresso IDE. This is needed to make indexing work, and to avoid the "Orphaned configuration" error message in project properties. 
    1. In MCUXpresso IDE, go to Help > "Install New Software...", then a new dialog will appear.
    2. In the Name field type `Eclipse Embedded CDT Plug-ins` and in the Location field type `https://download.eclipse.org/embed-cdt/updates/neon`, then click on the Add button.
    3. Go with the flow and install the plugin.
    
4. In the IDE, import this project by invoking *File -> Import -> General -> Existing Projects into Workspace*, select the *left* or *right* directory depending on the desired firmware, then click on the *Finish* button.

5. In order to be able to flash the firmware via USB from the IDE, you must build [Agent](https://github.com/UltimateHackingKeyboard/agent) which is Git submodule of the this repo and located in the `lib/agent` directory.

6. Finally, in the IDE, click on *Run -> External Tools -> External Tools Configurations*, then select a release firmware to be flashed such as *uhk60-right_release_kboot*, and click on the *Run* button.

Going forward, it's easier to flash the firmware of your choice by using the downwards toolbar icon which is located rightwards of the *green play + toolbox icon*.

### Old Minimal development setup

1. Install the ARM cross-compiler, cross-assembler and stdlib implementation. Eg. on Arch Linux the packages `arm-none-eabi-binutils`, `arm-none-eabi-gcc`, `arm-none-eabi-newlib`.

2. Install Node.js. You find the expected Node.js version in `lib/agent/.nvmrc` file. Use your OS package manager to install it. [Check the NodeJS site for more info.](https://nodejs.org/en/download/package-manager/ "Installing Node.js via package manager") Mac OS users can simply `brew install node` to get both. Should you need multiple Node.js versions on the same computer, use Node Version Manager for [Mac/Linux](https://github.com/creationix/nvm) or for [Windows](https://github.com/coreybutler/nvm-windows)

3. Build UHK Agent. `cd lib/agent && npm ci && npm run build`.

4. Still inside the Agent submodule, compile flashing util scripts. `cd packages/usb && npx tsc`.

5. Generate `versions.h`. `cd scripts && npm ci && ./generate-versions-h.js`

6. When developing, cd to the directory you're working on (`left`/`right`). To build and flash the firmware, run `make flash`. Plain `make` just builds without flashing.

### Releasing

To build a full firmware tarball:

1. Run `npm install` in `scripts`.
2. Run `scripts/make-release.mjs`. (Or `scripts/make-release.mjs --allowSha` for development purposes.)
3. Now, the created tarball `scripts/uhk-firmware-VERSION.tar.gz` can be flashed with UHK Agent.

If `make-release.mjs` fails with a build error, it'll probably succeed in Nordic's shell environment.

1. Install [nRF Util](https://www.nordicsemi.com/Products/Development-tools/nRF-Util).
2. Install the nRF Connect Toolchain Manager with `nrfutil install toolchain-manager`
3. Enter the Toolchain Manager shell with `nrfutil toolchain-manager launch --shell`
4. Within the shell, run `make-release.mjs` according to the above.

## Contributing

Want to contribute? Let us show you [how](/CONTRIBUTING.md).
