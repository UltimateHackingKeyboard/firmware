# How to gather logs from uhk whose usb doesn't work

Put this into a macro named `$onInit`:

```
set devMode true
zephyr uhk logs 1
zephyr log enable wrn Battery
zephyr log enable wrn Bt
zephyr log enable wrn StateSync
zephyr log enable inf udc
zephyr log enable wrn udc_nrf
zephyr log enable inf hogp
zephyr log enable dbg c2usb
```

Adjust log levels as needed. 

You can see the logs in real time:

- on the oled display
- in Agent's advanced settings (click the Agent button 7x on the about page and scroll up)

Create this macro and bind it somewhere convenient:

```
zephyr uhk snaplog
```

Reproduce the issue. 

Run the `snaplog` macro. This will export the usb log buffer into the persistent status buffer.

Bring usb back to life. If need be, restart uhk using its reset button, but do not powercycle it.

Start Agent. 

Yellow bar should pop up with the zephyr your log.
