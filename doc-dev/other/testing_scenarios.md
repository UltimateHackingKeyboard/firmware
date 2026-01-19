Following is a list testing suggestions. Not all have to be tested each time.

## Bluetooth:

- Pair ble hid. Unpair. Pair again. Save it into connections. Make sure the Agent doesn't offer saving the same device multiple times.
- When two ble hid devices are paired, test that we can `switchHost` between them. 
- When a device is paired, check that we can disconnect it and connect it again - i.e., that the bonding information didn't get corrupted. (Optionally, throw in uhk restart, bluetooth restart, test this with multiple devices.)
- Test that we can switch between two dongles and that their leds light up accordingly.
- Issue `switchHost device` against a device that is not present. `switchHost` to current usb. See that uhk now advertises hid (pairing icon) and that a ble hid host can connect. 
- Test that we can add a paired device into the host connections. Try pair and add multiple devices within one Agent session.
- Unpair+pair dongle. (Optionally, let a bluetooth hid active while doing so.)
- Pair two ble hids using `bluetooth pair` while all devices are active.

Known issues:
- Oob pairing (of the dongle) fails when another device is connected, including a dongle that is *not* connected over usb.

## Usb suspend:

In all scenarios make sure that uhk works afterwards. 

Test with a pc and with a mac separately.

- suspend and wake the pc when connected via usb, via pc's button(s).
- suspend and wake the pc when connected via usb, by tapping any key.
- suspend and wake the pc when connected via usb, by tapping the wake key.
- suspend and wake the pc when connected via ble, via pc's button(s).

## Troubleshooting:

- Use the `panic` macro to check that crash logs are generated as expected. (`set devMode true` is needed on uhk80)

## Macros: 

These are obscure scenarios that are not worth to test often, but may be good to test from time to time.

- test `oneShot` modifiers with multiple oneShot keys. When keystrokeDelay = 250ms and oneshotTimeout = 5000, you have to see modifiers release only after the oneShot key is pressed.
- test `overlayLayer` - test that overlaid keys are working as expected, are in the right places and that their neighbours didn't get affected.

- 
