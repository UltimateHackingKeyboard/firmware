# Changelog

All notable changes to this project will be documented in this file.

The format is loosely based on [Keep a Changelog](http://keepachangelog.com/en/1.0.0/)
and this project adheres to the [UHK Versioning](VERSIONING.md) conventions.

## [12.3.2] - 2025-01-27

Device Protocol: 4.14.0 | Module Protocol: 4.3.0 | User Config: 8.3.0 | Hardware Config: 1.0.0 | Smart Macros: 3.1.0

- Don't require confirmation of BLE pairing on the host. Only the pairing code has to be typed on the UHK 80.

## [12.3.1] - 2025-01-16

Device Protocol: 4.14.0 | Module Protocol: 4.3.0 | User Config: 8.3.0 | Hardware Config: 1.0.0 | Smart Macros: 3.1.0

- Implement high resolution scrolling for the UHK 80.
- Fix horizontal scrolling on the UHK 80.
- Use full keyboard scancode range for the UHK 80, including scancodes such as International 1.

## [12.3.0] - 2025-01-12

Device Protocol: 4.**14.0** | Module Protocol: 4.3.0 | User Config: 8.3.0 | Hardware Config: 1.0.0 | Smart Macros: 3.**1.0**

- Make connection handling more robust.
- Add the `switchHost { next | previous | last }` macro command which iterates over host connections. `SMARTMACROS:MINOR`
- Add the `switchHost "name"` macro command, which can be used to select a specific host connection.
- Implement connection switchover. If a host connection's switchover checkbox is checked, the UHK automatically switches to this host when its connection is established.
- When a connection is disconnected, switch to the host connection that is currently connected and highest in the host connection list.
- The OLED display now shows the name of the current host connection.
- Multiple dongles can be connected at the same time.
- Support caps/num/scroll lock states in host routing.
- Add `EraseAllSettings` device protocol command. `DEVICEPROTOCOL:MINOR`

## [12.2.0] - 2025-01-01

Device Protocol: 4.**13.0** | Module Protocol: 4.3.0 | User Config: 8.3.0 | Hardware Config: 1.0.0 | Smart Macros: 3.0.0

- Fix touchpad stuck tap and doubletap.
- Always update keymap on module change, including bootup.
- Make pairing screen always on.
- Reduce audible noise and expose related settings via the device protocol. `DEVICEPROTOCOL:MINOR`
- Fix ISO LED lighting in switch test mode.
- Fade UHK 80 LEDs gradually.

## [12.1.0] - 2024-12-23

Device Protocol: 4.**12.0** | Module Protocol: 4.3.0 | User Config: 8.3.0 | Hardware Config: 1.0.0 | Smart Macros: 3.0.0

- Fix layer toggling.
- Fix ISO LEDs handling.
- Fix firmware tags and version numbers returned by the firmware and shown by Agent.
- Make USB resume from sleep via keypress work.
- Fix NewPairings flag and add GetNewPairings paging device protocol command. `DEVICEPROTOCOL:MINOR`
- Fix BLE bond clearing.
- Make mouse interface not freeze when switching between host connections.
- Add HID battery interface.
- Upgrade to nRF Connect SDK 2.8.0

## [12.0.1] - 2024-12-18

Device Protocol: 4.11.0 | Module Protocol: 4.3.0 | User Config: 8.3.0 | Hardware Config: 1.0.0 | Smart Macros: 3.0.0

- Fix left half dissapearing in Agent.
- Fix spurious loads of default module mappings.
- Fix touchpad gestures.

## [12.0.0] - 2024-12-15

Device Protocol: 4.**11.0** | Module Protocol: 4.3.0 | User Config: **8.3.0** | Hardware Config: 1.0.0 | Smart Macros: **3.0.0**

- Add support for the UHK 80. `USERCONFIG:MAJOR` `DEVICEPROTOCOL:MINOR`
- The UHK 60 v1 and UHK 60 v2 may experience some regressions due to major refactors, but this firmware should be well-suited for everyday use, as we are not aware of any major issues.
- The key ids have been changed to be more consistent with the UHK 80 and future keyboards. This means that old numeric key ids need to be migrated, ideally to key id abbreviations. Otherwise, for instance, `ifGesture 21 holdKey a` will be triggered by `h`, where formerly it used to be triggered by `j`. `SMARTMACROS:MAJOR`
- Key id abbreviations for UHK 80 have been added. Previous key id abbreviations remain the same and are universal for both the UHK 80 and UHK 60 v1/v2. `SMARTMACROS:MINOR`
- New `powerMode` macro command and battery-related `leds.*` fade timeout variables. `SMARTMACROS:MINOR`
- Implement high resolution scrolling for the UHK 60 v1 and UHK 60 v2.

## [11.2.0] - 2024-10-06

Device Protocol: 4.10.0 | Module Protocol: 4.3.0 | User Config: 7.1.0 | Hardware Config: 1.0.0 | Smart Macros: 2.**5.0**
- Implement ifHold/ifTap commands. `SMARTMACROS:MINOR`
- Fix macroStartTime for postponed time. `SMARTMACROS:PATCH`
- Keyid parser fixes. `SMARTMACROS:PATCH`

## [11.1.1] - 2024-06-27

Device Protocol: 4.10.0 | Module Protocol: 4.3.0 | User Config: 7.1.0 | Hardware Config: 1.0.0 | Smart Macros: 2.4.**4**

- Reset in-memory configuration state when saving the configuration.
- Fix press modmask modifier. `SMARTMACROS:PATCH`
- Fix mouse button 20.
- Make the fn LED light up when activating fn2 to fn5 layers.

## [11.1.0] - 2024-05-03

Device Protocol: 4.10.0 | Module Protocol: 4.3.0 | User Config: 7.**1.0** | Hardware Config: 1.0.0 | Smart Macros: 2.4.**3**

- Allow inverting horizontal scroll direction for all modules. `USERCONFIG:MINOR`
- Fix USB Switch Keymap getting reverted on first macro layer switch. `SMARTMACROS:PATCH`
- Parse key id abbreviations in `set keymapAction`. `SMARTMACROS:PATCH`

## [11.0.2] - 2024-03-15

Device Protocol: 4.10.0 | Module Protocol: 4.3.0 | User Config: 7.0.0 | Hardware Config: 1.0.0 | Smart Macros: 2.4.**2**

- Fix UHK freeze on `ifGesture #key`. `SMARTMACROS:PATCH`

## [11.0.1] - 2024-03-10

Device Protocol: 4.10.0 | Module Protocol: 4.3.0 | User Config: 7.0.0 | Hardware Config: 1.0.0 | Smart Macros: 2.4.**1**

- Fix the `ifLayer` macro command to work with native layer switching. `SMARTMACROS:PATCH`
- Fix H key backlight on UHK 60 v1 keyboards.

## [11.0.0] - 2024-02-28

Device Protocol: 4.10.0 | Module Protocol: 4.3.0 | User Config: **7.0.0** | Hardware Config: 1.0.0 | Smart Macros: 2.**4.0**

- Add user configuration entries for the new module pages, the "Typing behavior" page, and the new settings of the "Mouse key speed" page. Agent 4.0.0+ reveals the new settings. `USERCONFIG:MAJOR`
- Reorder USB interfaces to achieve better compatibility with KVM switches and docking stations. Only Agent 4.0.0+ is guaranteed to be able to save the configuration with the changed USB interfaces.
- Make per-key (monochrome) backlight work on the UHK 60 v1 (with the LEDs installed).
- Make key cluster RGB backlight work with the UHK 60 v1.
- Make the `oneShot` command wait for actual USB report change.
- Fix composite shortcuts in caret mode. `SMARTMACROS:PATCH`
- Revise advanced secondary role resolution strategy: safety-margin-related behavior might have changed.
- Add `ifLayerToggled` command. `SMARTMACROS:MINOR`
- Add `secondaryRole.advanced.triggerByMouse` config value. Advanced secondary roles are now not triggered by mouse movement by default. `SMARTMACROS:MINOR`
- Add `secondaryRole.advanced.triggerByPress` config value. `SMARTMACROS:MINOR`
- Extend UHK to 20 mouse buttons. `USERCONFIG:MINOR`
- Enable all LEDs in factory test mode.
- Make LEDs not blink momentarily upon startup.

## [10.6.0] - 2024-01-02

Device Protocol: 4.10.0 | Module Protocol: 4.3.0 | User Config: 6.0.0 | Hardware Config: 1.0.0 | Smart Macros: 2.3.0

- Set trackpoint drift reset period to 1s, resulting in quicker auto-recovery.
- Don't change the test keymap when it's active upon module reconnection.

## [10.5.0] - 2023-12-14

Device Protocol: 4.10.0 | Module Protocol: 4.3.0 | User Config: 6.0.0 | Hardware Config: 1.0.0 | Smart Macros: 2.**3.0**

- Automatically recover from trackpoint drifts after a couple of seconds. This initial implementation may use some fine-tuning, so we welcome feedback.
- Fix `final {` command. `SMARTMACROS:PATCH`
- Fix `ifDoubletap` command to not miss doubletap if another macro fit in between the two activations. `SMARTMACROS:PATCH`
- Add `ifModuleConnected` condition. `SMARTMACROS:MINOR`
- Return deprecation errors for removed functionality. `SMARTMACROS:PATCH`
- Make the smart macro documentation insert `invertScrollDirectionY` instead of `invertScrollDirection`.

## [10.4.0] - 2023-11-12

Device Protocol: 4.10.0 | Module Protocol: 4.3.0 | User Config: 6.0.0 | Hardware Config: 1.0.0 | Smart Macros: 2.**2.0**

- Implement touchpad hold continuation timeout via `set module.touchpad.holdContinuationTimeout <0-65535>` (milliseconds). If non-zero, the touchpad module allows the release of finger for the specified amount of time during drag-and-drop without the left click getting released. `SMARTMACROS:MINOR`
- Blink adaptive mode (triangle) LED display icon when recording runtime macros. `SMARTMACROS:PATCH`
- Fix `repeatFor`-related macro validation error. `SMARTMACROS:PATCH`
- Fix `"$($keyId.*)"` parsing. `SMARTMACROS:PATCH`
- Clear sticky modifiers when navigation mode is active.
- Improve macro guide documentation and add examples.

## [10.3.1] - 2023-10-13

Device Protocol: 4.10.0 | Module Protocol: 4.3.0 | User Config: 6.0.0 | Hardware Config: 1.0.0 | Smart Macros: 2.1.**1**

- Fix the `&&` macro command. `SMARTMACROS:PATCH`
- Fix the `setLedTxt` macro command to consume parameters correctly. `SMARTMACROS:PATCH`

## [10.3.0] - 2023-10-01

Device Protocol: 4.10.0 | Module Protocol: 4.3.0 | User Config: 6.0.0 | Hardware Config: 1.0.0 | Smart Macros: 2.**1.0**

- Implement macro scopes via curly brackets. `SMARTMACROS:MINOR`
- Implement `while` macro command. `SMARTMACROS:MINOR`
- Implement `else` macro condition. `SMARTMACROS:MINOR`
- Implement `exit` macro command. `SMARTMACROS:MINOR`
- Implement `oneShot` macro command timeout. `SMARTMACROS:MINOR`
- Report macro line position in errors. `SMARTMACROS:MINOR`
- Don't throw "unprocessed input" macro error as consequence of another error. `SMARTMACROS:PATCH`
- Fix redundant "unprocessed input" warning after `if BOOLEAN` condition. `SMARTMACROS:PATCH`
- Fix `oneShot` macro command timeout in combination with commands that sleep until release. `SMARTMACROS:PATCH`

## [10.2.0] - 2023-09-16

Device Protocol: 4.**10.0** | Module Protocol: 4.**3.0** | User Config: 6.0.0 | Hardware Config: 1.0.0 | Smart Macros: **2.0.0**

- Introduce named, dynamically typed variables via the `setVar` command and `$` syntax. For example: `setVar myDelay 50` and `set doubletapDelay $myDelay`
- Introduce expression parser that allows in-line arithmetics, including `+`, `-`, `*`, `/`, `%`, `min(...)`, `max(...)` and boolean conditions, including `<`, `<=`, `>`, `>=`, `==`, `!=`, `!`, `&&`, `||`. For example: `ifShift set leds.brightness ($leds.brightness * 1.5 + 0.01)` and `ifNotShift set leds.brightness ($leds.brightness / 1.5)`
- Introduce `if (EXPRESSION)` command. For example: `if ($a < 2 || $a > 4) tapKey a`
- Introduce string parser. Strings must be enclosed in double or single quotes. Double-quote strings support variable expansion and `\` escapes. For example: `write "Current keystrokeDelay is $keystrokeDelay. One plus one is $(1 + 1).\n"` and `write 'This is literal "$" character. And here you have an apostrophe '"'"'.'`
- Introduce key id parser. As a result, `ifGesture`, `ifShortcut`, and some other commands now accept key id abbreviations. For example: `ifShortcut a b holdLayer fn`
- Make `setLedTxt` accept numeric expressions. For example: `setLedTxt 500 $keystrokeDelay`
- Configuration values now can be retrieved using the `$<name>` syntax, just as variables. For example: `set leds.brightness ($leds.brightness * 1.5 + 0.01)`
- Deprecated `#`, `%`, and `@` syntaxes.
- Deprecated registers, including the `ifRegEq`, `ifNotRegEq`, `ifRegGt`, `ifRegLt`, `setReg`, `addReg`, `subReg`, `mulReg` commands.
- Deprecate `#` comments in favor of the `//` comment syntax.
- Deprecated `writeExpr` and `setStatusPart` commands.
- Implement `set autoShiftDelay <time in ms (NUMBER)>`. If nonzero, the autoshift feature is turned on. This adds shift to a scancode when the key is held for at least `autoShiftDelay` ms. For example, tapping 'a' results in 'a', pressing 'a' for a little bit longer results in 'A'. `SMARTMACROS:MINOR`
- Limit `set` command values, preventing invalid values. `SMARTMACROS:PATCH`
- Expose the status buffer via the UsbCommand_GetVariable:UsbVariable_StatusBuffer USB command. `DEVICEPROTOCOL:MINOR`
- Unify layer switching implementations, so macro commands behave like native layer switcher actions. `SMARTMACROS:PATCH`
- Expose the current keymap via the GetDeviceState USB command.
- Fix some initialization-related bugs. `DEVICEPROTOCOL:MINOR`
- Implement firmware MD5 checksums to make firmware udpates faster and more reliable. `DEVICEPROTOCOL:MINOR` `MODULEPROTOCOL:MINOR`
- Add `untoggleLayer` alias for the `unToggleLayer` macro command.
- Switch error logs to line numbers and fix trailing newlines. `SMARTMACROS:PATCH`
- Validate all macros when the configuration is updated. `SMARTMACROS:PATCH`
- Always update LEDs according to `leds.fadeTimeout`. `SMARTMACROS:PATCH`
- Alternate between segment display texts when there is more than one thing to show, such as ERR, WRN, or the name of the current keymap.
- Fix the behavior of the advanced secondary role strategy when rolling over multiple secondary role keys. Latter keys tended to produce false secondary role.

## [10.1.0] - 2023-07-26

Device Protocol: 4.9.0 | Module Protocol: 4.2.0 | User Config: 6.0.0 | Hardware Config: 1.0.0 | Smart Macros: 1.**4.0**

- Fix `oneShot` macro command for the proper composition of modifiers. `SMARTMACROS:PATCH`
- Fix `ifPrimary` and `ifSecondary` macro commands, which were not functional after reconnection onto the default secondary role mechanism. `SMARTMACROS:PATCH`
- Fix the navigation mode of modifier layers to always correspond to the base layer.
- Make non-keypress actions in modifier layers not receive the relevant modifier.
- Add `overlayKeymap`, `overlayLayer`, and `replaceLayer` macro commands. `SMARTMACROS:MINOR`
- Add `module.MODULEID.{invertScrollDirectionX,invertScrollDirectionY}` macro variables. Use `module.MODULEID.invertScrollDirectionY` instead of the now deprecated `module.MODULEID.invertScrollDirection`. `SMARTMACROS:MINOR`
- Add `$onCapsLockStateChange`, `$onNumLockStateChange`, and `$onScrollLockStateChange` macro events. Scroll Lock only works as expected on Windows. `SMARTMACROS:MINOR`
- Add `ifCapsLockOn`, `ifNotCapsLockOn`, `ifScrollLockOn`, `ifNotScrollLockOn`, `ifNumLockOn`, `ifNotNumLockOn` conditional macro commands. Scroll Lock only works as expected on Windows. `SMARTMACROS:MINOR`
- Add `set backlight.keyRgb.LAYERID.KEYID` macro command. `SMARTMACROS:MINOR`
- Add Artificial delay between touchpad presses and releases, fixing doubletap-to-select-word for systems that ignore sub-20ms mouse button releases.
- Any module pointer action now triggers secondary roles.
- Make module states not oscillate temporarily upon module attachment/detachment.

## [10.0.0] - 2023-07-10

Device Protocol: 4.9.0 | Module Protocol: 4.2.0 | User Config: **6.0.0** | Hardware Config: 1.0.0 | Smart Macros: 1.**3.0**

- Add backlighting mode, per-key RGB values, functional backlighting color values, and LED fade timeout to the user configuration. `USERCONFIG:MAJOR`
- Add `oneShot` smart macro command. `SMARTMACROS:MINOR`
- Add advanced, alphanumeric-friendly secondary role resolution strategy via `secondaryRole.*` smart macro variables. `SMARTMACROS:MINOR`
- Fix simple secondary role resolution when chording swaps releases.
- Fix macro-activated sticky modifiers.

Smart macro migration guide:
- `ifPrimary` and `ifSecondary` commands have been connected to the `simple` secondary role resolution strategy. Please migrate `ifPrimary` to `ifPrimary advancedStrategy` and `ifSecondary` to `ifSecondary advancedStrategy` in your macro commands.
- `resolveSecondary` is kept for backward compatibility without changes. We strongly advise to avoid it.

## [9.3.0] - 2023-06-25

Device Protocol: 4.9.0 | Module Protocol: 4.2.0 | User Config: 5.1.0 | Hardware Config: 1.0.0 | Smart Macros: 1.**2.0**

- Send empty report after every USB write command, fixing the bug that resulted in 'onewo' when encountering consecutive 'one', 'two' write commands. `SMARTMACROS:PATCH`
- Disallow navigation modes in modifier layers. `SMARTMACROS:MINOR`
- Switch the `leds.fadeTimeout` smart macro variable from minutes to seconds. `SMARTMACROS:MINOR`
- Add `onLayerChange` and `onKeymapLayerChange` macro events. `SMARTMACROS:MINOR`
- Add `prepend` option for the `activateKeyPostponed` command. `SMARTMACROS:MINOR`

## [9.2.3] - 2023-04-09

Device Protocol: 4.9.0 | Module Protocol: 4.2.0 | User Config: 5.1.0 | Hardware Config: 1.0.0 | Smart Macros: 1.1.2

- Fix the initialization of non-base layers, resolving the fallback of unmapped modifier layer actions.

## [9.2.2] - 2023-04-03

Device Protocol: 4.9.0 | Module Protocol: 4.2.0 | User Config: 5.1.0 | Hardware Config: 1.0.0 | Smart Macros: 1.1.2

- Make extended smart macro commands always available and ignore `set macroEngine.extendedCommands {0|1}`
- Fix `fork`, `call`, and `exec` smart macro commands on non-existent macro names. `SMARTMACROS:PATCH`
- Fix the logic of the `resolveSecondary` smart macro command. `SMARTMACROS:PATCH`

## [9.2.1] - 2023-01-16

Device Protocol: 4.9.0 | Module Protocol: 4.2.0 | User Config: 5.1.0 | Hardware Config: 1.0.0 | Smart Macros: 1.1.**1**

- Fix shortcut parser to parse `-` correctly. `SMARTMACROS:PATCH`

## [9.2.0] - 2023-01-10

Device Protocol: 4.**9**.0 | Module Protocol: 4.2.0 | User Config: 5.1.0 | Hardware Config: 1.0.0 | Smart Macros: 1.**1.0**

- Allow executing macro commands via USB. `DEVICEPROTOCOL:MINOR`
- Add set `i2cBaudRate` macro command. `SMARTMACROS:MINOR`
- Rename super to gui and control to ctrl for the sake of consistency for smart macros. The old naming remains to be supported. `SMARTMACROS:MINOR`
- Fix bug that altered media navigation mode when the zoom mode was altered. `SMARTMACROS:PATCH`
- Make `ifRegEq`, `ifNotRegEq`, `ifRegGt`, and `ifRegLt` macro commands treat registers as 32-bit values as they are instead of 8-bit values. `SMARTMACROS:PATCH`
- Fix a macro unscheduling bug that made some macros stuck during or after using the `call` smart macro command. `SMARTMACROS:PATCH`
- Fix maxiumum macro count.

## [9.1.4] - 2022-11-14

Device Protocol: 4.8.0 | Module Protocol: 4.2.0 | User Config: 5.1.0 | Hardware Config: 1.0.0 | Smart Macros: 1.0.**3**

- Fix `goTo` and related smart macro command bug. `SMARTMACROS:PATCH`
- Fix scheduling error in `call` smart macro command. `SMARTMACROS:PATCH`
- Disable USB gamepad interface as it interferes with gamepads. Will re-enable it later after exposing its feature set via Agent.

## [9.1.3] - 2022-11-10

Device Protocol: 4.8.0 | Module Protocol: 4.2.0 | User Config: 5.1.0 | Hardware Config: 1.0.0 | Smart Macros: 1.0.**2**

- Fix memory corruption in macro engine core which occasionally caused undefined behavior. `SMARTMACROS:PATCH`

## [9.1.2] - 2022-10-30

Device Protocol: 4.8.0 | Module Protocol: 4.2.0 | User Config: 5.1.0 | Hardware Config: 1.0.0 | Smart Macros: 1.0.**1**

- Fix interactive smart macro documentation to properly generate mouseKeys.{move,scroll}.initialAcceleration `SMARTMACROS:PATCH`
- Add USB gamepad interface, which cannot be used yet from Agent.

## [9.1.1] - 2022-10-21

Device Protocol: 4.8.0 | Module Protocol: 4.2.0 | User Config: 5.1.0 | Hardware Config: 1.0.0 | Smart Macros: 1.0.0

- Make the touchpad module not blink in Agent, and make its pointer movement much smoother.

## [9.1.0] - 2022-10-04

Device Protocol: 4.8.0 | Module Protocol: 4.2.0 | User Config: 5.**1**.0 | Hardware Config: 1.0.0 | Smart Macros: 1.0.0

- Fix layer activation/toggle priority. (Since v9.0.0, any layer activation, according to the base layer, released the toggled layer instead of triggering the relevant action on non-base layers.)
- Add the new layers to the list of secondary role layers. `USERCONFIG:MINOR`
- Hide the widgets of the smart macro documentation when no macro command is focused.

## [9.0.1] - 2022-09-10

Device Protocol: 4.8.0 | Module Protocol: 4.2.0 | User Config: 5.0.0 | Hardware Config: 1.0.0 | Smart Macros: 1.0.0

- Fix 16-bit scancodes, such as "start calculator".
- Make the smart macro documentation of the repo self-contained, and add docDir property to /doc/package.json, allowing Agent to fetch the online documentation of any firmware release.

## [9.0.0] - 2022-09-05

Device Protocol: 4.**8.0** | Module Protocol: 4.**2.0** | User Config: **5.0.0** | Hardware Config: 1.0.0 | Smart Macros: 1.0.0

The features marked with (*) can only be used with Agent 2.0.0 to be released soon, or above.

- Add smart macro engine. (*) `USERCONFIG:MINOR`
- Extend the original 4 layers with 4 regular layers (Fn2, Fn3, Fn4, Fn5) and 4 modifier layers (Shift, Ctrl, Alt, Super). (*) `USERCONFIG:MAJOR`
- Fix USB descriptors which caused high CPU load on Macintosh computers.
- Implement touchpad pinch-to-zoom, two-finger scrolling, and doubletap-to-drag.
- Allow rebinding touchpad tap action.
- Fix occasional trackpoint pointer jumps.
- Reload keymap when a module is swapped.
- Properly disconnect slaves on I2C reinitialization.
- Implement N-key rollover.
- Make accelerate and decelerate actions work with modules.
- Add resetTrackpoint macro command. `MODULEPROTOCOL:MINOR`
- Expose device and module git and version properties. `DEVICEPROTOCOL:MINOR` `MODULEPROTOCOL:MINOR`

## [8.10.12] - 2021-10-27

Device Protocol: 4.7.1 | Module Protocol: 4.1.0 | User Config: 4.2.0 | Hardware Config: 1.0.0

- Reduce trackball module wake up lag.
- Reduce the audible noise of UHK 60 v2 LED drivers.

## [8.10.11] - 2021-09-16

Device Protocol: 4.7.1 | Module Protocol: 4.1.0 | User Config: 4.2.0 | Hardware Config: 1.0.0

- Use the ANSI vs ISO RGB LED of the UHK 60 v2 according to the actual layout.
- Fix USB HID idle period.

## [8.10.10] - 2021-05-23

Device Protocol: 4.7.1 | Module Protocol: 4.1.0 | User Config: 4.2.0 | Hardware Config: 1.0.0

- Fix occasional, accidental scrolling on key cluster module initialization.
- Always initialize key cluster module LEDs with the correct colors.
- Fix macOS wake up and a number of USB-related compliance issues potentially resolving other USB issues.

## [8.10.9] - 2021-04-21

Device Protocol: 4.7.1 | Module Protocol: 4.1.0 | User Config: 4.2.0 | Hardware Config: 1.0.0

- Add caret navigation mode and bind it to the Fn layer.
- Fix the configuration parser to use the configuration of the attached modules upon applying the configuration.
- Fix the double tap to hold feature to not hold the layer when a key gets pressed during the second tap of the double tap.
- Fix a potential memory corruption bug related to SLAVE_COUNT that may result in random bugs occasionally.

## [8.10.8] - 2021-03-31

Device Protocol: 4.7.1 | Module Protocol: 4.1.0 | User Config: 4.2.0 | Hardware Config: 1.0.0

- Make module cursor movement more accurate, especially at slower speed.
- Update the default user configuration and factory configuration so that "double tap to lock" is only enabled for the Mouse key.
- Fix the backlight color of Tab and Backspace.

## [8.10.7] - 2021-03-11

Device Protocol: 4.7.1 | Module Protocol: 4.1.0 | User Config: 4.2.0 | Hardware Config: 1.0.0

- Allow saving the mapping of more than three modules which fixes the "Response code: 5" Agent error message.
- Adjust per-module speed and acceleration settings and add base speed setting.
- Make touchpad sensing more stable by adjusting Prox Hardware Settings firmware values.
- Set touchpad resolution to 1,000 DPI.
- Change default navigation modes to not affect mouse layer functionality.
- Improve the factory keymap in regard to modules.
- Fix the color of tab, backspace, and key cluster backspace keys on the base and mod layers.
- Adjust UHK 60 v2 backlighting according to the key backlight brightness setting.

## [8.10.6] - 2021-03-02

Device Protocol: 4.7.1 | Module Protocol: 4.1.0 | User Config: 4.2.0 | Hardware Config: 1.0.0

- Use optimized per-module speed and acceleration settings.
- Use cursor navigation mode on base layer and scroll mode on the mouse layer for right-sided modules and vice versa for the key cluster module.

## [8.10.5] - 2021-02-15

Device Protocol: 4.7.1 | Module Protocol: 4.1.0 | User Config: 4.2.0 | Hardware Config: 1.0.0

- Disable touchpad auto-sleep mode.
- Don't query touchpad delta values, resulting in a much faster refresh rate.
- Change UHK 60 v2 USB product ID from 0x6122 to 0x6124.
- Change USB product name to "UHK 60 v1" and "UHK 60 v2" according to the actual device.

## [8.10.4] - 2021-01-13

Device Protocol: 4.7.**1** | Module Protocol: 4.1.0 | User Config: 4.2.0 | Hardware Config: 1.0.0

- Make the GetKeyboardState USB command show the presence of the touchpad module. `DEVICEPROTOCOL:PATCH`

## [8.10.3] - 2020-12-23

Device Protocol: 4.7.0 | Module Protocol: 4.1.0 | User Config: 4.2.0 | Hardware Config: 1.0.0

- Support the UHK 60 v2.
- Migrate the key cluster module MCU back from MKL17Z32VFM4 to MKL03Z32VFK4.
- Add IS31FL3199 LED driver support which is utilized in the key cluster module.
- Change touchpad module I2C address.
- Increase trackball sensor resolution from 500 CPI to 1000 CPI.

## [8.10.2] - 2020-09-21

Device Protocol: 4.7.0 | Module Protocol: 4.1.0 | User Config: 4.2.0 | Hardware Config: 1.0.0

- Migrate key cluster module MCU from MKL03Z32VFK4 to MKL17Z32VFM4.
- Fix trackball module right button port.
- Handle touchpad module single tap, two finger tap, and scroll events.

## [8.10.1] - 2020-06-21

Device Protocol: 4.7.0 | Module Protocol: 4.1.0 | User Config: 4.2.0 | Hardware Config: 1.0.0

- Fix USB 3.x compatibility issues which mostly affected Ryzen PCs.

## [8.10.0] - 2020-05-25

Device Protocol: 4.7.0 | Module Protocol: 4.1.0 | User Config: 4.**2.0** | Hardware Config: 1.0.0

- Implement the $+-*/|\\<>?_'",\`@={} characters for the LED segment display. `USERCONFIG:MINOR`

## [8.9.3] - 2020-05-03

Device Protocol: 4.7.0 | Module Protocol: 4.1.0 | User Config: 4.1.1 | Hardware Config: 1.0.0

- Make sticky shortcuts not stick on the base layer.

## [8.9.2] - 2020-04-30

Device Protocol: 4.7.0 | Module Protocol: 4.1.0 | User Config: 4.1.1 | Hardware Config: 1.0.0

- Make every layer switcher action always deactivate held layers of their own layer without any side effects.
- Allow to cycle keymaps without releasing the relevant layer switcher key.

## [8.9.1] - 2020-04-23

Device Protocol: 4.7.0 | Module Protocol: 4.1.0 | User Config: 4.1.1 | Hardware Config: 1.0.0

- Fix the handling of layer toggle actions.

## [8.9.0] - 2020-04-18

Device Protocol: 4.7.0 | Module Protocol: 4.1.0 | User Config: 4.1.1 | Hardware Config: 1.0.0

- Make secondary roles resolve recursively.
- Make Ctrl+tab and Ctrl+arrow shortcuts sticky.
- Fix memory corruption in the LED display driver that might have caused occasional issues.

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
