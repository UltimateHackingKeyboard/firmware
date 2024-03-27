#include "storage.h"
#include "zephyr/storage/flash_map.h"

uint8_t Storage_LaunchTransfer(eeprom_operation_t operation, config_buffer_id_t config_buffer_id, void (*successCallback))
{
    const struct flash_area *my_area;
    int err = flash_area_open(FIXED_PARTITION_ID(user_config_partition), &my_area);
}
