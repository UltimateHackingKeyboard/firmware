# Firmware Flashing Implementation

This directory contains documentation for implementing autonomous module firmware flashing on the UHK right half.

## Goal

Move K-boot protocol implementation from the Agent (desktop tool) to the right half firmware, enabling autonomous module firmware updates without Agent involvement in the flashing process.

## Documentation Files

| File | Description |
|------|-------------|
| [configuration-buffer.md](configuration-buffer.md) | Config buffer APIs and memory layout |
| [usb-communication.md](usb-communication.md) | USB command infrastructure |
| [module-communication.md](module-communication.md) | I2C/UART slave scheduler and module discovery |
| [kboot-protocol.md](kboot-protocol.md) | K-boot protocol details for porting |

## Current Architecture

```
┌─────────────┐      USB        ┌─────────────┐      I2C       ┌─────────────┐
│   Agent     │ ────────────────▶│  Right Half │ ───────────────▶│   Module    │
│  (Desktop)  │                 │  (Buspal)   │                │ (Bootloader)│
└─────────────┘                 └─────────────┘                └─────────────┘
       │                              │                              │
       │  K-boot protocol            │  Forward packets             │
       │  implementation             │  transparently               │
       │                              │                              │
```

**Problem**: K-boot protocol logic is in the Agent. Right half only forwards packets.

## Target Architecture

```
┌─────────────┐      USB        ┌─────────────┐      I2C       ┌─────────────┐
│   Agent     │ ────────────────▶│  Right Half │ ───────────────▶│   Module    │
│  (Desktop)  │                 │  (K-boot)   │                │ (Bootloader)│
└─────────────┘                 └─────────────┘                └─────────────┘
       │                              │                              │
       │  Upload firmware            │  K-boot protocol             │
       │  binary only                │  implementation              │
       │                              │                              │
```

**Goal**: Agent uploads firmware binary. Right half handles K-boot protocol.

## Two-Phase Implementation

### Phase 1: USB Transfer to Right Half

1. Analyze existing configuration buffer APIs (see [configuration-buffer.md](configuration-buffer.md))
2. Design firmware upload protocol
3. Implement chunked USB transfer (similar to config write)
4. Store firmware in staging buffer or stream directly

### Phase 2: Right Half to Module

1. Port K-boot protocol from Agent (see [kboot-protocol.md](kboot-protocol.md))
2. Implement I2C transport layer
3. Implement flashing state machine:
   - Reboot module to bootloader
   - Erase flash
   - Write firmware chunks
   - Verify and reset

## Proposed USB Commands

| Command | ID | Description |
|---------|-----|-------------|
| StartModuleFirmwareUpload | 0x20 | Begin firmware upload (module ID, size) |
| WriteModuleFirmwareChunk | 0x21 | Write firmware data chunk |
| FlashModule | 0x22 | Start flashing (uses uploaded firmware) |
| GetFlashingStatus | 0x23 | Query flashing progress |
| AbortFlashing | 0x24 | Cancel flashing operation |

## Key Challenges

1. **Memory**: Module firmware (~40-60 KB) exceeds config buffer (~32 KB)
   - Solution: Stream directly to module, or use UHK80 external flash

2. **Timing**: I2C transfers while maintaining keyboard responsiveness
   - Solution: Use slave scheduler, low priority for flashing

3. **Error Recovery**: Handle I2C failures, power loss during flash
   - Solution: Verify writes, implement retry logic

4. **State Management**: Track flashing progress across multiple USB commands
   - Solution: Implement state machine with persistent state

## Technical Constraints

- Firmware must fit in available RAM or be streamed
- K-boot protocol requires both I2C and UART support
- Must handle communication failures gracefully
- Module must remain functional if flashing fails

## Success Criteria

- [ ] Firmware can be uploaded to right half via USB
- [ ] Right half can autonomously flash connected module
- [ ] No dependency on Agent for K-boot protocol execution
- [ ] Agent only uploads firmware binary, doesn't manage flashing
