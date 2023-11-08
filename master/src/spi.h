#ifndef __SPI_H__
#define __SPI_H__

// Includes:

    #include <zephyr/kernel.h>

// Variables:

    extern struct k_mutex SpiMutex;

// Functions:

    extern void writeSpi(uint8_t data);
    extern void InitSpi(void);

#endif
