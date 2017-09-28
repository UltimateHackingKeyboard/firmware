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

    #define LED_DRIVER_REGISTER_SHUTDOWN 0x0A
    #define LED_DRIVER_REGISTER_FRAME    0xFD

    #define LED_DRIVER_FRAME_1 0
    #define LED_DRIVER_FRAME_2 1
    #define LED_DRIVER_FRAME_3 2
    #define LED_DRIVER_FRAME_4 3
    #define LED_DRIVER_FRAME_5 4
    #define LED_DRIVER_FRAME_6 5
    #define LED_DRIVER_FRAME_7 6
    #define LED_DRIVER_FRAME_8 7
    #define LED_DRIVER_FRAME_FUNCTION 0x0B

    #define LED_DRIVER_LED_COUNT (2*8*9)
    #define LED_DRIVER_BUFFER_LENGTH (LED_DRIVER_LED_COUNT + 1)

    #define FRAME_REGISTER_LED_CONTROL_FIRST   0x00
    #define FRAME_REGISTER_LED_CONTROL_LAST    0x11
    #define FRAME_REGISTER_BLINK_CONTROL_FIRST 0x12
    #define FRAME_REGISTER_BLINK_CONTROL_LAST  0x23
    #define FRAME_REGISTER_PWM_FIRST           0x24
    #define FRAME_REGISTER_PWM_LAST            0xB3

    #define SHUTDOWN_MODE_SHUTDOWN 0
    #define SHUTDOWN_MODE_NORMAL   1

// Functions:

    void InitLedDriver(void);

#endif
