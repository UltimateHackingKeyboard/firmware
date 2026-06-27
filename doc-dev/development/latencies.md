# Communication Latencies

## Bus Latencies

- **I2C (UHK 60):** ~8ms. (On uhk60, i2c drives all modules and their led drivers over a single bus.)
- **UART (UHK 80 bridge cable):** ~2.5ms.
- **BLE (UHK 80 left-to-right and right-to-dongle):** The BLE minimum connection interval is 7.5ms, but Bluetooth protocol overhead (multiple protocol layers, internal queuing, framing) means the nearest communication window is not always hit. Real measured latencies range from 6ms to 16ms.
- **Third party sensors (trackball, trackpoint, touchpad):** usually around 10ms scanning rate, so average latency would be 5ms.

## End-to-End Latencies

### UHK 80

| Path | Typical Average Latency |
|------|-----------------|
| Right half keys (wired) | ~2ms |
| Left half keys (wired) | ~4ms |
| Right modules (wired) | ~8ms |
| Wireless (left-right-dongle) | 14-32ms |

### UHK 60

| Path | Typical Average Latency |
|------|-----------------|
| Right half keys | ~2ms |
| Left half keys | ~8ms |
| Right modules | ~15ms |

## Polling Rate

| Device | Polling rate |
|------|-----------------|
| UHK 60 (Usb) | 1000Hz |
| UHK 80 (Usb) | 1000Hz |
| UHK Dongle (Usb)| 1000Hz |

The main argument for 1000Hz polling (vs. the original 250Hz) is smoother mouse key movement. This is currently moot for modules, as their scanning rate is lower.
