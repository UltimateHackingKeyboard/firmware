#include "i2c_error_logger.h"

i2c_slave_error_counter_t I2cSlaveErrorCounters[MAX_SLAVE_COUNT];

void LogI2cError(uint8_t slaveId, status_t status)
{
    i2c_slave_error_counter_t *i2cSlaveErrorCounter = I2cSlaveErrorCounters + slaveId;
    uint8_t errorIdx;

    for (errorIdx=0; errorIdx<i2cSlaveErrorCounter->errorTypeCount; errorIdx++) {
        i2c_error_count_t *currentI2cError = i2cSlaveErrorCounter->errors + errorIdx;
        if (currentI2cError->status == status) {
            currentI2cError->count++;
            break;
        }
    }

    if (errorIdx == i2cSlaveErrorCounter->errorTypeCount && errorIdx < MAX_LOGGED_I2C_ERROR_TYPES_PER_SLAVE) {
        i2cSlaveErrorCounter->errorTypeCount++;
        i2c_error_count_t *currentI2cError = i2cSlaveErrorCounter->errors + errorIdx;
        currentI2cError->status = status;
        currentI2cError->count = 1;
    }
}
