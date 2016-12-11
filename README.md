# Ultimate Hacking Keyboard firmware

This repository hosts the firmware of the [Ultimate Hacking Keyboard](https://ultimatehackingkeyboard.com/).

## Build

Please make sure to clone this repo with:

`git clone --recursive git@github.com:UltimateHackingKeyboard/firmware.git`

Install [Kinetis Design Studio](http://www.nxp.com/products/software-and-tools/run-time-software/kinetis-software-and-tools/ides-for-kinetis-mcus/kinetis-design-studio-integrated-development-environment-ide:KDS_IDE) (KDS) and import the project by invoking File -> Import -> General -> Existing Projects into Workspace, select the `right` directory, and click on the Finish button. At this point, you should be able to build the firmware in KDS.

Alternatively, you can use the build scripts of the `right/build/armgcc` directory.

## Contributing

Want to contribute? Let us show you [how](/CONTRIBUTING.md).
