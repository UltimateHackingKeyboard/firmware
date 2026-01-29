# Test Suite Development Guidelines

## Key Timing

- **Debouncing**: Takes 50ms on press and another 50ms on release
- Always use `TEST_DELAY__(50)` or more after press/release actions to account for debouncing
- For sequences requiring key to be fully registered, use at least 50ms delays

## Key Selection

- **Always use right-half keys** for all tests (j, k, l, ;, p, o, i, u, m, n, h, y, 7, 8, 9, 0, etc.)
- Never use left-half keys (a, s, d, f, q, w, e, r, etc.)

## Macro Formatting

For `TEST_SET_MACRO`, always use multiline macro formatting with `\n`:

```c
// CORRECT - multiline format
TEST_SET_MACRO("j", "ifShortcut k final tapKey n\n"
                    "holdKey j")

// WRONG - single line
TEST_SET_MACRO("j", "ifShortcut k final tapKey n\n holdKey j")
```

## Expected Report Sequences

- Be explicit about all intermediate USB report states
- `tapKey` produces two reports: key down, then key up (empty)
- Modifier keystrokes produce: modifier alone, then modifier+key, then modifier alone, then empty
- Only use `TEST_EXPECT___________MAYBE` for genuinely optional timing-dependent states

## Logging Behavior

- `TestSuite_Verbose` controls logging
- `LOG_VERBOSE(fmt, ...)` macro for conditional logging
- `TEST_EXPECT___________MAYBE` only logs in verbose mode (single test run or failed test rerun)
- Failure details are always logged immediately
- Failed tests are automatically rerun with verbose logging
- Default: non-verbose, rerun failed tests with verbose
- Important: Reset `TestSuite_Verbose = false` after verbose rerun completes

## File Structure

- `test_suite.c` - Main orchestration, verbose/rerun logic
- `test_input_machine.c` - Processes test actions (key presses, config changes)
- `test_output_machine.c` - Validates USB reports against expectations
- `tests/*.c` - Individual test modules

## Test Actions

Defined in `test_actions.h`:
- `TEST_PRESS______(key_id)` / `TEST_RELEASE__U(key_id)` - Simulate key events
- `TEST_DELAY__(ms)` - Wait for processing
- `TEST_EXPECT__________(shortcuts)` - Validate USB report (OutputMachine)
- `TEST_EXPECT___________MAYBE(shortcuts)` - Optional expectation
- `TEST_CHECK_NOW(shortcuts)` - Immediate validation (InputMachine)
- `TEST_SET_ACTION(key_id, shortcut)` - Configure key action
- `TEST_SET_MACRO(key_id, macro_text)` - Configure inline macro
- `TEST_SET_CONFIG(config_text)` - Run set command
- `TEST_SET_LAYER_HOLD(key_id, layer_id)` - Configure layer hold
- `TEST_SET_LAYER_ACTION(layer_id, key_id, shortcut)` - Configure layer action
- `TEST_SET_SECONDARY_ROLE(key_id, scancode, role)` - Configure secondary role

## Adding New Test Actions

1. Add macro in `test_actions.h`
2. Add enum value in `test_action_type_t`
3. Add any needed fields to `test_action_t` struct
4. Handle in `test_input_machine.c` switch statement
5. Add to skip list in `test_output_machine.c` if it's an input-only action
