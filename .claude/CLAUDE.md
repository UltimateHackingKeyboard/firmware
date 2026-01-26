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

1. Add handler function in `right/src/macros/commands.c` (or appropriate file)
2. Add case in `processCommand()` switch (organized by first character)
3. Handle `Macros_DryRun` flag for validation mode
4. Return `MacroResult_Finished` for synchronous operations
5. Document in `doc-dev/reference-manual.md`

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
