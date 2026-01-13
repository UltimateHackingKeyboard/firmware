Things to test:

- Pair ble hid. Unpair. Pair again.
- When two ble hid devices are paired, test that we can `switchHost` between them. 
- When a device is paired, check that we can disconnect it and connect it again - i.e., that the bonding information didn't get corrupted. (Optionally, throw in uhk restart, bluetooth restart, test this with multiple devices.)
- Test that we can switch between two dongles and that their leds light up accordingly.
- Issue `switchHost device` against a device that is not present. `switchHost` to current usb. See that uhk now advertises hid (pairing icon) and that a ble hid host can connect. 
- Test that we can add a paired device into the host connections. Try pair and add multiple devices within one Agent session.
