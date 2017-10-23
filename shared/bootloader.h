#ifndef __BOOTLOADER_H__
#define __BOOTLOADER_H__

// Includes:

    #include <stdint.h>
    #include <stddef.h>
    #include "attributes.h"
    #include "i2c_addresses.h"

// Macros:

    #define BOOTLOADER_TAG 0x6766636B
    #define BOOTLOADER_TIMEOUT_MS 100
    #define CLOCK_FLAG_HIGH_SPEED_MODE 0x01

    #define DEFINE_BOOTLOADER_CONFIG_AREA(address) \
        const ATTR_BOOTLOADER_CONFIG bootloader_config_t BootloaderConfig = { \
            .tag = BOOTLOADER_TAG, \
            .enabledPeripherals = EnabledBootloaderPeripherial_I2c, \
            .i2cSlaveAddress = address, \
            .peripheralDetectionTimeoutMs = BOOTLOADER_TIMEOUT_MS, \
            .clockFlags = CLOCK_FLAG_HIGH_SPEED_MODE, \
            .clockDivider = ~0 \
    };

// Typedefs:

    typedef enum {
        EnabledBootloaderPeripherial_Uart   = (1<<0),
        EnabledBootloaderPeripherial_I2c    = (1<<1),
        EnabledBootloaderPeripherial_Spi    = (1<<2),
        EnabledBootloaderPeripherial_Can    = (1<<3),
        EnabledBootloaderPeripherial_UsbHid = (1<<4),
        EnabledBootloaderPeripherial_UsbMsc = (1<<7),
    } enabled_bootloader_peripheral_t;

    typedef struct {
        uint32_t tag;                          // Magic number to verify bootloader configuration is valid. Must be set to 'kcfg'.
        uint32_t reserved[3];
        uint8_t enabledPeripherals;            // Bitfield of peripherals to enable.
                                               // bit 0 - LPUART, bit 1 - I2C, bit 2 - SPI, bit 3 - CAN, bit 4 - USB
        uint8_t i2cSlaveAddress;               // If not 0xFF, used as the 7-bit I2C slave address.
                                               // If 0xFF, defaults to 0x10 for I2C slave address.
        uint16_t peripheralDetectionTimeoutMs; // Timeout in milliseconds for active peripheral detection.
                                               // If 0xFFFF, defaults to 5 seconds.
        uint16_t reserved2[2];
        uint32_t reserved3;
        uint8_t clockFlags;   // The flags in the clockFlags configuration field are enabled if the corresponding bit is cleared (0).
                              // bit 0 - HighSpeed Enable high speed mode (i.e., 48 MHz).
        uint8_t clockDivider; // Inverted value of the divider to use for core and bus clocks when in high speed mode.
    } bootloader_config_t;

// Functions:

    void JumpToBootloader(void);

#endif
