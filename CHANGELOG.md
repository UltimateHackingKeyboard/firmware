# Changelog

All notable changes to this project will be documented in this file.

The format is loosely based on [Keep a Changelog](http://keepachangelog.com/en/1.0.0/)
and this project adheres to the [UHK Versioning](VERSIONING.md) conventions.

## [2.1.0] - 2017-10-13

Data Model: 1.0.0 (unchanged) | USB Protocol: 1.2.0 (minor bump) | Slave Protocol: 2.1.0 (minor bump)

- Add jumpToSlaveBootloader USB and slave protocol command. `USBPROTOCOL:MINOR` `SLAVEPROTOCOL:MINOR`
- Fix generic HID descriptor enumeration error.

## [2.0.0] - 2017-10-10

Data Model: 1.0.0 (unchanged) | USB Protocol: 1.1.0 (minor bump) | Slave Protocol: 2.0.0 (major bump)

- Read the hardware and user configuration area of the EEPROM upon startup and set the default keymap.
- Greatly improve the I2C watchdog and drivers. Communication between the halves or the add-ons should never fail again.
- Implement generic enumeration sequence and per-slave state for UHK modules, allowing add-ons to be added. `SLAVEPROTOCOL:MAJOR`
- Make the master cache the output fields of slave modules, allowing for more frequent input updates.
- Optimize I2C protocol scheduler resulting in increased roustness and more efficient use of I2C bandwidth.
- Add I2C message headers containing a length header, allowing for variable-length messages and a CRC16-CCITT checksum, allowing for robust communication. `SLAVEPROTOCOL:MAJOR`
- Add mechanism to dump the internal state of the KL03 via SPI for debugging purposes.
- Add merge sensor state and attached module IDs to GetDebugInfo(). `USBPROTOCOL:PATCH`
- Throw ParserError_InvalidKeymapCount if keymapCount == 0. `DATAMODEL:PATCH`

## [1.0.0] - 2017-08-30

Data Model: 1.0.0 | USB Protocol: 1.0.0 | Slave Protocol: 1.0.0

- First Release
