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
- Do not use autoformatting tools - manually format code to match surrounding codebase style

## Code Review Philosophy

Focus on functional aspects, not nitpicks. Only flag magic constants if they're used in multiple places and may need future changes. Don't require comments unless truly necessary.

## UHK80 / Zephyr Dev Environment — Findings

Key gotchas discovered while setting up:

- **Re-run `./build.sh update` after any branch/manifest switch.** The west modules (zephyr, nrf, …)
  are pinned per `west_nrfsdk.yml`; if they don't match the current checkout, CMake configure fails
  with `Error finding board: uhk-80-right ... Malformed "build" section ... SchemaError`. This is a
  Zephyr version mismatch, not a `board.yml` bug — `./build.sh update` (west update + patch) fixes it.
- **Single-device builds run in the foreground** (`./build.sh right build`), which is easiest for
  testing; multi-device (`left right dongle`) fans out into tmux `buildsession` panes.
- **Dongle link depends on `keyboard/` sources.** `right/src/slave_drivers/kboot_driver.c` is compiled
  for all UHK80 targets, but `device/src/CMakeLists.txt` only compiles `keyboard/*.c` (which includes
  `uart_modules.c`) for left/right, **not** the dongle. So any symbol `kboot_driver.c` pulls from
  `uart_modules.c` (e.g. the `KbootUart_*` UART-transport functions) must be stubbed/guarded for the
  dongle or the dongle link fails with `undefined reference`.
- **Agent npm shadow.** `lib/agent` is nested inside this repo; the firmware root's `node_modules/npm`
  (11.8.0, pinned by the root lockfile) shadows nvm's npm during the Agent build and trips its
  `check-node-version` (>=11.13.0). Fix: `npm install npm@11.13.0 --no-save --no-package-lock` in the
  firmware root. Details in the reference doc above.
