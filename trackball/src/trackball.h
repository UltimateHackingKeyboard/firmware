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

    #define TRACKBALL_MOSI_PORT PORTA
    #define TRACKBALL_MOSI_GPIO GPIOA
    #define TRACKBALL_MOSI_IRQ PORTA_IRQn
    #define TRACKBALL_MOSI_CLOCK kCLOCK_PortA
    #define TRACKBALL_MOSI_PIN 7

    #define TRACKBALL_MISO_PORT PORTA
    #define TRACKBALL_MISO_GPIO GPIOA
    #define TRACKBALL_MISO_IRQ PORTA_IRQn
    #define TRACKBALL_MISO_CLOCK kCLOCK_PortA
    #define TRACKBALL_MISO_PIN 6

    #define TRACKBALL_SCK_PORT PORTB
    #define TRACKBALL_SCK_GPIO GPIOB
    #define TRACKBALL_SCK_IRQ PORTB_IRQn
    #define TRACKBALL_SCK_CLOCK kCLOCK_PortB
    #define TRACKBALL_SCK_PIN 0

    #define TRACKBALL_SPI_MASTER SPI0
    #define TRACKBALL_SPI_MASTER_SOURCE_CLOCK kCLOCK_BusClk

// Functions:

    void Trackball_Init(void);

#endif
