#include "flash.h"

const struct flash_area *hardwareConfigArea;
const struct flash_area *userConfigArea;

uint8_t Flash_LaunchTransfer(storage_operation_t operation, config_buffer_id_t configBufferId, void (*successCallback))
{
    const struct flash_area *configArea = configBufferId == ConfigBufferId_HardwareConfig ? hardwareConfigArea : userConfigArea;
    size_t configSize = configBufferId == ConfigBufferId_HardwareConfig ? HARDWARE_CONFIG_SIZE : USER_CONFIG_SIZE;

    if (operation == StorageOperation_Read) {
        flash_area_read(configArea, 0, ConfigBufferIdToConfigBuffer(configBufferId)->buffer, configSize);
    } else if (operation == StorageOperation_Write) {
        flash_area_erase(configArea, 0, configSize);
        flash_area_write(configArea, 0, ConfigBufferIdToConfigBuffer(configBufferId)->buffer, configSize);
    }

    return 0;
}
