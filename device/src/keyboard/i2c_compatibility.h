#ifndef __I2C_COMPATIBILITY_H__
#define __I2C_COMPATIBILITY_H__

// Includes:

    #include <stdbool.h>
    #include <stdint.h>
    #include <stddef.h>

// Typedefs:
    typedef int32_t status_t;
    #define MAKE_STATUS(group, code) ((((group)*100) + (code)))

    /*! @brief Direction of master and slave transfers. */
    typedef enum _i2c_direction
    {
        kI2C_Write = 0x0U, /*!< Master transmit to slave. */
        kI2C_Read = 0x1U,  /*!< Master receive from slave. */
    } i2c_direction_t;

    /** I2C - Register Layout Typedef */
    typedef struct { } I2C_Type;

    /*! @brief I2C master handle typedef. */
    typedef struct _i2c_master_handle i2c_master_handle_t;

    /*! @brief I2C master transfer callback typedef. */
    typedef void (*i2c_master_transfer_callback_t)(I2C_Type *base,
                                                   i2c_master_handle_t *handle,
                                                   status_t status,
                                                   void *userData);

    /*! @brief I2C master transfer structure. */
    typedef struct _i2c_master_transfer
    {
        uint32_t flags;            /*!< Transfer flag which controls the transfer. */
        uint8_t slaveAddress;      /*!< 7-bit slave address. */
        i2c_direction_t direction; /*!< Transfer direction, read or write. */
        uint32_t subaddress;       /*!< Sub address. Transferred MSB first. */
        uint8_t subaddressSize;    /*!< Size of command buffer. */
        uint8_t *volatile data;    /*!< Transfer buffer. */
        volatile size_t dataSize;  /*!< Transfer size. */
    } i2c_master_transfer_t;

    /*! @brief I2C master handle structure. */
    struct _i2c_master_handle
    {
        i2c_master_transfer_t transfer;                    /*!< I2C master transfer copy. */
        size_t transferSize;                               /*!< Total bytes to be transferred. */
        uint8_t state;                                     /*!< Transfer state maintained during transfer. */
        i2c_master_transfer_callback_t completionCallback; /*!< Callback function called when transfer finished. */
        void *userData;                                    /*!< Callback parameter passed to callback function. */
    };


    /*! @brief Status group numbers. */
    enum _status_groups
    {
        kStatusGroup_Generic = 0,                 /*!< Group number for generic status codes. */
        kStatusGroup_FLASH = 1,                   /*!< Group number for FLASH status codes. */
        kStatusGroup_LPSPI = 4,                   /*!< Group number for LPSPI status codes. */
        kStatusGroup_FLEXIO_SPI = 5,              /*!< Group number for FLEXIO SPI status codes. */
        kStatusGroup_DSPI = 6,                    /*!< Group number for DSPI status codes. */
        kStatusGroup_FLEXIO_UART = 7,             /*!< Group number for FLEXIO UART status codes. */
        kStatusGroup_FLEXIO_I2C = 8,              /*!< Group number for FLEXIO I2C status codes. */
        kStatusGroup_LPI2C = 9,                   /*!< Group number for LPI2C status codes. */
        kStatusGroup_UART = 10,                   /*!< Group number for UART status codes. */
        kStatusGroup_I2C = 11,                    /*!< Group number for UART status codes. */
        kStatusGroup_LPSCI = 12,                  /*!< Group number for LPSCI status codes. */
        kStatusGroup_LPUART = 13,                 /*!< Group number for LPUART status codes. */
        kStatusGroup_SPI = 14,                    /*!< Group number for SPI status code.*/
        kStatusGroup_XRDC = 15,                   /*!< Group number for XRDC status code.*/
        kStatusGroup_SEMA42 = 16,                 /*!< Group number for SEMA42 status code.*/
        kStatusGroup_SDHC = 17,                   /*!< Group number for SDHC status code */
        kStatusGroup_SDMMC = 18,                  /*!< Group number for SDMMC status code */
        kStatusGroup_SAI = 19,                    /*!< Group number for SAI status code */
        kStatusGroup_MCG = 20,                    /*!< Group number for MCG status codes. */
        kStatusGroup_SCG = 21,                    /*!< Group number for SCG status codes. */
        kStatusGroup_SDSPI = 22,                  /*!< Group number for SDSPI status codes. */
        kStatusGroup_FLEXIO_I2S = 23,             /*!< Group number for FLEXIO I2S status codes */
        kStatusGroup_SDRAMC = 35,                 /*!< Group number for SDRAMC status codes. */
        kStatusGroup_POWER = 39,                  /*!< Group number for POWER status codes. */
        kStatusGroup_ENET = 40,                   /*!< Group number for ENET status codes. */
        kStatusGroup_PHY = 41,                    /*!< Group number for PHY status codes. */
        kStatusGroup_TRGMUX = 42,                 /*!< Group number for TRGMUX status codes. */
        kStatusGroup_SMARTCARD = 43,              /*!< Group number for SMARTCARD status codes. */
        kStatusGroup_LMEM = 44,                   /*!< Group number for LMEM status codes. */
        kStatusGroup_QSPI = 45,                   /*!< Group number for QSPI status codes. */
        kStatusGroup_DMA = 50,                    /*!< Group number for DMA status codes. */
        kStatusGroup_EDMA = 51,                   /*!< Group number for EDMA status codes. */
        kStatusGroup_DMAMGR = 52,                 /*!< Group number for DMAMGR status codes. */
        kStatusGroup_FLEXCAN = 53,                /*!< Group number for FlexCAN status codes. */
        kStatusGroup_LTC = 54,                    /*!< Group number for LTC status codes. */
        kStatusGroup_FLEXIO_CAMERA = 55,          /*!< Group number for FLEXIO CAMERA status codes. */
        kStatusGroup_NOTIFIER = 98,               /*!< Group number for NOTIFIER status codes. */
        kStatusGroup_DebugConsole = 99,           /*!< Group number for debug console status codes. */
        kStatusGroup_ApplicationRangeStart = 100, /*!< Starting number for application groups. */
    };

    /*! @brief Generic status return codes. */
    enum _generic_status
    {
        kStatus_Success = MAKE_STATUS(kStatusGroup_Generic, 0),
        kStatus_Fail = MAKE_STATUS(kStatusGroup_Generic, 1),
    };


    /*! @brief  I2C status return codes. */
    enum _i2c_status
    {
        kStatus_I2C_Busy = MAKE_STATUS(kStatusGroup_I2C, 0),            /*!< I2C is busy with current transfer. */
        kStatus_I2C_Idle = MAKE_STATUS(kStatusGroup_I2C, 1),            /*!< Bus is Idle. */
        kStatus_I2C_Nak = MAKE_STATUS(kStatusGroup_I2C, 2),             /*!< NAK received during transfer. */
        kStatus_I2C_ArbitrationLost = MAKE_STATUS(kStatusGroup_I2C, 3), /*!< Arbitration lost during transfer. */
        kStatus_I2C_Timeout = MAKE_STATUS(kStatusGroup_I2C, 4),         /*!< Wait event timeout. */
    };






#endif
