# Changelog

All notable changes to this project will be documented in this file.

The format is loosely based on [Keep a Changelog](http://keepachangelog.com/en/1.0.0/)
and this project adheres to the [UHK Versioning](VERSIONING.md) conventions.

## [6.0.0] - 2017-12-12

Device Protocol: **3.0.0** | Module Protocol: 3.0.0 | User Config: 4.0.0 | Hardware Config: 1.0.0

- Change the value of almost every Device Protocol commands because there were unused intervals between them. `DEVICEPROTOCOL:MAJOR`
- Disable LED display icons by default.
- Update LED brightness levels upon applying the configuration.

## [5.0.1] - 2017-12-09

Device Protocol: 2.0.0 | Module Protocol: 3.0.0 | User Config: 4.0.0 | Hardware Config: 1.0.0

 - Make key presses continue to emit scancodes even if a USB interface (typically the mouse interface) is not polled by the host anymore.
 - Make scrolling always immediately react to keypresses regardless of the previous internal scroll state.

## [5.0.0] - 2017-12-04

Device Protocol: 2.0.0 | Module Protocol: 3.0.0 | User Config: **4.0.0** | Hardware Config: 1.0.0

- Move pointerRole from keymaps to module configurations as pointerMode. Add angularShift, modLayerPointerFunction, fnLayerPointerFunction, and mouseLayerPointerFunction to module configurations. `USERCONFIG:MAJOR`

## [4.0.0] - 2017-11-30

Device Protocol: 2.0.0 | Module Protocol: 3.0.0 | User Config: **3.0.0** | Hardware Config: 1.0.0

- Implement mouse movement and scrolling deceleration and acceleration.
- Toggle layers upon double tapping their keys. Make the double tap timeout configurable.
- Make the parser read additional user configuration properties: USERCONFIGMajorVersion, USERCONFIGMinorVersion, USERCONFIGPatchVersion, doubleTapSwitchLayerTimeout, iconsAndLayerTextsBrightness, alphanumericSegmentsBrightness, keyBacklightBrightness, mouseMoveInitialSpeed, mouseMoveAcceleration, mouseMoveDeceleratedSpeed, mouseMoveBaseSpeed, mouseMoveAcceleratedSpeed, mouseScrollInitialSpeed, mouseScrollAcceleration, mouseScrollDeceleratedSpeed, mouseScrollBaseSpeed, mouseScrollAcceleratedSpeed. `USERCONFIG:MAJOR`

## [3.0.0] - 2017-11-15

Device Protocol: **2.0.0** | Module Protocol: **3.0.0** | User Config: **2.0.0** | Hardware Config: 1.0.0

- Detect the use of USB interfaces and only wait for the ones that are actually used by the host.
- Implement key debouncer.
- Use the menu key in the factory keymap.
- Make pressing the reset button revert to the factory preset.
- Revert to the factory default state when the reset button is pressed upon firmware startup. Display FTY on the display in this case.
- Make the LED display show the abbreviation of the current keymap even when it gets reinitialized by the I2C watchdog.
- Swap SlaveCommand_RequestKeyStates and SlaveCommand_JumpToBootloader, thereby making SlaveCommand_JumpToBootloader the lower number because it's more essential and shouldn't change in the future. `MODULEPROTOCOL:MAJOR`
- Suppress pressed keys upon layer switcher key release.
- Handle secondary role modifiers and layer switchers.
- Make UsbCommand_JumpToSlaveBootloader expect a slave slot id instead of a uhkModuleDriverId. `DEVICEPROTOCOL:MAJOR`
- Set UsbResponse_InvalidCommand upon encountering with an invalid USB command. `DEVICEPROTOCOL:MINOR`
- Remove UsbCommandId_ReadMergeSensor now that it can be queried via UsbCommandId_GetKeyboardState. `DEVICEPROTOCOL:MAJOR`
- Make the getAdcValue and getDebugInfo USB commands utilize the first byte of the response to provide status as dictated by the UHK protocol. `DEVICEPROTOCOL:MAJOR`
- Switch keymap only upon keypress.
- Handle layer toggle actions.
- Keep the active layer active even if another layer switcher key gets pressed while holding it.
- Read the new UserConfig.userConfigLength user config field. `USERCONFIG:MAJOR`
- Change Ctrl and Alt back according to the official UHK factory keymap.
- Update system keyboard HID descriptor which doesn't make the pointer go to the top left corner on OSX anymore.
- Scan keyboard matrices in a more efficient manner from timer interrupts instead of the main loop.
- Add UsbCommand_SendKbootCommand. `DEVICEPROTOCOL:MINOR`
- Make the reenumerate USB command accept a timeout value. `DEVICEPROTOCOL:MINOR`
- Make the config parser read the device name. `USERCONFIG:MAJOR`
- Update release file format containing device and module directories and hex files instead of srec.
- Remove obsolete ARM GCC build files.

## [2.1.0] - 2017-10-13

Device Protocol: 1.**2.0** | Module Protocol: 2.**1.0** | User Config: 1.0.0 | Hardware Config: 1.0.0

- Add jumpToSlaveBootloader USB and Module Protocol command. `DEVICEPROTOCOL:MINOR` `MODULEPROTOCOL:MINOR`
- Fix generic HID descriptor enumeration error.

## [2.0.0] - 2017-10-10

Device Protocol: 1.**1.0** | Module Protocol: **2.0.0** | User Config: 1.0.0 | Hardware Config: 1.0.0

- Read the hardware and user configuration area of the EEPROM upon startup and set the default keymap.
- Greatly improve the I2C watchdog and drivers. Communication between the halves or the add-ons should never fail again.
- Implement generic enumeration sequence and per-slave state for UHK modules, allowing add-ons to be added. `MODULEPROTOCOL:MAJOR`
- Make the master cache the output fields of slave modules, allowing for more frequent input updates.
- Optimize I2C protocol scheduler resulting in increased roustness and more efficient use of I2C bandwidth.
- Add I2C message headers containing a length header, allowing for variable-length messages and a CRC16-CCITT checksum, allowing for robust communication. `MODULEPROTOCOL:MAJOR`
- Add mechanism to dump the internal state of the KL03 via SPI for debugging purposes.
- Add merge sensor state and attached module IDs to GetDebugInfo(). `DEVICEPROTOCOL:PATCH`
- Throw ParserError_InvalidKeymapCount if keymapCount == 0. `USERCONFIG:PATCH`

## [1.0.0] - 2017-08-30

Device Protocol: 1.0.0 | Module Protocol: 1.0.0 | User Config: 1.0.0 | Hardware Config: 1.0.0

- First Release
