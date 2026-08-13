# USB command security analysis

Analysis of the USB command surface (`right/src/usb_protocol_handler.c`, implementations in
`right/src/usb_commands/`) in the context of the proposal to **deny all USB commands except
`GetDeviceState` until the user explicitly unlocks the device from the keyboard**.

The threat model assumed here is a *hostile host*: the machine the keyboard is plugged into (or a
malicious application on it) is untrusted, and we want to prevent it from exfiltrating secrets,
persistently backdooring the keyboard, or turning the keyboard into an input-injection device
against a *different* host.

Severity legend:

- **Critical** — direct code/keystroke injection, persistence, or secret exfiltration.
- **High** — can subvert device behaviour or reveal sensitive user data.
- **Medium** — nuisance, denial of service, or weak fingerprinting.
- **Low** — read-only, non-sensitive, or already implied by the USB descriptors.

---

## Common commands (UHK60 + UHK80)

| ID   | Command                  | Concern                                | Rationale                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       | Implication of denying                                                                                                                                                                                                |
| ---- | ------------------------ | -------------------------------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| 0x00 | `GetDeviceProperty`      | **Medium**–**High**                    | Mostly benign version/uptime/checksum data. But `BleAddress`, `PairedRightPeerBleAddress`, `PairingStatus` and `NewPairings` leak BLE identity and bonding state — usable to track the user and to plan a BLE-side attack.                                                                                                                                                                                                                                                                                      | Agent cannot show firmware/version info or detect version mismatch; the whole pairing UI breaks. Consider allowing only the version/config-size subset while unlocked-gating the BLE sub-properties.                  |
| 0x01 | `Reenumerate`            | **Critical**                           | Reboots the device, and with `EnumerationMode_Bootloader` drops it into the bootloader — i.e. arbitrary firmware flashing. This is the single most dangerous command. On UHK80 with `rebootPeripherals` it also reboots left half and dongle.                                                                                                                                                                                                                                                                   | No firmware updates over USB without unlocking. This is the intended effect. Bootloader entry should arguably require a *physical* gesture, not just a software unlock flag.                                          |
| 0x04 | `ReadConfig`             | **High**                               | Dumps hardware and user config buffers. User config contains all keymaps and **macro text**, which users commonly use to store passwords/tokens (`tapKeySeq`-style credential macros). Straight exfiltration primitive.                                                                                                                                                                                                                                                                                         | Agent cannot read the current configuration — no editing, no backup. Agent becomes effectively read-only-blind.                                                                                                       |
| 0x05 | `WriteHardwareConfig`    | **High**                               | Writes the hardware-config staging buffer. Lower value than user config, but still device identity/layout tampering.                                                                                                                                                                                                                                                                                                                                                                                            | Cannot change hardware config (ISO/ANSI, iso adaptations) from the host.                                                                                                                                              |
| 0x06 | `WriteStagingUserConfig` | **Critical**                           | Stages an attacker-supplied full user configuration. Combined with `ApplyConfig` + `LaunchStorageTransfer` this is *persistent* keylogger/backdoor installation: an attacker can add macros that exfiltrate or rewrite keystrokes and have them survive a reboot on a different host.                                                                                                                                                                                                                           | No configuration upload. Agent cannot save anything.                                                                                                                                                                  |
| 0x07 | `ApplyConfig`            | **Critical**                           | Activates the staged config (parses and swaps buffers, runs `MacroEvent_OnInit`, i.e. executes `$onInit` macros). Second half of the persistence chain.                                                                                                                                                                                                                                                                                                                                                         | Uploaded config never takes effect. Harmless to deny if 0x06 is denied.                                                                                                                                               |
| 0x08 | `LaunchStorageTransfer`  | **Critical** (write) / **High** (read) | `StorageOperation_Write` commits the buffer to flash/EEPROM — this is what makes a hostile config **persistent**. Read direction pulls stored config into a readable buffer, feeding `ReadConfig`.                                                                                                                                                                                                                                                                                                              | No persistent config changes; no config restore from flash.                                                                                                                                                           |
| 0x09 | `GetDeviceState`         | **Low**                                | Reports flash-busy, halves-merged, module IDs, active layer, keymap index, status-buffer dirty flag. Active layer + keymap index are a mild side channel about what the user is doing, but the host already sees every keystroke. This is the command that must stay open for Agent to detect the device at all.                                                                                                                                                                                                | **Must remain allowed** — it is the discovery/handshake command and the vehicle for signalling "device is locked".                                                                                                    |
| 0x0b | `GetDebugBuffer`         | **Low**                                | Timer value plus I²C/matrix counters on UHK60. Weak timing side channel at most.                                                                                                                                                                                                                                                                                                                                                                                                                                | Debug tooling loses a diagnostic; nothing user-visible.                                                                                                                                                               |
| 0x0e | `GetModuleProperty`      | **Low**                                | Module version/git tag/checksum.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                | Agent cannot display module versions or detect module firmware mismatch.                                                                                                                                              |
| 0x11 | `SwitchKeymap`           | **Medium**                             | Silently switches the active keymap. An attacker can move the user onto a keymap whose bindings they know or that disables a security feature. Not persistent.                                                                                                                                                                                                                                                                                                                                                  | Loses host-side keymap switching (used by some users for per-application keymaps via host scripts). This is a *legitimate* everyday use case for some people — denying it by default is a real functional regression. |
| 0x12 | `GetVariable`            | **Medium**–**High**                    | Mostly harmless (debounce times, semaphores, `DevMode`), **except** `UsbVariable_StatusBuffer` and `UsbVariable_ShellBuffer`, which drain the macro status buffer and the Zephyr log buffer. Those can contain macro-printed data, typed content from debugging macros, and internal state.                                                                                                                                                                                                                     | Agent loses the error/status console — a significant usability loss, since macro errors are reported through the status buffer. Consider gating only the two buffer variables.                                        |
| 0x13 | `SetVariable`            | **High**                               | `DebounceTime*` can be set to values that make the keyboard drop or duplicate keys (DoS). `UsbVariable_ShellEnabled` turns on the USB log sink; `UsbVariable_DevMode` flips the device into dev mode. `TestSwitches` puts the keyboard into switch-test mode, i.e. suppresses normal typing. Enabling the shell sink is a prerequisite for reading shell output back out.                                                                                                                                       | Loses debounce tuning, switch tester, and the whole Agent "device tester"/dev-mode UI.                                                                                                                                |
| 0x14 | `ExecMacroCommand`       | **Critical**                           | Executes an **arbitrary macro command** in the macro engine. This is arbitrary "code" execution in the firmware's own language: it can `tapKeySeq` arbitrary keystrokes (input injection into whatever host the keyboard is *currently* attached to, including a different one over BLE), read/write macro variables, change config at runtime via `set`, and print secrets into the status buffer for later retrieval via `GetVariable`. Together with `Reenumerate` this is the top-priority command to lock. | Agent's macro/command console stops working, and any host-side integration that drives the keyboard through macro commands breaks.                                                                                    |
| 0x1e | `ExecShellCommand`       | **Critical** (UHK80)                   | Feeds a string straight into the Zephyr shell (`Shell_Input`). The shell exposes far more than the macro engine — settings, BT commands, memory inspection — depending on the enabled shell modules. On UHK60 it is a stub that only echoes, so **Low** there.                                                                                                                                                                                                                                                  | Loses the UHK80 remote shell. Development/diagnostics only; no normal user impact.                                                                                                                                    |
| 0x20 | `WriteModuleFirmware`    | **Critical**                           | Writes into the module-firmware buffer (`UsbCommand_WriteConfig` on `ConfigBufferId_ModuleFirmware`). Stage one of flashing attacker firmware into an attached module.                                                                                                                                                                                                                                                                                                                                          | No module firmware updates without unlocking.                                                                                                                                                                         |
| 0x21 | `FlashModule`            | **Critical**                           | Triggers the kboot flash of the staged image into a module over I²C. Persistent compromise of a peripheral that sits on the key matrix path.                                                                                                                                                                                                                                                                                                                                                                    | Same as above; this is the intended lock.                                                                                                                                                                             |
| 0x22 | `GetModuleFlashState`    | **Low**                                | Progress/error readback.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        | Flashing UI loses progress reporting (moot if 0x21 is denied).                                                                                                                                                        |
| 0x23 | `ValidateBufferCrc`      | **Low**–**Medium**                     | Computes CRC16 over a config buffer and reports match/mismatch. Read-only, but it is a *CRC oracle* over the config buffer, so with enough queries it can confirm guesses about buffer content. Its side effect (`ModuleFirmwareValidatedSize`) is part of the module-flash chain.                                                                                                                                                                                                                              | Loses upload integrity verification; module flashing would refuse to proceed.                                                                                                                                         |

## UHK80-only commands (`__ZEPHYR__`)

| ID | Command | Concern | Rationale | Implication of denying |
|----|---------|---------|-----------|------------------------|
| 0x15 | `DrawOled` | **Medium** | Draws arbitrary pixels on the canvas screen. Enables **spoofing the OLED UI** — e.g. faking the very "unlock confirmation" prompt this proposal depends on, or faking a pairing dialog. That interaction makes it more dangerous than it first looks. | Loses host-driven OLED drawing (canvas screen / Agent-side display features). |
| 0x1f | `ReadOled` | **Medium** | Reads back the framebuffer. The OLED can display macro output, status text, and pairing information, so this is an exfiltration path for whatever is on screen. | Loses OLED screenshotting (used for testing and for Agent's display preview). |
| 0x16 | `GetPairingData` | **High** | Returns the local OOB pairing data (address + `r`/`c` confirm values). Handing OOB material to a hostile host undermines the out-of-band assumption of the pairing flow. | Cannot initiate Agent-driven OOB pairing. Pairing must then be done entirely from the keyboard. |
| 0x17 | `SetPairingData` | **Critical** | Injects remote OOB data **and persists a peer address** into settings (`uhk/addr/left`, `uhk/addr/right`). An attacker can insert their own device as the paired left half / right half — i.e. become a man in the middle of the key matrix link. | Same as above; the Agent-assisted pairing workflow is gone. |
| 0x18 | `PairPeripheral` | **High** | Starts peripheral-side pairing without user presence. | No host-initiated pairing. |
| 0x19 | `PairCentral` | **High** | Starts central-side pairing; on the dongle/right this is how a new peer gets bonded. | No host-initiated pairing. |
| 0x1a | `UnpairAll` | **High** | With a zero address, deletes **all** bonds. Denial of service: the user's wireless setup is destroyed and the keyboard falls back to needing a cable. | Cannot unpair from Agent; user must unpair from the keyboard UI. |
| 0x1b | `IsPaired` | **Medium** | Bond oracle for an arbitrary address — lets a host probe which devices this keyboard is bonded to. | Agent cannot show bond status. |
| 0x1c | `EnterPairingMode` | **High** | Puts the device into OOB pairing mode without any physical action, opening a window for an attacker-controlled peer to bond. | Pairing mode must be entered from the keyboard — which is arguably the correct design anyway. |
| 0x1d | `EraseBleSettings` | **High** | `BtPair_UnpairAllNonLR()` + `Settings_Erase()`. Destructive; wipes BLE settings wholesale. | Cannot factory-reset BLE state from the host. |

## UHK60-only commands (non-Zephyr)

| ID | Command | Concern | Rationale | Implication of denying |
|----|---------|---------|-----------|------------------------|
| 0x02 | `JumpToModuleBootloader` | **Critical** | Puts a module into its bootloader — first step of flashing arbitrary module firmware. | No module firmware updates. |
| 0x03 | `SendKbootCommandToModule` | **Critical** | Sends raw kboot commands to an arbitrary I²C address. Effectively unrestricted access to the module bus, including erase/write. | No module flashing / recovery from the host. |
| 0x0a | `SetTestLed` | **Low** | Toggles the test LED. Cosmetic. | Loses a manufacturing/test feature. |
| 0x0c | `GetAdcValue` | **Low** | Reads an ADC value. | Loses a diagnostic. |
| 0x0d | `SetLedPwmBrightness` | **Medium** | Can set brightness to 0 — the user cannot see backlight-based indicators. Minor DoS / could hide a visual security cue. | Agent cannot control brightness (a normal user-facing feature today). |
| 0x0f | `GetSlaveI2cErrors` | **Low** | Error counters. | Loses a diagnostic. |
| 0x10 | `SetI2cBaudRate` | **Medium** | A bad baud rate breaks the module bus — the left half and modules stop working until reboot. DoS. | Loses an experimental tuning knob. |

---

## Summary and recommendations

### Tier 1 — vital: must remain open even when locked

Without these the device is not merely locked, it is *unidentifiable*. Agent cannot tell a secured
keyboard from a broken one, cannot negotiate a protocol version, and cannot render a meaningful
"please unlock your keyboard" message. All are read-only and leak nothing the host does not already
have by virtue of receiving every keystroke.

| ID   | Command                        | Why it is vital                                                                                                          | Residual risk                                                                                                                                                                                                           |
| ---- | ------------------------------ | ------------------------------------------------------------------------------------------------------------------------ | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| 0x09 | `GetDeviceState`               | The discovery and handshake command. Also the natural carrier for a "device is locked" status bit.                       | Active layer + keymap index are a weak activity side channel. Accept.                                                                                                                                                   |
| 0x00 | `GetDeviceProperty` *(subset)* | Agent needs `ProtocolVersions` to speak to the device at all; the version check is what stops it sending malformed data. | Allow `DeviceProtocolVersion`, `ProtocolVersions`, `ConfigSizes`, `GitTag`, `GitRepo`, `Uptime`, `BuiltFirmwareChecksumByModuleId`. **Gate** `BleAddress`, `PairedRightPeerBleAddress`, `PairingStatus`, `NewPairings`. |
| 0x0e | `GetModuleProperty`            | Module version/checksum reporting; needed to warn about module firmware mismatch, which is a correctness issue.          | Purely informational. Accept.                                                                                                                                                                                           |

### Tier 2 — harmless features: open by default, but defensible to gate

These are everyday user-facing or diagnostic features with no exfiltration or persistence value.
Denying them buys little security and costs real functionality, so the default should be open —
but a paranoid profile could close them without breaking device identification.

| ID   | Command                       | Value of keeping it open                                                                                      | Worst case if abused                                                                          |
| ---- | ----------------------------- | ------------------------------------------------------------------------------------------------------------- | ---------------------------------------------------------------------------------------------- |
| 0x11 | `SwitchKeymap`                | Host-driven per-application keymap switching — a real workflow for a subset of users.                          | Attacker silently moves the user to a keymap they know. Non-persistent, visible on the OLED/LEDs. |
| 0x0d | `SetLedPwmBrightness` (UHK60) | Backlight brightness control from Agent is a normal user-facing setting.                                       | Brightness set to 0; could hide a visual security cue. Non-persistent.                          |
| 0x0a | `SetTestLed` (UHK60)          | Manufacturing/test feature.                                                                                    | Cosmetic only.                                                                                  |
| 0x0c | `GetAdcValue` (UHK60)         | Diagnostic.                                                                                                    | Reveals an ADC reading. Negligible.                                                             |
| 0x0f | `GetSlaveI2cErrors` (UHK60)   | Diagnostic; used when troubleshooting module bus problems.                                                     | Error counters only. Negligible.                                                                |
| 0x0b | `GetDebugBuffer`              | Diagnostic; timer and matrix/I²C counters.                                                                     | Weak timing side channel. Negligible.                                                           |
| 0x22 | `GetModuleFlashState`         | Progress reporting only — read-only and inert unless `FlashModule` (0x21) is also permitted.                    | None on its own.                                                                                |

**Deliberately *not* in Tier 2: `GetVariable` (0x12).** It is tempting, because
`UsbVariable_StatusBuffer` is how macro errors reach Agent and losing it makes failures invisible.
But the same command also drains `UsbVariable_ShellBuffer`, and the status buffer itself can hold
macro-printed secrets. If the error console is judged important enough to keep in the locked state,
gate it **per variable id** — allow `StatusBuffer`, `DebounceTime*`, `UsbReportSemaphore`; deny
`ShellBuffer`, `ShellEnabled`, `DevMode`. Do not open the command wholesale.

**The critical set that the lock exists to protect** (deny unconditionally when locked, and consider
requiring physical confirmation even when unlocked):

- `Reenumerate` into bootloader (0x01)
- `WriteStagingUserConfig` / `ApplyConfig` / `LaunchStorageTransfer` write (0x06 / 0x07 / 0x08)
- `ExecMacroCommand` (0x14) and `ExecShellCommand` (0x1e)
- `WriteModuleFirmware` / `FlashModule` (0x20 / 0x21), `JumpToModuleBootloader` /
  `SendKbootCommandToModule` (0x02 / 0x03)
- `SetPairingData` (0x17) and the rest of the pairing group (0x16, 0x18–0x1d)

**The main exfiltration set:** `ReadConfig` (0x04) — macro text frequently contains credentials —
plus `GetVariable`'s `StatusBuffer` / `ShellBuffer` (0x12), `ReadOled` (0x1f) and
`GetPairingData` (0x16).

### Notable interactions

- **`DrawOled` (0x15) vs. the unlock prompt.** If the unlock confirmation is shown on the OLED, a
  host that can draw to the OLED can spoof or overdraw that prompt. `DrawOled` must be locked, and
  the unlock prompt should render on a screen the canvas cannot overwrite.
- **`ExecMacroCommand` subsumes much of the rest.** The macro engine can change config at runtime
  (`set` commands) and print to the status buffer. Locking 0x04/0x06 while leaving 0x14 open would
  achieve very little.
- **`GetVariable`/`SetVariable` are coarse.** They multiplex genuinely harmless variables
  (debounce, semaphore) with dangerous ones (`ShellEnabled`, `DevMode`, `StatusBuffer`,
  `ShellBuffer`). Per-variable gating is worth the extra code.
- **Denying everything breaks Agent's first-run experience.** With only 0x09 allowed, Agent sees a
  device it cannot identify or configure. The locked state should be *explicitly reported* — e.g. a
  device-state bit or a dedicated `UsbStatusCode_Locked` return — so Agent can tell the user to
  unlock the keyboard rather than showing a generic failure.

### Real functional regressions to weigh

Locking by default costs real features that some users rely on today, not merely dev tooling:

- Host-driven keymap switching (0x11) for per-application layouts.
- Backlight brightness control from the host (0x0d, UHK60).
- Host-side scripting that drives the keyboard via `ExecMacroCommand` (0x14).
- The Agent status/error console (0x12 `StatusBuffer`) — without it, macro errors become invisible.

A per-command (or per-category) unlock, persisted across reboots once the user has granted it, would
preserve these while still closing the drive-by-hostile-host hole.
