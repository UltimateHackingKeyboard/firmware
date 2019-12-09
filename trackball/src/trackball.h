#ifndef __TRACKBALL_H__
#define __TRACKBALL_H__

// Includes:

    #include "slave_protocol.h"
    #include "fsl_port.h"

// Macros:

    #define TRACKBALL_SHTDWN_PORT PORTA
    #define TRACKBALL_SHTDWN_GPIO GPIOA
    #define TRACKBALL_SHTDWN_IRQ PORTA_IRQn
    #define TRACKBALL_SHTDWN_CLOCK kCLOCK_PortA
    #define TRACKBALL_SHTDWN_PIN 4

    #define TRACKBALL_RIGHT_PORT PORTB
    #define TRACKBALL_RIGHT_GPIO GPIOB
    #define TRACKBALL_RIGHT_IRQ PORTB_IRQn
    #define TRACKBALL_RIGHT_CLOCK kCLOCK_PortB
    #define TRACKBALL_RIGHT_PIN 6

    #define TRACKBALL_UP_PORT PORTA
    #define TRACKBALL_UP_GPIO GPIOA
    #define TRACKBALL_UP_IRQ PORTA_IRQn
    #define TRACKBALL_UP_CLOCK kCLOCK_PortA
    #define TRACKBALL_UP_PIN 3

    #define TRACKBALL_DOWN_PORT PORTA
    #define TRACKBALL_DOWN_GPIO GPIOA
    #define TRACKBALL_DOWN_IRQ PORTA_IRQn
    #define TRACKBALL_DOWN_CLOCK kCLOCK_PortA
    #define TRACKBALL_DOWN_PIN 4

// Variables:

    extern pointer_delta_t Trackball_PointerDelta;

// Functions:

    void Trackball_Init(void);
    void Trackball_Update(void);

#endif
