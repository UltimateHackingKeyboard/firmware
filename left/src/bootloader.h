#ifndef __BOOTLOADER_H__
#define __BOOTLOADER_H__

// Includes:

    #include <stdint.h>
    #include <stddef.h>

// Typedefs:

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

    extern void JumpToBootloader(void);

#endif
