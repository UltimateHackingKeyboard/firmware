#ifndef __I2C_H__
#define __I2C_H__

// Includes:

#ifdef __ZEPHYR__
    #include "keyboard/i2c_compatibility.h"
#else
    #include "fsl_i2c.h"
#endif

    #include "slave_protocol.h"

// Macros:

    // Main bus

    #define I2C_MAIN_BUS_BASEADDR         I2C0
    #define I2C_MAIN_BUS_IRQ_ID           I2C0_IRQn
    #define I2C_MAIN_BUS_CLK_SRC          I2C0_CLK_SRC
    #define I2C_MAIN_BUS_NORMAL_BAUD_RATE 100000
    #define I2C_MAIN_BUS_BUSPAL_BAUD_RATE 30000
    #define I2C_MAIN_BUS_MUX              kPORT_MuxAlt7

    #define I2C_MAIN_BUS_SDA_GPIO  GPIOD
    #define I2C_MAIN_BUS_SDA_PORT  PORTD
    #define I2C_MAIN_BUS_SDA_CLOCK kCLOCK_PortD
    #define I2C_MAIN_BUS_SDA_PIN   3

    #define I2C_MAIN_BUS_SCL_GPIO  GPIOD
    #define I2C_MAIN_BUS_SCL_PORT  PORTD
    #define I2C_MAIN_BUS_SCL_CLOCK kCLOCK_PortD
    #define I2C_MAIN_BUS_SCL_PIN   2

    // EEPROM bus

    #define I2C_EEPROM_BUS_BASEADDR  I2C1
    #define I2C_EEPROM_BUS_IRQ_ID    I2C1_IRQn
    #define I2C_EEPROM_BUS_CLK_SRC   I2C1_CLK_SRC
    #define I2C_EEPROM_BUS_BAUD_RATE 1000000  // 1 Mhz is the maximum speed of the EEPROM.
    #define I2C_EEPROM_BUS_MUX       kPORT_MuxAlt2

    #define I2C_EEPROM_BUS_SDA_GPIO  GPIOC
    #define I2C_EEPROM_BUS_SDA_PORT  PORTC
    #define I2C_EEPROM_BUS_SDA_CLOCK kCLOCK_PortC
    #define I2C_EEPROM_BUS_SDA_PIN   11

    #define I2C_EEPROM_BUS_SCL_GPIO  GPIOC
    #define I2C_EEPROM_BUS_SCL_PORT  PORTC
    #define I2C_EEPROM_BUS_SCL_CLOCK kCLOCK_PortC
    #define I2C_EEPROM_BUS_SCL_PIN   10

// Variables:

    extern i2c_master_handle_t I2cMasterHandle;
    extern volatile uint32_t I2C_Watchdog;

// Functions:

    status_t I2cAsyncWrite(uint8_t i2cAddress, uint8_t *data, size_t dataSize);
    status_t I2cAsyncRead(uint8_t i2cAddress, uint8_t *data, size_t dataSize);
    status_t I2cAsyncWriteMessage(uint8_t i2cAddress, i2c_message_t *message);
    status_t I2cAsyncReadMessage(uint8_t i2cAddress, i2c_message_t *message);

#endif
