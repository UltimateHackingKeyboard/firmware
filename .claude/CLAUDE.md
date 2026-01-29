# UHK Firmware Development Notes

## Build System

- Build command: `./build.sh right make` (or `left`, `dongle`, `all`)
- Full rebuild: `./build.sh right build`
- Zephyr-based firmware for UHK 80

## Macro Command Implementation

When adding new macro commands in `right/src/macros/commands.c`:

1. **Create a dedicated function** like `processMyCommand(parser_context_t* ctx)` rather than inline code
2. **Always consume arguments** even in `Macros_DryRun` mode - validation can be skipped but parsing must happen
3. **Use proper parsing utilities**:
   - `IsEnd(ctx)` to check if at end of input (handles context expansions)
   - `ConsumeToken(ctx, "keyword")` to match specific keywords
   - `ConsumeAnyToken(ctx)` to consume arbitrary tokens
   - `TokEnd(ctx->at, ctx->end)` to find token boundaries before consuming
4. **Avoid static buffers** for string arguments - pass start/end pointers to called functions
5. **Don't manipulate `ctx->at` directly** unless necessary - use Consume functions

Example pattern for capturing token values:
```c
const char* tokenStart = ctx->at;
const char* tokenEnd = TokEnd(ctx->at, ctx->end);
ConsumeAnyToken(ctx);
// Now use tokenStart/tokenEnd
```

## Test Suite

Located in `right/src/test_suite/`:

- `test_suite.c` - Main orchestration, verbose/rerun logic
- `test_input_machine.c` - Processes test actions (key presses, config changes)
- `test_output_machine.c` - Validates USB reports against expectations
- `tests/*.c` - Individual test modules

### Test Actions

Defined in `test_actions.h`:
- `TEST_PRESS(key_id)` / `TEST_RELEASE(key_id)` - Simulate key events
- `TEST_DELAY(ms)` - Wait for processing
- `TEST_EXPECT(shortcuts)` - Validate USB report (OutputMachine)
- `TEST_EXPECT_MAYBE(shortcuts)` - Optional expectation
- `TEST_SET_ACTION(key_id, shortcut)` - Configure key action
- `TEST_SET_MACRO(key_id, macro_text)` - Configure inline macro
- `TEST_SET_CONFIG(config_text)` - Run set command
- `TEST_SET_SECONDARY_ROLE(key_id, scancode, role)` - Configure secondary role

### Adding New Test Actions

1. Add macro in `test_actions.h`
2. Add enum value in `test_action_type_t`
3. Add any needed fields to `test_action_t` struct
4. Handle in `test_input_machine.c` switch statement
5. Add to skip list in `test_output_machine.c` if it's an input-only action

### Verbose Logging

- `TestSuite_Verbose` controls logging
- `LOG_VERBOSE(fmt, ...)` macro for conditional logging
- Default: non-verbose, rerun failed tests with verbose
- Important: Reset `TestSuite_Verbose = false` after verbose rerun completes

## String Utilities

In `str_utils.h`:
- `string_segment_t` - Pointer pair (start, end) for non-null-terminated strings
- `StrEqual(a, aEnd, b, bEnd)` - Compare two segments
- `IsEnd(ctx)` - Check if parser is at end (use instead of `ctx->at >= ctx->end`)

## Key Action Types

In `key_action.h`:
- `KeyActionType_Keystroke` - Regular key with optional modifiers and secondary role
- `KeyActionType_InlineMacro` - Macro text pointer
- `keystroke.secondaryRole` - Secondary role ID (see `secondary_role_driver.h`)

## Configuration

- `ConfigManager_ResetConfiguration(false)` - Reset config without reloading keymap
- `Macro_ProcessSetCommand(ctx)` - Execute a set command programmatically
