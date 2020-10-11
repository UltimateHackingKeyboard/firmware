# Ultimate Hacking Keyboard firmware

[![Build Status](https://travis-ci.org/UltimateHackingKeyboard/firmware.svg?branch=master)](https://travis-ci.org/UltimateHackingKeyboard/firmware)

This repository hosts the firmware of the [Ultimate Hacking Keyboard](https://ultimatehackingkeyboard.com/).

If you want to use the latest firmware version for your UHK, then instead of going through the pain of building the firmware, simply download the [latest release of Agent](https://github.com/UltimateHackingKeyboard/agent/releases/latest) and update to the latest firmware version within Agent with a click of a button.

If you're one of the brave few who wants to hack the firmware then read on.

1. Make sure to clone this repo with:

`git clone --recursive git@github.com:UltimateHackingKeyboard/firmware.git`

2. Download and install MCUXpresso IDE for [Linux](https://storage.googleapis.com/ugl-static/mcuxpresso-ide/mcuxpressoide-11.2.0_4120.x86_64.deb.bin), [Mac](https://storage.googleapis.com/ugl-static/mcuxpresso-ide/MCUXpressoIDE_11.2.0_4120.pkg), or [Windows](https://storage.googleapis.com/ugl-static/mcuxpresso-ide/MCUXpressoIDE_11.2.0_4120.exe).

3. Install the GNU ARM Eclipse Plugins for in McuXpresso IDE. This is needed to make indexing work, and to avoid the "Orphaned configuration" error message in project properties. 
    1. In MCUXpresso IDE, go to Help > "Install New Software...", then a new dialog will appear.
    2. In the Name field type `Eclipse Embedded CDT Plug-ins` and in the Location field type `https://download.eclipse.org/embed-cdt/updates/neon`, then click on the Add button.
    3. Go with the flow and install the plugin.
    
4. In the IDE, import this project by invoking *File -> Import -> General -> Existing Projects into Workspace*, select the *left* or *right* directory depending on the desired firmware, then click on the *Finish* button.

5. In order to be able to flash the firmware via USB from the IDE, you must build [Agent](https://github.com/UltimateHackingKeyboard/agent) which is Git submodule of the this repo and located in the `lib/agent` directory.

6. Finally, in the IDE, click on *Run -> External Tools -> External Tools Configurations*, then select a release firmware to be flashed such as *uhk60-right_release_kboot*, and click on the *Run* button.

Going forward, it's easier to flash the firmware of your choice by using the downwards toolbar icon which is located rightwards of the *green play + toolbox icon*.

## Contributing

Want to contribute? Let us show you [how](/CONTRIBUTING.md).

## Custom Firmwares

The following list contains unofficial forks of the firmware. These forks provide functionality unavailable in the official firmware, but come without guarantees of any kind:

- [https://github.com/kareltucek/firmware](https://github.com/kareltucek/firmware) - firmware featuring macro engine extended by a set of custom commands, allowing more advanced configurations including custom layer switching logic, doubletap bindings, alternative secondary roles etc.

- [https://github.com/p4elkin/firmware](https://github.com/p4elkin/firmware) - firmware fork which comes with an alternative implementation of the secondary key role mechanism making it possible to use the feature for keys actively involved in typing (e.g. alphanumeric ones).

