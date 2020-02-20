# Changelog

All notable changes to this project will be documented in this file.

The format is loosely based on [Keep a Changelog](http://keepachangelog.com/en/1.0.0/)
and this project adheres to the [UHK Versioning](VERSIONING.md) conventions.

## [8.8.1] - 2020-02-20

Device Protocol: 4.7.0 | Module Protocol: 4.1.0 | User Config: 4.1.1 | Hardware Config: 1.0.0

- Make the empty characters of keymap abbreviations show up empty on the LED display.
- Add touchpad module slave driver.
- Enable the pull-up resistors for the GPIO pins of the hall sensors of the BlackBerry trackball. This makes the new sensors work correctly.

## [8.8.0] - 2020-02-16

Device Protocol: 4.**7.0** | Module Protocol: 4.1.0 | User Config: 4.1.1 | Hardware Config: 1.0.0

- Prevent deadlocks and races in USB semaphores. If your UHK frooze every now and then, it shouldn't happen again.
- Implement mouse buttons 4-8.
- Make arrow scancodes stick their modifiers.
- Implement functional trackpoint firmware.
- Expose toggled layer via the GetKeyboardState USB command. `DEVICEPROTOCOL:MINOR`

## [8.7.1] - 2020-01-07

Device Protocol: 4.6.0 | Module Protocol: 4.1.0 | User Config: 4.1.1 | Hardware Config: 1.0.0

- Don't make shortcuts sticky with the exception of Alt+Tab and Cmd+Tab.
- Add extra USB reports for pressing and releasing the modifiers of shortcuts. This makes the firmware play nicer with Karabiner, RDP, and possibly some other applications.
- Make scroll key actions always emit a USB scroll event, even when tapped momentarily.
- When conflicting mouse keys are pressed at the same time, make the most recent key the dominant one.
- Add key cluster, trackball, and trackpoint firmware images to the firmware tarball.
- Set sensible default key actions for modules.
- Make the key cluster scroll and the other modules move the pointer by default.
- Move from .tar.bz2 to .tar.gz as the firmware release file format because the latter can be attached in GitHub issues.

## [8.7.0] - 2019-12-03

Device Protocol: 4.**6.0** | Module Protocol: 4.1.0 | User Config: 4.1.1 | Hardware Config: 1.0.0

- Make the Agent icon of the LED display light up when Agent is running.
- Fix debouncer related data races which slightly affect the left keyboard half.
- When a dual-role key is held and the secondary role gets triggered by another key, don't debounce the latter key.
- Set the key states of disconnected modules to unpressed.
- Update module states upon disconnecting the left and right modules.
- Expose the active layer via the GetDeviceState USB command. `DEVICEPROTOCOL:MINOR`

## [8.6.0] - 2019-08-23

Device Protocol: 4.5.0 | Module Protocol: 4.**1.0** | User Config: 4.1.**1** | Hardware Config: 1.0.0

- Make the config parser accept mouse button 4 to 8. `USERCONFIG:PATCH`
- Implement API for modules to send pointer movements to the master. `MODULEPROTOCOL:MINOR`
- Fix empty macro playback.

## [8.5.4] - 2019-01-05

Device Protocol: 4.5.0 | Module Protocol: 4.0.0 | User Config: 4.1.0 | Hardware Config: 1.0.0

- Make the modifiers of shortcut keys stick only as long as the layer switcher key of a shortcut key is being held.

## [8.5.3] - 2018-10-20

Device Protocol: 4.5.0 | Module Protocol: 4.0.0 | User Config: 4.1.0 | Hardware Config: 1.0.0

- Re-enable the I2C watchdog of the left keyboard half which was accidentally disabled starting from firmware 8.4.3. This should fix the freezes of the left keyboard half.

## [8.5.2] - 2018-10-06

Device Protocol: 4.5.0 | Module Protocol: 4.0.0 | User Config: 4.1.0 | Hardware Config: 1.0.0

- Don't suppress keys upon keymap change.

## [8.5.1] - 2018-10-04

Device Protocol: 4.5.0 | Module Protocol: 4.0.0 | User Config: 4.1.0 | Hardware Config: 1.0.0

- Reset UsbReportUpdateSemaphore if it gets stuck for 100ms. This should fix occasional freezes.

## [8.5.0] - 2018-10-04

Device Protocol: 4.**5.0** | Module Protocol: 4.0.0 | User Config: 4.1.0 | Hardware Config: 1.0.0

- Send primary role modifiers consistently.
- Only allow layer switcher keys to deactivate toggled layers.
- Deactivate secondary roles when switching keymaps.
- Use the correct scancode so that commas are outputted for macros.
- Move the pointer not by 1 but by 5 pixels when testing the USB stack to make the pointer easier to see.
- Expose UsbReportUpdateSemaphore via UsbCommand_{Get,Set}Variable() `DEVICEPROTOCOL:MINOR`
- Extract CurrentTime and remove Timer_{Get,Set}CurrentTime()

## [8.4.5] - 2018-08-21

Device Protocol: 4.4.0 | Module Protocol: 4.0.0 | User Config: 4.1.0 | Hardware Config: 1.0.0

- Suppress pressed keys when the layer or keymap changes.

## [8.4.4] - 2018-08-14

Device Protocol: 4.4.0 | Module Protocol: 4.0.0 | User Config: 4.1.0 | Hardware Config: 1.0.0

- Don't wake the host if a key is held down through the beginning of sleep.
- Ensure that secondary roles are triggered consistently.

## [8.4.3] - 2018-08-12

Device Protocol: 4.4.0 | Module Protocol: 4.0.0 | User Config: 4.1.0 | Hardware Config: 1.0.0

- Compensate "double tap to lock layer" timeouts for the timer fix to make them as long as before 8.3.3

## [8.4.2] - 2018-08-02

Device Protocol: 4.4.0 | Module Protocol: 4.0.0 | User Config: 4.1.0 | Hardware Config: 1.0.0

- Fix various bugs related to secondary role handling and sticky modifier states.

## [8.4.1] - 2018-07-31

Device Protocol: 4.4.0 | Module Protocol: 4.0.0 | User Config: 4.1.0 | Hardware Config: 1.0.0

- Make some improvements to the sleep/wake code.

## [8.4.0] - 2018-07-24

Device Protocol: 4.**4.0** | Module Protocol: 4.0.0 | User Config: 4.1.0 | Hardware Config: 1.0.0

- Rewrite the key debouncer and set the press and release timeouts to 50ms.
- Add hardcoded test keymap.
- Make debounce timeouts configurable via USB. `DEVICEPROTOCOL:MINOR`
- Make the hardcoded test keymap able to trigger via USB. `DEVICEPROTOCOL:MINOR`
- Allow the USB stack test mode to be activated via USB. `DEVICEPROTOCOL:MINOR`

## [8.3.3] - 2018-07-03

Device Protocol: 4.3.1 | Module Protocol: 4.0.0 | User Config: 4.1.0 | Hardware Config: 1.0.0

- Implement the macro engine.
- Fix the timer which makes it tick twice as fast as before.
- Fix the nondeterministic bug that made USB hang.
- Restore the Windows related commits of firmware 8.3.1 because the USB hang bug has been fixed.
- Restore debouncing to 100ms until it gets really fixed.

## [8.3.2] - 2018-06-27

Device Protocol: 4.3.1 | Module Protocol: 4.0.0 | User Config: 4.1.0 | Hardware Config: 1.0.0

- Make the debouncer debounce not only on key presses but also on key releases, and change the debounce interval from 100ms to the suggested 5ms of MX switches.
- Revert the Windows related commits of firmware 8.3.1 because they introduced a nondeterministic bug that made USB hang.
- Add base layer key mappings for the left and right add-ons for testing purposes.

## [8.3.1] - 2018-06-07

Device Protocol: 4.3.1 | Module Protocol: 4.0.0 | User Config: 4.1.0 | Hardware Config: 1.0.0

- Fix media key repetition bug on Windows.
- Fix bug that made Windows unable to sleep when the UHK was plugged in.
- Fix bug that made Chrome Remote Desktop blocked from interacting on Windows.
- Fix bug that made locked layers not release. This bug was introduced in the previous release.

## [8.3.0] - 2018-06-03

Device Protocol: 4.3.1 | Module Protocol: 4.0.0 | User Config: 4.**1.0** | Hardware Config: 1.0.0

- Make the config parser handle switch layer actions with hold on double tap disabled. `USERCONFIG:MINOR`
- Set key debounce timeout from 80ms to 100ms. This should further reduce key chattering.

## [8.2.5] - 2018-05-27

Device Protocol: 4.3.1 | Module Protocol: 4.0.0 | User Config: 4.0.1 | Hardware Config: 1.0.0

- Now really fix the bug that made the hardware and user configuration not load from the EEPROM on some hosts right after firmware update.

## [8.2.4] - 2018-05-21

Device Protocol: 4.3.**1** | Module Protocol: 4.0.0 | User Config: 4.0.1 | Hardware Config: 1.0.0

- Fix the bug that made the hardware and user configuration not load from the EEPROM on some hosts right after firmware update.
- Set the signature of the hardware config to "FTY" in the RAM when the keyboard is in factory reset mode, allowing Agent to be aware of the factory reset state. `DEVICEPROTOCOL:PATCH`
- Load the hardware and user configuration from the EEPROM even in factory reset mode.
- Set key debounce timeout from 60ms to 80ms. This should further reduce key chattering.

## [8.2.3] - 2018-05-15

Device Protocol: 4.3.0 | Module Protocol: 4.0.0 | User Config: 4.0.1 | Hardware Config: 1.0.0

- Don't switch keymaps instead of playing macros.
- Make saving the user configuration faster by only writing the part of the EEPROM which actually contains the user configuration.

## [8.2.2] - 2018-05-09

Device Protocol: 4.3.0 | Module Protocol: 4.0.0 | User Config: 4.0.**1** | Hardware Config: 1.0.0

- Parse long media macro actions. `USERCONFIG:PATCH`
- Fix vendor-specific USB usage page entry. This makes the HIDAPI Linux/hidraw driver able to access interface 0.

## [8.2.1] - 2018-05-02

Device Protocol: 4.3.0 | Module Protocol: 4.0.0 | User Config: 4.0.0 | Hardware Config: 1.0.0

- Don't suppress modifier keys upon releasing a layer.
- Restore Caps Lock indicator when saving the configuration.

## [8.2.0] - 2018-04-20

Device Protocol: 4.**3.0** | Module Protocol: 4.0.0 | User Config: 4.0.0 | Hardware Config: 1.0.0

- Change the scheduling of USB reports which changes mouse pointer speeds.
- Disable LEDs while the host sleeps.
- Make any key wake up the host while it sleeps.
- Add UsbCommand_SwitchKeymap(). `DEVICEPROTOCOL:MINOR`
- Make GCC optimize the release builds for execution speed (-O3).

## [8.1.5] - 2018-04-04

Device Protocol: 4.2.0 | Module Protocol: 4.0.0 | User Config: 4.0.0 | Hardware Config: 1.0.0

- Set key debounce timeout from 30ms to 60ms. This should eliminate key chattering.
- Use the correct scancode for the menu key of the factory keymap.

## [8.1.4] - 2018-03-05

Device Protocol: 4.2.0 | Module Protocol: 4.0.0 | User Config: 4.0.0 | Hardware Config: 1.0.0

- Set key debounce timeout from 20ms to 30ms. This should eliminate key chattering.
- Set double tap lock layer timeout from 250ms to 150ms. This should minimize the chance of locking layers accidentally by double tapping their keys.

## [8.1.3] - 2018-02-18

Device Protocol: 4.2.0 | Module Protocol: 4.0.0 | User Config: 4.0.0 | Hardware Config: 1.0.0

- Fix system keyboard descriptor, so it is byte-aligned.
- Set key debounce timeout from 15ms to 20ms. This should at least reduce and hopefully eliminate key chattering.

## [8.1.2] - 2018-02-13

Device Protocol: 4.2.0 | Module Protocol: 4.0.0 | User Config: 4.0.0 | Hardware Config: 1.0.0

- Detect Caps Lock USB state and light up the Caps Lock icon of the LED display accordingly.
- Set key debounce timeout from 10ms to 15ms. This should at least reduce and hopefully eliminate key chattering.

## [8.1.1] - 2018-02-11

Device Protocol: 4.2.0 | Module Protocol: 4.0.0 | User Config: 4.0.0 | Hardware Config: 1.0.0

- Lock layers every time when double-tapping their layer switcher keys, regardless of how many times the layer switcher key was tapped before.
- Only lock layers via double-tapping if the second tap gets released within 100ms.

## [8.1.0] - 2018-01-15

Device Protocol: 4.**2**.0 | Module Protocol: 4.0.0 | User Config: 4.0.0 | Hardware Config: 1.0.0

- Enable left-half watchdog in reinit mode which seems to prevent freezes.
- Slow down main bus I2C baud rate to 30kHz when BusPal is on to make firmware transfer more robust.
- Implement UsbCommandId_GetSlaveI2cErrors. `DEVICEPROTOCOL:MINOR`
- Implement UsbCommandId_SetI2cBaudRate. `DEVICEPROTOCOL:MINOR`
- Implement DevicePropertyId_CurrentKbootCommand. `DEVICEPROTOCOL:MINOR`
- Implement DevicePropertyId_I2cMainBusBaudRate. `DEVICEPROTOCOL:MINOR`
- Implement DevicePropertyId_Uptime. `DEVICEPROTOCOL:MINOR`

## [8.0.1] - 2017-12-25

Device Protocol: 4.1.0 | Module Protocol: 4.0.0 | User Config: 4.0.0 | Hardware Config: 1.0.0

- Implement I2C watchdog for the left keyboard half which should resolve the occasional hangs of the left keyboard half.

## [8.0.0] - 2017-12-15

Device Protocol: 4.**1.0** | Module Protocol: **4.0.0** | User Config: 4.0.0 | Hardware Config: 1.0.0

- Make the modules transfer the module protocol version and firmware version composed of a major, a minor and a patch number. `MODULEPROTOCOL:MAJOR`
- Query module key count and pointer count in separate messages instead of a combined message for improved clarity. `MODULEPROTOCOL:MAJOR`
- Add new UsbCommand_GetModuleProperties() device protocol command. `DEVICEPROTOCOL:MINOR`

## [7.0.0] - 2017-12-14

Device Protocol: **4.0.0** | Module Protocol: 3.0.0 | User Config: 4.0.0 | Hardware Config: 1.0.0

- Make UsbCommand_JumpToModuleBootloader() more robust by not making it dependent on the state of the module driver.
- Don't make horizontal scrolling and vertical scrolling affect each other.
- Expose version numbers via the get property interface. `DEVICEPROTOCOL:MINOR`
- Add DevicePropertyId_ConfigSizes. `DEVICEPROTOCOL:MINOR`
- Remove DevicePropertyId_HardwareConfigSize and DevicePropertyId_UserConfigSize. `DEVICEPROTOCOL:MAJOR`

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
- Make the parser read additional user configuration properties: userConfigMajorVersion, userConfigMinorVersion, userConfigPatchVersion, doubleTapSwitchLayerTimeout, iconsAndLayerTextsBrightness, alphanumericSegmentsBrightness, keyBacklightBrightness, mouseMoveInitialSpeed, mouseMoveAcceleration, mouseMoveDeceleratedSpeed, mouseMoveBaseSpeed, mouseMoveAcceleratedSpeed, mouseScrollInitialSpeed, mouseScrollAcceleration, mouseScrollDeceleratedSpeed, mouseScrollBaseSpeed, mouseScrollAcceleratedSpeed. `USERCONFIG:MAJOR`

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
