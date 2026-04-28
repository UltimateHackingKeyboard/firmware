# Test Suite Development Guidelines

## Adding a new test

1. Create `tests/test_<name>.c` defining one or more `test_action_t[]` arrays and a `test_module_t TestModule_<Name>`. Use an existing file (e.g. `test_basic.c`, `test_oneshot.c`) as a template.
2. Add `extern const test_module_t TestModule_<Name>;` to `tests/tests.h`.
3. Add `&TestModule_<Name>,` to the `AllTestModules[]` array in `tests/tests.c`.
4. Add `tests/test_<name>.c` to `CMakeLists.txt`.

## Test actions reference

All actions are defined as macros in `test_actions.h`. The struct backing them is `test_action_t`. Designated-initializer macros zero unset fields, so `layerId` defaults to `LayerId_Base` (0) when not specified.

### Input actions
- `TEST_PRESS______(key_id)` — simulate key press
- `TEST_RELEASE__U(key_id)` — simulate key release
- `TEST_DELAY__(ms)` — wait (use ≥ 50ms after press/release for debounce; see Key Timing)

### Configuration actions (which layer they target)
| Macro | Layer |
| --- | --- |
| `TEST_SET_ACTION(key_id, shortcut)` | Base only |
| `TEST_SET_MACRO(key_id, macro_text)` | Base only |
| `TEST_SET_LAYER_MACRO(layer_id, key_id, macro_text)` | Specified layer |
| `TEST_SET_LAYER_HOLD(key_id, layer_id)` | Sets a layer-hold action on Base **and** on the target layer (so the layer remains held while the key is held) |
| `TEST_SET_LAYER_DOUBLETAP_TOGGLE(key_id, layer_id)` | Same dual-write as `TEST_SET_LAYER_HOLD`, mode = HoldAndDoubleTapToggle |
| `TEST_SET_LAYER_ACTION(layer_id, key_id, shortcut)` | Specified layer (shortcut, **not** a macro) |
| `TEST_SET_SECONDARY_ROLE(key_id, primary_scancode, role)` | Base only |
| `TEST_SET_GENERIC_ACTION(key_id, action)` | Base only |
| `TEST_SET_EMPTY(key_id)` | Base only |
| `TEST_SET_CONFIG(config_text)` | Runs a `set` command (no key mapping) |

`shortcut` strings go through `MacroShortcutParser_Parse` — same syntax as in macros (e.g. `"LS-i"`, `"sLA-tab"`, `"tab"`).

`macro_text` is the inline-macro body — same syntax you'd type in the user-facing config (multi-line, separate commands with `\n`).

### Validation actions
- `TEST_EXPECT__________(shortcuts)` — assert the next USB report change matches; takes space-separated shortcuts (e.g. `"LS"`, `"LS-i"`, `"LA-tab"`, `""` for empty)
- `TEST_EXPECT___________MAYBE(shortcuts)` — optional report (consumed if it appears, skipped otherwise); only use for genuinely timing-dependent intermediate states
- `TEST_CHECK_NOW(shortcuts)` — validate the current report immediately (no waiting)
- `TEST_END()` — marks end of action array

## Layer fall-through note

`LayerId_Mod` and the keystroke-modifier layers (`LayerId_Shift`/`Ctrl`/`Alt`/`Gui`) have a non-zero `modifierLayerMask`, so a key with no action on that layer falls through to its **base-layer** action. `LayerId_Fn`/`Mouse`/etc. do **not** fall through.

This matters when you want a base-layer macro to run while a layer is held (e.g. for sticky-modifier `Stick_Smart` behavior, which checks `ActiveLayerHeld`): on Mod, you can leave the macro on base; on Fn, you must use `TEST_SET_LAYER_MACRO`.

## Key Timing

- **Debouncing**: 50ms on press and another 50ms on release.
- Always `TEST_DELAY__(50)` (or more) after a press/release to let the key register.

## Key Selection

- **Always use right-half keys**: j, k, l, ;, p, o, i, u, m, n, h, y, 7, 8, 9, 0, etc.
- Never use left-half keys (a, s, d, f, q, w, e, r, etc.) — the test rig only drives the right half.

## Macro Formatting

For `TEST_SET_MACRO` / `TEST_SET_LAYER_MACRO`, use multiline format with `\n`:

```c
// CORRECT
TEST_SET_MACRO("j", "ifShortcut k final tapKey n\n"
                    "holdKey j")

// WRONG
TEST_SET_MACRO("j", "ifShortcut k final tapKey n\n holdKey j")
```

## Expected Report Sequences

- Be explicit about all intermediate USB report states.
- A keystroke with modifiers produces: modifier alone, then modifier+key, then modifier alone (on release), then empty.
- `tapKey` produces: key down, then key up (empty).
- `holdKey sLA-x` produces: `LA` (mods first), then `LA-x`, then on key release `LA` (sticky stays if `Stick_Smart` + layer held), then `""` once layer is released.
- Reserve `TEST_EXPECT___________MAYBE` for genuinely timing-dependent reports.

## Adding New Test Actions

1. Add the macro in `test_actions.h`.
2. Add an enum value in `test_action_type_t`.
3. If new fields are needed, add them to `test_action_t`.
4. Handle the new case in `test_input_machine.c`'s switch.
5. Add it to the skip list in `test_output_machine.c` if it is an input-only action.
6. Document it in the table above.

## File Layout

- `test_suite.c` — orchestration, verbose/rerun logic
- `test_input_machine.c` — applies test actions (presses, config changes)
- `test_output_machine.c` — validates USB reports against expectations
- `tests/*.c` — individual test modules

## Logging

- `TestSuite_Verbose` controls logging.
- `LOG_VERBOSE(fmt, ...)` is conditional.
- `TEST_EXPECT___________MAYBE` only logs in verbose mode.
- Failures are always logged immediately; failed tests are auto-rerun with verbose enabled.
- Reset `TestSuite_Verbose = false` after a verbose rerun.
