# Ultimate Hacking Keyboard firmware

[![Build Status](https://cloud.drone.io/api/badges/UltimateHackingKeyboard/firmware/status.svg)](https://cloud.drone.io/UltimateHackingKeyboard/firmware)

This repository hosts the firmware of the [Ultimate Hacking Keyboard](https://ultimatehackingkeyboard.com/).

If you want to use the latest firmware version for your UHK, then instead of going through the pain of building the firmware, simply download the [latest release of Agent](https://github.com/UltimateHackingKeyboard/agent/releases/latest) and update to the latest firmware version within Agent with a click of a button.

## Developing

If you're one of the brave few who wants to hack the firmware then read on.

1. Make sure to clone this repo with:

```bash
west init -m git@github.com:UltimateHackingKeyboard/firmware-uhk80.git firmware-uhk80
cd firmware-uhk80
west update
west config --local build.cmake-args -- "-Wno-dev"
./uhk/device/patches/patch-all.sh
cd uhk/lib
git submodule init --recursive
git submodule update --init --recursive
```

Then, depending whether you want a full IDE experience or just minimal tools for building and flashing firmware, read *VS Code setup* or *Minimal development setup* (if you prefer a text editor + command line).

### VS Code setup

- Install [nRF Connect SDK](https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/installation/install_ncs.html) including VS Code extensions.
- In VS Code, click nRF connect icon in the left pane, then `Applications -> Create new build configuration` and select the relevant CMake preset. Now hit Build. This executes cmake steps.
- Now you can rebuild or flash using the Build and Flash actions.

### Minimal development setup

- Install commandline stuff from [nRF Connect SDK](https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/installation/install_ncs.html)
- Launch nrfutil shell:
    ```
    nrfutil toolchain-manager launch --shell --ncs-version v2.5.0 
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

6. To build a full firmware tarball:
    1. Run `npm install` in `scripts`.
    2. Run `scripts/make-release.js`. (Or `scripts/make-release.js --allowSha` for development purposes.)
    3. Now, the created tarball `scripts/uhk-firmware-VERSION.tar.gz` can be flashed with UHK Agent.

## Contributing

Want to contribute? Let us show you [how](/CONTRIBUTING.md).

## Custom Firmwares

The following list contains unofficial forks of the firmware. These forks provide functionality unavailable in the official firmware, but come without guarantees of any kind:

- [https://github.com/kareltucek/firmware](https://github.com/kareltucek/firmware) - firmware featuring macro engine extended by a set of custom commands, allowing more advanced configurations including custom layer switching logic, doubletap bindings, alternative secondary roles etc.

- [https://github.com/p4elkin/firmware](https://github.com/p4elkin/firmware) - firmware fork which comes with an alternative implementation of the secondary key role mechanism making it possible to use the feature for keys actively involved in typing (e.g. alphanumeric ones).

