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

    #define TRACKBALL_NCS_PORT PORTB
    #define TRACKBALL_NCS_GPIO GPIOB
    #define TRACKBALL_NCS_IRQ PORTB_IRQn
    #define TRACKBALL_NCS_CLOCK kCLOCK_PortB
    #define TRACKBALL_NCS_PIN 1

// Variables:

    extern pointer_delta_t Trackball_PointerDelta;

// Functions:

    void Trackball_Init(void);
    void Trackball_Update(void);

#endif
