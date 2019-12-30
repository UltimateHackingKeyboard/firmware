#ifndef __BLACKBERRY_TRACKBALL_H__
#define __BLACKBERRY_TRACKBALL_H__

// Includes:

    #include "slave_protocol.h"
    #include "fsl_port.h"

// Macros:

    #define BLACKBERRY_TRACKBALL_LEFT_PORT PORTB
    #define BLACKBERRY_TRACKBALL_LEFT_GPIO GPIOB
    #define BLACKBERRY_TRACKBALL_LEFT_IRQ PORTB_IRQn
    #define BLACKBERRY_TRACKBALL_LEFT_CLOCK kCLOCK_PortB
    #define BLACKBERRY_TRACKBALL_LEFT_PIN 13

    #define BLACKBERRY_TRACKBALL_RIGHT_PORT PORTB
    #define BLACKBERRY_TRACKBALL_RIGHT_GPIO GPIOB
    #define BLACKBERRY_TRACKBALL_RIGHT_IRQ PORTB_IRQn
    #define BLACKBERRY_TRACKBALL_RIGHT_CLOCK kCLOCK_PortB
    #define BLACKBERRY_TRACKBALL_RIGHT_PIN 6

    #define BLACKBERRY_TRACKBALL_UP_PORT PORTA
    #define BLACKBERRY_TRACKBALL_UP_GPIO GPIOA
    #define BLACKBERRY_TRACKBALL_UP_IRQ PORTA_IRQn
    #define BLACKBERRY_TRACKBALL_UP_CLOCK kCLOCK_PortA
    #define BLACKBERRY_TRACKBALL_UP_PIN 3

    #define BLACKBERRY_TRACKBALL_DOWN_PORT PORTA
    #define BLACKBERRY_TRACKBALL_DOWN_GPIO GPIOA
    #define BLACKBERRY_TRACKBALL_DOWN_IRQ PORTA_IRQn
    #define BLACKBERRY_TRACKBALL_DOWN_CLOCK kCLOCK_PortA
    #define BLACKBERRY_TRACKBALL_DOWN_PIN 4

// Functions:

    void BlackberryTrackball_Init(void);
    void BlackberryTrackball_Update(void);

#endif
