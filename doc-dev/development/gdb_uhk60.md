Compile and flash the firmware with debug symbols:

```
export DEBUG=1
cd firmware/right/uhk60v2
make clean
make flash
```

Start gdb server against UHK60:

```
/opt/SEGGER/JLink/JLinkGDBServerExe
```

Start gdb client against UHK60:

```
cd firmware/right/uhk60v2/build_make
/opt/arm-gnu-toolchain-11.3.rel1-x86_64-arm-none-eabi/bin/arm-none-eabi-gdb -tui -ex "target remote 127.0.0.1:2331" *.axf
```



