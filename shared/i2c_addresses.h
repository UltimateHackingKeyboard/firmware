#ifndef __I2C_H__
#define __I2C_H__

// 7-bit I2C addresses

#define I2C_ADDRESS_LEFT_KEYBOARD_HALF 8

// IS31FL3731 LED drivers range from 0x74 to 0x77
#define I2C_ADDRESS_LED_DRIVER_LEFT  0b1110100
#define I2C_ADDRESS_LED_DRIVER_RIGHT 0b1110111

#define IS_I2C_LED_DRIVER_ADDRESS (address) \
    (I2C_ADDRESS_LED_DRIVER_LEFT <= address && address <= I2C_ADDRESS_LED_DRIVER_RIGHT)

#endif
