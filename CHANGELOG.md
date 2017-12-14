# Changelog

All notable changes to this project will be documented in this file.

The format is loosely based on [Keep a Changelog](http://keepachangelog.com/en/1.0.0/)
and this project adheres to the [UHK Versioning](VERSIONING.md) conventions.

## [6.0.0] - 2017-12-12

Data Model: 4.0.0 | USB Protocol: **3.0.0** | Slave Protocol: 3.0.0

- Change the value of almost every USB protocol commands because there were unused intervals between them. `USBPROTOCOL:MAJOR`
- Disable LED display icons by default.
- Update LED brightness levels upon applying the configuration.

## [5.0.1] - 2017-12-09

Data Model: 4.0.0 | USB Protocol: 2.0.0 | Slave Protocol: 3.0.0

 - Make key presses continue to emit scancodes even if a USB interface (typically the mouse interface) is not polled by the host anymore.
 - Make scrolling always immediately react to keypresses regardless of the previous internal scroll state.

## [5.0.0] - 2017-12-04

Data Model: **4.0.0** | USB Protocol: 2.0.0 | Slave Protocol: 3.0.0

- Move pointerRole from keymaps to module configurations as pointerMode. Add angularShift, modLayerPointerFunction, fnLayerPointerFunction, and mouseLayerPointerFunction to module configurations. `DATAMODEL:MAJOR`

## [4.0.0] - 2017-11-30

Data Model: **3.0.0** | USB Protocol: 2.0.0 | Slave Protocol: 3.0.0

- Implement mouse movement and scrolling deceleration and acceleration.
- Toggle layers upon double tapping their keys. Make the double tap timeout configurable.
- Make the parser read additional user configuration properties: dataModelMajorVersion, dataModelMinorVersion, dataModelPatchVersion, doubleTapSwitchLayerTimeout, iconsAndLayerTextsBrightness, alphanumericSegmentsBrightness, keyBacklightBrightness, mouseMoveInitialSpeed, mouseMoveAcceleration, mouseMoveDeceleratedSpeed, mouseMoveBaseSpeed, mouseMoveAcceleratedSpeed, mouseScrollInitialSpeed, mouseScrollAcceleration, mouseScrollDeceleratedSpeed, mouseScrollBaseSpeed, mouseScrollAcceleratedSpeed. `DATAMODEL:MAJOR`

## [3.0.0] - 2017-11-15

Data Model: **2.0.0** | USB Protocol: **2.0.0** | Slave Protocol: **3.0.0**

- Detect the use of USB interfaces and only wait for the ones that are actually used by the host.
- Implement key debouncer.
- Use the menu key in the factory keymap.
- Make pressing the reset button revert to the factory preset.
- Revert to the factory default state when the reset button is pressed upon firmware startup. Display FTY on the display in this case.
- Make the LED display show the abbreviation of the current keymap even when it gets reinitialized by the I2C watchdog.
- Swap SlaveCommand_RequestKeyStates and SlaveCommand_JumpToBootloader, thereby making SlaveCommand_JumpToBootloader the lower number because it's more essential and shouldn't change in the future. `SLAVEPROTOCOL:MAJOR`
- Suppress pressed keys upon layer switcher key release.
- Handle secondary role modifiers and layer switchers.
- Make UsbCommand_JumpToSlaveBootloader expect a slave slot id instead of a uhkModuleDriverId. `USBPROTOCOL:MAJOR`
- Set UsbResponse_InvalidCommand upon encountering with an invalid USB command. `USBPROTOCOL:MINOR`
- Remove UsbCommandId_ReadMergeSensor now that it can be queried via UsbCommandId_GetKeyboardState. `USBPROTOCOL:MAJOR`
- Make the getAdcValue and getDebugInfo USB commands utilize the first byte of the response to provide status as dictated by the UHK protocol. `USBPROTOCOL:MAJOR`
- Switch keymap only upon keypress.
- Handle layer toggle actions.
- Keep the active layer active even if another layer switcher key gets pressed while holding it.
- Read the new UserConfig.userConfigLength user config field. `DATAMODEL:MAJOR`
- Change Ctrl and Alt back according to the official UHK factory keymap.
- Update system keyboard HID descriptor which doesn't make the pointer go to the top left corner on OSX anymore.
- Scan keyboard matrices in a more efficient manner from timer interrupts instead of the main loop.
- Add UsbCommand_SendKbootCommand. `USBPROTOCOL:MINOR`
- Make the reenumerate USB command accept a timeout value. `USBPROTOCOL:MINOR`
- Make the config parser read the device name. `DATAMODEL:MAJOR`
- Update release file format containing device and module directories and hex files instead of srec.
- Remove obsolete ARM GCC build files.

## [2.1.0] - 2017-10-13

Data Model: 1.0.0 | USB Protocol: 1.**2.0** | Slave Protocol: 2.**1.0**

- Add jumpToSlaveBootloader USB and slave protocol command. `USBPROTOCOL:MINOR` `SLAVEPROTOCOL:MINOR`
- Fix generic HID descriptor enumeration error.

## [2.0.0] - 2017-10-10

Data Model: 1.0.0 | USB Protocol: 1.**1.0** | Slave Protocol: **2.0.0**

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
