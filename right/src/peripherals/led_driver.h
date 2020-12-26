#ifndef __LED_DRIVER_REGISTER_H__
#define __LED_DRIVER_REGISTER_H__

// Includes:

    #include "fsl_gpio.h"
    #include "fsl_port.h"
    #include "fsl_i2c.h"
    #include "i2c.h"
    #include "i2c_addresses.h"

// Macros:

    #define LED_DRIVER_SDB_PORT  PORTA
    #define LED_DRIVER_SDB_GPIO  GPIOA
    #define LED_DRIVER_SDB_CLOCK kCLOCK_PortA
    #define LED_DRIVER_SDB_PIN   2

    #define LED_DRIVER_IICRST_PORT  PORTA
    #define LED_DRIVER_IICRST_GPIO  GPIOA
    #define LED_DRIVER_IICRST_CLOCK kCLOCK_PortA
    #define LED_DRIVER_IICRST_PIN   4

    #define LED_DRIVER_REGISTER_CONFIGURATION 0x00
    #define LED_DRIVER_REGISTER_GLOBAL_CURRENT 0x01
    #define LED_DRIVER_REGISTER_SHUTDOWN 0x0A
    #define LED_DRIVER_REGISTER_FRAME 0xFD
    #define LED_DRIVER_REGISTER_WRITE_LOCK 0xFE

    #define LED_DRIVER_WRITE_LOCK_DISABLE 0x00
    #define LED_DRIVER_WRITE_LOCK_ENABLE_ONCE 0xC5

    #define LED_DRIVER_FRAME_1 0
    #define LED_DRIVER_FRAME_2 1
    #define LED_DRIVER_FRAME_3 2
    #define LED_DRIVER_FRAME_4 3
    #define LED_DRIVER_FRAME_5 4
    #define LED_DRIVER_FRAME_6 5
    #define LED_DRIVER_FRAME_7 6
    #define LED_DRIVER_FRAME_8 7
    #define LED_DRIVER_FRAME_FUNCTION 0x0B

    #define LED_DRIVER_LED_COUNT_IS31FL3199 (9)
    #define LED_DRIVER_LED_COUNT_IS31FL3731 (2*8*9)
    #define LED_DRIVER_LED_COUNT_IS31FL3737 (12*16)
    #define LED_DRIVER_LED_COUNT_MAX \
        MAX( \
            LED_DRIVER_LED_COUNT_IS31FL3199, \
            MAX( \
                LED_DRIVER_LED_COUNT_IS31FL3731, \
                LED_DRIVER_LED_COUNT_IS31FL3737) \
        )

    #define FRAME_REGISTER_LED_CONTROL_FIRST   0x00
    #define FRAME_REGISTER_LED_CONTROL_LAST    0x11
    #define FRAME_REGISTER_BLINK_CONTROL_FIRST 0x12
    #define FRAME_REGISTER_BLINK_CONTROL_LAST  0x23
    #define FRAME_REGISTER_PWM_FIRST_IS31FL3199 0x07
    #define FRAME_REGISTER_PWM_FIRST_IS31FL3731 0x24
    #define FRAME_REGISTER_PWM_FIRST_IS31FL3737 0x00
    #define FRAME_REGISTER_PWM_LAST            0xB3

    #define SHUTDOWN_MODE_SHUTDOWN 0
    #define SHUTDOWN_MODE_NORMAL   1

// Functions:

    void InitLedDriver(void);

#endif
