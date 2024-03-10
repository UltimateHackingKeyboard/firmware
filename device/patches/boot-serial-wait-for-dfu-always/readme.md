Adds the BOOT_SERIAL_WAIT_FOR_DFU_ALWAYS MCUboot Kconfig option to enter serial recovery
for the duration of BOOT_SERIAL_WAIT_FOR_DFU_TIMEOUT upon physically resetting the board.
Otherwise, MCUboot immediately executes the application.
