# UHK Versioning

This document describes the various version numbers that are used by the UHK.

## Version number format

The format of the UHK version numbers is a subset of [Semantic Versioning](http://semver.org/) that only allows major, minor, and patch numbers as part of the version number. No alpha, beta, rc, or any extensions are allowed.

## USB protocol version

The UHK USB protocol is used by the UHK to interact with the host system via USB. [Agent](https://github.com/UltimateHackingKeyboard/agent) speaks the UHK USB protocol in order to read and write the configuration of the UHK, and to query its state or manipulate it in any way. Please note that the UHK USB protocol doesn't have anything to do with standard keyboard and mouse functions which are taken care by operating system drivers.

* The major number is bumped upon breaking the protocol, such as changing a protocol command ID or changing an already utilized byte within the payload.
* The minor number is bumped upon an extension of the protocol, such as adding a new protocol command ID, or utilizing a previously unutilized byte within the payload.
* The patch number is bumped upon a protocol related implementation fix, for example adding a new validation check.

In order for a host application to communicate with the UHK, its major USB protocol version must match, and its minor USB protocol version must be less or equal.

## Slave protocol version

The Slave protocol is the I2C based application protocol of the UHK via which the master module (right keyboard half), and the slave modules (left keyboard half, left add-on, right add-on) communicate.

* The major number is bumped upon breaking the protocol, such as changing a protocol command ID or changing an already utilized byte within the payload.
* The minor number is bumped upon an extension of the protocol, such as adding a new protocol command ID, or extending the payload.
* The patch number is bumped upon a protocol related implementation fix, for example adding a new validation check.

In order for the master module to communicate with the slave modules, its major slave protocol version must match, and its minor slave protocol version must be less or equal.

## Data model version

The data model is the binary serialization format of the user configuration which includes keymaps, macros, and every other configuration item.

* The major number is bumped upon breaking the data model, such as adding a new item type or changing a previously utilized interval of an already exising item type.
* The minor number is bumped upon an extension of the data model, such as using a previously unutilized interval of a type number to add a new item type.
* The patch number is bumped upon a data model related implementation fix, for example adding a new validation check.

In order for a host application to parse the configuration of the UHK, its major data model version must match, and its minor data model version must be less or equal.

For the sake of completeness, it's worth mentioning that not only the (user) data model exists, but the hardware data model too which contains hardware-specific configuration items, such as ANSI vs ISO keyboard type. The hardware data model also has a version number field, but it's not expected to ever change so for the sake of simplicity, it's not included into changelog releases. The hardware configuration version is 1.0.0

## Firmware version

The version number of the firmware changes according to the following rules.

* The major number is bumped if the major number of any of the above version numbers is bumped.
* The minor number is bumped if the minor number of any of the above version numbers is bumped.
* The patch number is bumped if the patch number of any of the above version numbers is bumped.
