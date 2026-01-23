# UHK Firmware Test Suite Specification

**Version:** 1.0  
**Date:** 2026-01-23  
**Target:** UHK 80 Right Half

## 1. Overview

End-to-end test suite integrated directly into firmware. Tests run on-device without host interaction. Tests stored in flash, minimal RAM usage.

## 2. Core Concept

**Two Hooks:**
- **Hook 1**: Key matrix scanner - inject synthetic key events OR call normal scanner (not both)
- **Hook 2**: USB report output - capture reports before sending to USB stack

**Two State Machines:**
- **Input**: Execute test actions (press/release/delay/validate)
- **Output**: Validate USB report sequence

**Features Tested:**
- Basic key mapping (scancodes)
- Modifiers, layers
- Macros (UHK macro engine)
- Mouse keys
- Complex action sequences

## 3. Hooks

### Hook 1: Key Matrix Scanner
- Location: Key matrix scan interrupt handler
- Behavior: `if (test_active) { inject_test_events(); } else { normal_scan(); }`
- Debouncing happens later in pipeline
- Translates Key ID → matrix position via reverse layout lookup

### Hook 2: USB Report Capture
- Location: Before USB report transmission
- Captures last N reports in circular buffer (N=8-16)
- Stores: modifiers, keycodes[6], optional timestamp
- Does not block normal USB transmission

## 4. Key Translation

**Problem:** Key IDs (logical) ≠ Matrix positions (physical)

**Solution:**
1. Parse Key ID string → `key_id_t` (use existing KeyIdParser)
2. Reverse lookup through layout: `key_id_t` → (row, col)
3. Inject at matrix position

**New Function Needed:**
```c
bool KeyLayout_GetMatrixPosition(key_id_t key_id, uint8_t* row, uint8_t* col);
```

## 5. InRomMacro Action Type

**Problem:** Current macros use 16-bit offsets, can't address flash/const memory

**Solution:** New action type `KeyAction_InRomMacro`
- Direct const pointer to macro text
- Integrates with existing macro engine
- Test macros stored as const strings in flash

**Usage:**
```c
static const char test_macro[] = "Hello #l1 #delayUntilRelease";
// Assign to key via KeyAction_InRomMacro pointing to test_macro
```

## 6. Test Structure

Tests = arrays of actions in flash memory

**Action Types:**
- Press key
- Release key  
- Delay (ms)
- Check report (exact or contains)
- Set key action (map action to key)
- Load macro

**Example (pseudocode, feel free to design more convenient API, e.g., with macros):**
```c
const test_action_t test[] = {
    {SET_ACTION, key="A", action=Scancode('A')},
    {PRESS, key="A"},
    {CHECK_REPORT, expect=[0x04]},
    {RELEASE, key="A"},
    {CHECK_REPORT, expect=[]},
    {END}
};
```

## 7. Validation

**Focus:** Sequence validation, not timing (due to timing constraints)

**Modes:**
- Exact: Reports must match exactly
- Contains: Reports must contain expected codes

## 8. Open Questions

- Test trigger mechanism (boot flag, key combo, etc.)
- Test result output (LEDs, USB serial, etc.)
- Test selection (all vs. specific tests)


