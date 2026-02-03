# Communication Latencies

## Bus Latencies

(These were measured by a round trip test.)

- **I2C (UHK 60):** ~10ms. (On uhk60, i2c drives all modules and their led drivers over a single bus.)
- **UART (UHK 80 bridge cable):** ~2.5ms.
- **BLE:** The BLE minimum connection interval is 7.5ms, but Bluetooth protocol overhead (multiple protocol layers, internal queuing, framing) means the nearest communication window is not always hit. Real measured latencies range from 6ms to 16ms.

## End-to-End Latencies

### UHK 80

| Path | Typical Latency |
|------|-----------------|
| Right half keys (wired) | ~2ms |
| Left half keys (wired) | ~4ms |
| Right modules (wired) | ~10ms |
| Wireless (any) | 14-30ms |

### UHK 60

| Path | Typical Latency |
|------|-----------------|
| Right half keys | ~2ms |
| Left half keys | ~7ms |
| Right modules | ~15ms |

(These are more of eyeballed estimates.)

## Polling Rate

The main argument for 1000Hz polling (vs. the original 250Hz) is smoother mouse key movement. This is currently moot for modules, as their scanning rate is lower (~10ms for most).
