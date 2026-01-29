# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Ultimate Hacking Keyboard (UHK) firmware supporting two hardware generations:
- **UHK80** (newer): Left half + Right half + Dongle, Zephyr/nRF Connect SDK, Bluetooth
- **UHK60** (legacy): Single right half with optional modules, MCuXpresso SDK, USB only

## Build Commands

```bash
./build.sh right make    # Incremental build (normal development)
./build.sh right build   # Full rebuild (after adding/moving files or fresh checkout)
```

## Code Organization

**`right/src/`** - Main user-facing logic, shared between UHK60 and UHK80:
- Key mapping, macros, layer handling, and core features
- `macros/` - Macro engine: commands, variables, display
- `config_parser/` - Configuration parsing
- `config_manager.c` - Runtime configuration (`Cfg` struct)

**`device/`** - UHK80-specific hardware code:
- Peripheral drivers, OLED rendering, Bluetooth handling
- Similar role to `right/src/peripherals/` for UHK60

**`shared/`** - Code shared between platforms

## Adding Macro Commands

In `right/src/macros/commands.c`:

1. **Create a dedicated function** like `processMyCommand(parser_context_t* ctx)` rather than inline code
2. Add case in `processCommand()` switch (organized by first character)
3. **Always consume arguments** even in `Macros_DryRun` mode - validation can be skipped but parsing must happen
4. Return `MacroResult_Finished` for synchronous operations
5. Document in `doc-dev/reference-manual.md`

**Use proper parsing utilities**:
- `IsEnd(ctx)` to check if at end of input (handles context expansions)
- `ConsumeToken(ctx, "keyword")` to match specific keywords
- `ConsumeAnyToken(ctx)` to consume arbitrary tokens
- `TokEnd(ctx->at, ctx->end)` to find token boundaries before consuming

**Avoid static buffers** for string arguments - pass start/end pointers to called functions.

**Don't manipulate `ctx->at` directly** unless necessary - use Consume functions.

Example pattern for capturing token values:
```c
const char* tokenStart = ctx->at;
const char* tokenEnd = TokEnd(ctx->at, ctx->end);
ConsumeAnyToken(ctx);
// Now use tokenStart/tokenEnd
```

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

## Documentation

- `doc-dev/reference-manual.md` - Formal specification of all macro commands
- `doc-dev/user-guide.md` - User-facing macro documentation
- `doc-dev/other/` - Internal docs (crash logs, testing, troubleshooting)

## Coding Standards

- 4 spaces indentation (no tabs), Unix line endings
- Always use explicit curly braces
- Extern functions: `UpperCamelCase` with `GroupName_FunctionName` pattern
- Static functions: `lowerCamelCase`
- Types: `snake_case_t`
- Format with: `clang-format -i <file>`

## Code Review Philosophy

Focus on functional aspects, not nitpicks. Only flag magic constants if they're used in multiple places and may need future changes. Don't require comments unless truly necessary.
