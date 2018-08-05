#ifndef __DEBUG_OVER_SPI_H__
#define __DEBUG_OVER_SPI_H__

// Includes:

    #include "fsl_common.h"
    #include "fsl_port.h"
    #include "fsl_spi.h"

// Macros:

    #define DEBUG_OVER_SPI_MOSI_PORT  PORTA
    #define DEBUG_OVER_SPI_MOSI_GPIO  GPIOA
    #define DEBUG_OVER_SPI_MOSI_CLOCK kCLOCK_PortA
    #define DEBUG_OVER_SPI_MOSI_PIN   7

    #define DEBUG_OVER_SPI_SCK_PORT  PORTB
    #define DEBUG_OVER_SPI_SCK_GPIO  GPIOB
    #define DEBUG_OVER_SPI_SCK_CLOCK kCLOCK_PortB
    #define DEBUG_OVER_SPI_SCK_PIN   0

// Functions:

    void DebugOverSpi_Init(void);
    void DebugOverSpi_Send(uint8_t *tx, uint8_t len);

#endif
