#ifndef __SPI_H__
#define __SPI_H__

// Includes:

    #include <zephyr/drivers/spi.h>
    #include <zephyr/kernel.h>

// Macros:

// Variables:

    extern struct k_mutex SpiMutex;

// Functions:

    extern void writeSpi(uint8_t data);
    extern void writeSpi2(uint8_t* data, uint8_t len);
    extern void InitSpi(void);

    // Idle notifications from the LED and OLED drivers. Once both the key backlight and
    // the display are asleep, the bus gets suspended; the next writeSpi resumes it.
    extern void Spi_SetLedsIdle(bool idle);
    extern void Spi_SetDisplayIdle(bool idle);

#endif // __SPI_H__
