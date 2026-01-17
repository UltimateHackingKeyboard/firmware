# Crash logs - users

## Uhk80

Crash logs have to be enabled via `set devMode true` in `$onInit`

## Uhk60

Crash logs are enabled by default. However, `set devMode true` is suggested as it enables logging in cases of ESD events.

(We don't log esd reboots because they are often and these crash logs are significantly more disturbing than a quick reboot.)

# Developer notes

In order to fit all our uart applications onto two controllers, we use the async uart driver. Unfortunatelly, the async serial backend is significantly less reliable than the default interrupt backend. Especially it is not available at all during early boot phases. When these log are needed, the interrupt driver has to be used:

- Comment out pin wiring stuff in device/src/main.c.

- Comment out this in uhk-80-right.conf:

```
# CONFIG_SHELL_BACKEND_SERIAL_API_ASYNC=y
```

