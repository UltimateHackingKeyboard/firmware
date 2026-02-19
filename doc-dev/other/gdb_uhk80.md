# Debugging UHK80 with VSCode

In terminal:

```
export DEBUG=1
```

Then build your firmware:

```
./build.sh right build
```

Then open vscode. Have the nrf extension installed. Open the nrfsdk.code-workspace vscode workspace.

## Debugging with nrf connect:

In the left nrfConnect pane, you should now see (it is automatically imported from the above build):

```
- Applications
  - device
    - uhk-80-right
      - device (uhk-80-right) 
      - mcuboot (uhk-80-right)
```

Right click `device(uhk-80-right)`

In Actions section, you may now be able to run Debug, flash, build.

Running (Debug) now however gets us into bootloader instead of firmware. This is unsolved for now.

## Debugging without nrf connect extension:

Run:

```
west vscode -d device/build/uhk-80-right --debug uhk80_debug_config
```

This creates a new launch config that you should be able to run from the debug pane.


