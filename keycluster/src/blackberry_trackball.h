#ifndef __BLACKBERRY_TRACKBALL_H__
#define __BLACKBERRY_TRACKBALL_H__

// Includes:

    #include "slave_protocol.h"

// Macros:

    #define BLACKBERRY_TRACKBALL_LEFT_PORT PORTB
    #define BLACKBERRY_TRACKBALL_LEFT_GPIO GPIOB
    #define BLACKBERRY_TRACKBALL_LEFT_IRQ PORTB_IRQn
    #define BLACKBERRY_TRACKBALL_LEFT_CLOCK kCLOCK_PortB
    #define BLACKBERRY_TRACKBALL_LEFT_PIN 6

    #define BLACKBERRY_TRACKBALL_IRQ_HANDLER PORTB_IRQHandler

// Variables:

    extern pointer_delta_t BlackBerryTrackball_PointerDelta;

// Functions:

    void BlackberryTrackball_Init(void);

#endif
