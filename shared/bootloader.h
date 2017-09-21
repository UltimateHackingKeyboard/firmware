#ifndef __KBOOT_H__
#define __KBOOT_H__

// Includes:

    #include <stdint.h>
    #include <stddef.h>

// Macros:

    #define BOOTLOADER_TAG 0x6766636B
    #define BOOTLOADER_TIMEOUT_MS 100

    // bits for bootloader_config_t.enabledPeripherals
    #define ENABLE_PERIPHERAL_UART     (1<<0)
    #define ENABLE_PERIPHERAL_I2C      (1<<1)
    #define ENABLE_PERIPHERAL_SPI      (1<<2)
    #define ENABLE_PERIPHERAL_CAN      (1<<3)
    #define ENABLE_PERIPHERAL_USB_HID  (1<<4)
    #define ENABLE_PERIPHERAL_USB_MSC  (1<<7)

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

    extern void JumpToKboot(void);

#endif
