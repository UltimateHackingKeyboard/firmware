#ifndef __I2C_H__
#define __I2C_H__

// Macros:

    #define I2C_BUS_BASEADDR  I2C0
    #define I2C_BUS_CLK_SRC   I2C0_CLK_SRC
    #define I2C_BUS_BAUD_RATE 100000 // 100 kHz works even with a 20 meter long bridge cable.

#ifdef CPU_MKL17Z32VFM4
    #define I2C_BUS_MUX       kPORT_MuxAlt2

    #define I2C_BUS_SDA_PORT  PORTB
    #define I2C_BUS_SDA_CLOCK kCLOCK_PortB
    #define I2C_BUS_SDA_PIN   1

    #define I2C_BUS_SCL_PORT  PORTB
    #define I2C_BUS_SCL_CLOCK kCLOCK_PortB
    #define I2C_BUS_SCL_PIN   0
#else
    #define I2C_BUS_MUX       kPORT_MuxAlt2

    #define I2C_BUS_SDA_PORT  PORTB
    #define I2C_BUS_SDA_CLOCK kCLOCK_PortB
    #define I2C_BUS_SDA_PIN   4

    #define I2C_BUS_SCL_PORT  PORTB
    #define I2C_BUS_SCL_CLOCK kCLOCK_PortB
    #define I2C_BUS_SCL_PIN   3
#endif

#endif
