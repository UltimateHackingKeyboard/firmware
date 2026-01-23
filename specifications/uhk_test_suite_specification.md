# UHK Firmware Test Suite Specification

**Version:** 1.1
**Date:** 2026-01-23
**Target:** UHK 80 Right Half

## 1. Overview

End-to-end test suite integrated directly into firmware. Tests run on-device without host interaction. Tests stored in flash, minimal RAM usage.

## 2. Core Concept

**Two Hooks:**
- **Hook 1**: Key matrix scanner - inject synthetic key events OR call normal scanner (not both)
- **Hook 2**: USB report output - trigger output validation when report changes

**Two State Machines:**
- **InputMachine**: Triggered from scanner tick. Processes input actions (press/release/delay/set_action). Skips Expect actions.
- **OutputMachine**: Triggered from USB report hook. Processes Expect and CheckNow actions. Skips input actions. Advances only when report actually changes.

Both machines share a single action index into the test. Each machine processes its own action types and skips the others without advancing. This creates natural synchronization - InputMachine blocks on Expect until OutputMachine advances past it.

**Features Tested:**
- Basic key mapping (scancodes)
- Modifiers, layers
- Macros (UHK macro engine)
- Mouse keys
- Complex action sequences

## 3. Hooks

### Hook 1: Key Matrix Scanner
- Location: `scanKeys()` in key_scanner.c
- Behavior: When `TestHooks_Active`, calls `scanTestKeys()` instead of `scanAllKeys()`
- `scanTestKeys()` copies `TestHooks_KeyStates` to `KeyStates[].hardwareSwitchState`
- Only signals EventVector when state actually changes
- Calls `TestHooks_Tick()` to advance InputMachine

### Hook 2: USB Report Capture
- Location: `UsbBasicKeyboardSendActiveReport()` before sending
- Behavior: When `TestHooks_Active`, calls `TestHooks_CaptureReport()`
- Compares new report with previous report
- Only advances OutputMachine when report actually changes
- Does not block normal USB transmission

## 4. State Machine Execution

### Action Types and Ownership

| Action          | InputMachine | OutputMachine |
|-----------------|--------------|---------------|
| Press           | ✓ executes   | skips         |
| Release         | ✓ executes   | skips         |
| Delay           | ✓ executes   | skips         |
| SetAction       | ✓ executes   | skips         |
| Expect   | skips        | ✓ validates   |
| CheckNow| ✓ validates  | ✓ validates   |
| End             | ✓ stops      | ✓ stops       |

### Expect vs CheckNow

- **Expect**: Processed only by OutputMachine. InputMachine skips it. Validates against the *next* report change.
- **CheckNow**: Processed by both machines. Validates against the *current* report state immediately. Useful for checking intermediate states.

### Execution Flow

```
Scanner tick (InputMachine):
  while action is input type (Press/Release/Delay/SetAction):
    execute action
    advance index
  if action is Expect:
    STOP (wait for OutputMachine to advance)
  if action is CheckNow:
    validate current report
    advance index
  if action is End:
    test complete

USB report change (OutputMachine):
  if action is Expect or CheckNow:
    validate report
    advance index
  else:
    FAIL (unexpected report change)
```

### Synchronization

Both machines share one action index.

- InputMachine processes input actions and advances. When it hits Expect, it STOPS without advancing, waiting for OutputMachine.
- OutputMachine runs only when USB report changes. It expects current action to be Expect or CheckNow.

A test fails if:
- OutputMachine runs but current action is not Expect/CheckNow (unexpected report)
- Validation fails (report doesn't match expected)
- Timeout waiting for expected report

## 5. Key Translation

**Problem:** Key IDs (logical) ≠ Matrix positions (physical)

**Solution:**
1. Parse Key ID string → slot_id + key_id (use existing KeyIdParser)
2. Set `TestHooks_KeyStates[slot_id][key_id]` directly
3. Scanner copies to `KeyStates[].hardwareSwitchState`

No reverse matrix lookup needed - we inject at the KeyStates level, after layout translation.

## 6. InRomMacro Action Type

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

## 7. Test Structure

Tests = arrays of actions in flash memory

**Action Types:**
- Press key
- Release key
- Delay (ms)
- Expect (wait for report change, validate)
- CheckNow (validate current report immediately)
- SetAction (map action to key)
- LoadMacro (assign ROM macro to key)
- End

**Example:**
```c
static const test_action_t test_basic_keypress[] = {
    TEST_SET_SCANCODE("r.u", HID_KEYBOARD_SC_A),
    TEST_PRESS("r.u"),
    TEST_EXPECT(0, HID_KEYBOARD_SC_A),  // expect report with 'A'
    TEST_RELEASE("r.u"),
    TEST_EXPECT(0),                      // expect empty report
    TEST_END()
};
```

## 8. Validation

**Focus:** Sequence validation, not timing

**Modes:**
- Exact: Report must match exactly (modifiers + all scancodes)
- Contains: Report must contain specified scancodes (other scancodes allowed)

## 9. Open Questions

- Test trigger mechanism (for now: function call, later: macro command)
- Test result output (LEDs, USB serial, etc.)
- Test selection (all vs. specific tests)
- Timeout handling for missing reports
