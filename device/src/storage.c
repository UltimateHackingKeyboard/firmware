#include "storage.h"
#include "zephyr/storage/flash_map.h"

uint8_t Storage_LaunchTransfer(eeprom_operation_t operation, config_buffer_id_t configBufferId, void (*successCallback))
{
    uint8_t configAreaId = configBufferId == ConfigBufferId_HardwareConfig
        ? FLASH_AREA_ID(hardware_config_partition)
        : FLASH_AREA_ID(user_config_partition);
    const struct flash_area *configArea;
    flash_area_open(configAreaId, &configArea);

    size_t configSize = configBufferId == ConfigBufferId_HardwareConfig ? HARDWARE_CONFIG_SIZE : USER_CONFIG_SIZE;
    if (operation == EepromOperation_Read) {
        flash_area_read(configArea, 0, ConfigBufferIdToConfigBuffer(configBufferId)->buffer, configSize);
    } else if (operation == EepromOperation_Write) {
        flash_area_erase(configArea, 0, configSize);
        flash_area_write(configArea, 0, ConfigBufferIdToConfigBuffer(configBufferId)->buffer, configSize);
    }
    return 0;
}
