#include "bootloader.h"

__attribute__((used, section(".BootloaderConfig"))) const bootloader_config_t BootloaderConfig = {
    .tag = 0x6766636B,                    // Magic Number
    .enabledPeripherals = 0xE2,           // Enabled Peripheral: I2C
    .i2cSlaveAddress = 0x10,              // Use user-defined I2C address
    .peripheralDetectionTimeoutMs = 300,  // Use user-defined timeout (ms)
    .clockFlags = 0xFF,                   // Disable High speed mode
    .clockDivider = 0xFF,                 // Use clock divider (0)
};

void JumpToBootloader(void) {
    uint32_t runBootloaderAddress;
    void (*runBootloader)(void * arg);
    // Read the function address from the ROM API tree.
    runBootloaderAddress = **(uint32_t **)(0x1c00001c);
    runBootloader = (void (*)(void * arg))runBootloaderAddress;
    // Start the bootloader.
    runBootloader(NULL);
}
