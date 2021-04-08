#ifndef __I2C_ERROR_LOGGER_H__
#define __I2C_ERROR_LOGGER_H__

// Includes:

    #include "fsl_common.h"
    #include "slave_scheduler.h"

// Macros:

    #define MAX_LOGGED_I2C_ERROR_TYPES_PER_SLAVE 7

// Typedefs:

    typedef struct {
        status_t status;
        uint32_t count;
    } i2c_error_count_t;

    typedef struct {
        uint8_t errorTypeCount;
        i2c_error_count_t errors[MAX_LOGGED_I2C_ERROR_TYPES_PER_SLAVE];
    } i2c_slave_error_counter_t;

// Variables:

    extern i2c_slave_error_counter_t I2cSlaveErrorCounters[SLAVE_COUNT];

// Functions:

    void LogI2cError(uint8_t slaveId, status_t status);

#endif
