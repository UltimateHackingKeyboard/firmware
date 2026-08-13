Compile and flash the firmware with debug symbols:

```
export DEBUG=1
./build.sh rightv2 build flash
```

Start gdb server against UHK60:

```
/opt/SEGGER/JLink/JLinkGDBServerExe
```

Start gdb client against UHK60:

```
cd firmware/right/build/v2-debug
cd $PWD; /opt/arm-gnu-toolchain-15.2.rel1-x86_64-arm-none-eabi/bin/arm-none-eabi-gdb -tui -ex "target remote 127.0.0.1:2331" *.elf
```



