#ifndef __I2C_ADDRESSES_H__
#define __I2C_ADDRESSES_H__

// Includes:

    #include "fsl_gpio.h"

// Macros:

    // 7-bit I2C addresses - see http://www.i2c-bus.org/addressing/

    // General call / Start byte           0b0000000
    // CBUS address                        0b0000001
    // Reserved for different bus formats  0b0000010
    // Reserved for future purposes        0b0000011
    // High-Speed master code              0b00001XX
    #define I2C_ADDRESS_LEFT_KEYBOARD_HALF 0b0001000
    #define I2C_ADDRESS_EEPROM             0b1010000
    #define I2C_ADDRESS_LED_DRIVER_LEFT    0b1110100
    // LED driver / touchpad               0b1110101
    // LED driver / touchpad               0b1110110
    #define I2C_ADDRESS_LED_DRIVER_RIGHT   0b1110111
    // Touchpad                            0b00001XX
    // 10-bit slave addressing             0b11110XX
    // Reserved for future purposes        0b11111XX

    #define IS_I2C_LED_DRIVER_ADDRESS(address) \
        (I2C_ADDRESS_LED_DRIVER_LEFT <= (address) && (address) <= I2C_ADDRESS_LED_DRIVER_RIGHT)

#endif
