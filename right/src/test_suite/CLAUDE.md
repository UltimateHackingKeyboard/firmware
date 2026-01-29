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

- `TEST_EXPECT___________MAYBE` only logs in verbose mode (single test run or failed test rerun)
- Failure details are always logged immediately
- Failed tests are automatically rerun with verbose logging
