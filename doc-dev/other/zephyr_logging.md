# Uhk logs

## Uhk60

Uhk 60 has no extensive logging in place. See [crash logging](./crash_logs.md) for crash logging configuration.

## Uhk80

- Launch Agent.
- Go to the About page.
- Click the Agent logo seven times.
- Go to the newly visible "Advanced settings" menu.
- Click on the "Zephyr logging" button.
- Check the "Right half" checkbox. If the left half and/or dongle is affected and connected via USB, check them, too.

# Agent logs

You can find Agent logs in the following path:

- on Linux: `~/.config/uhk-agent/uhk-agent.log`
- on macOS: `~/Library/Logs/uhk-agent/uhk-agent.log`
- on Windows: `%USERPROFILE%\AppData\Roaming\uhk-agent/uhk-agent.log`

Depending on the issue, you may need to enable additional logging by passing additional commandline arguments. For communication related issues, it is `--log=usb`, valid values are `config | misc | usb | usbOps | all`. Furthermore, you may need to prefix them with multiple groups of `--`, exact count depending on your system and exact Agent build. Resulting command may look as follows:

```
./UHK.Agent-9.0.0-linux-x86_64.AppImage -- -- --log=misc,config,usb
```



