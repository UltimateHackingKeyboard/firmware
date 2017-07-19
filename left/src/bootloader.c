#include "bootloader.h"

/* bits for enabledPeripherals */
#define ENABLE_PERIPHERAL_UART     (1<<0)
#define ENABLE_PERIPHERAL_I2C      (1<<1)
#define ENABLE_PERIPHERAL_SPI      (1<<2)
#define ENABLE_PERIPHERAL_CAN      (1<<3)
#define ENABLE_PERIPHERAL_USB_HID  (1<<4)
#define ENABLE_PERIPHERAL_USB_MSC  (1<<7)

__attribute__((used, section(".BootloaderConfig"))) const bootloader_config_t BootloaderConfig = {
    .tag = 0x6766636B,                    // Magic Number
    .enabledPeripherals = ENABLE_PERIPHERAL_I2C /*0xE2*/,           // Enabled Peripheral: I2C
    .i2cSlaveAddress = 0x10,              // Use user-defined I2C address
    .peripheralDetectionTimeoutMs = 3000,  // Use user-defined timeout (ms)
    .clockFlags = 0xFF,                   // Disable High speed mode
    .clockDivider = 0xFF,                 // Use clock divider (0)
};

void JumpToBootloader(void) {
    uint32_t runBootloaderAddress;
    void (*runBootloader)(void *arg);

    /* Read the function address from the ROM API tree. */
    runBootloaderAddress = **(uint32_t **)(0x1c00001c);
    runBootloader = (void (*)(void * arg))runBootloaderAddress;

    /* Start the bootloader. */
    runBootloader(NULL);
}
