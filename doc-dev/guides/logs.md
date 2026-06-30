# Uhk logs

Special guides:

- [capturing log when usb doesn't work](./logs_c2usb.md)
- [capturing crash logs](./crash_logs.md)

## Accessing firmware logs via Agent

- in Agent, enable Advanced Settings:
    - Go to About page.
    - Click the Agent icon 7 times.
    - Optionally, go to Settings and check ‘Always enable advanced settings’.
    - Scroll the left pane up and click Advanced Settings.
- when in Advanced settings, click logging and check the Right half checkbox
- if relevant, check any other devices that you want/need logs from

## Interactive features

In case of Uhk80, the shell connects to the interactive zephyr shell. In case of uhk60, it connects to our own non-interactive logging buffer.

Bluetooth:

- `uhk connections` in the zephyr shell (the logging thing in Advanced settings)
- `set bluetooth.minAdvertisingDelay 2000` in a macro named `$onInit`, in case you are unable to use the shell due to too fast reconnection loops.

Logging controls:

- `log status` - lists logging modules
- `log enable {dbg|inf|wrn|err} <module>` - changes logging level for the module`
- `uhk log usbLog 1` - enables usb logging. If you want the logs to be always saved in uhk's buffer, add following command into a macro named `$onInit`: `zephyr uhk log usbLog 1`

# Agent logs

You can find Agent logs in the following path:

- on Linux: `~/.config/uhk-agent/uhk-agent.log`
- on macOS: `~/Library/Logs/uhk-agent/uhk-agent.log`
- on Windows: `%USERPROFILE%\AppData\Roaming\uhk-agent/uhk-agent.log`

Depending on the issue, you may need to enable additional logging by passing additional commandline arguments. For communication related issues, it is `--log=usb`, valid values are `config | misc | usb | usbOps | all`. Furthermore, you may need to prefix them with multiple groups of `--`, exact count depending on your system and exact Agent build. Resulting command may look as follows:

```
./UHK.Agent-9.0.0-linux-x86_64.AppImage -- -- --log=misc,config,usb
```



