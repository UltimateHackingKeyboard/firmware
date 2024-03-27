#ifndef __EEPROM_H__
#define __EEPROM_H__

// Includes:

    #include "config_parser/config_globals.h"
#ifdef __ZEPHYR__
    #include "zephyr/storage/flash_map.h"
#endif

// Macros:

    #define EEPROM_SIZE (32*1024)
    #define HARDWARE_CONFIG_SIZE 64

#ifdef __ZEPHYR__
    #define USER_CONFIG_SIZE EEPROM_SIZE
    // #define USER_CONFIG_SIZE FIXED_PARTITION_SIZE(user_config_partition) // TODO: Bump related uint16_t variables to uint32_t to avoid overflow
#else
    #define USER_CONFIG_SIZE (EEPROM_SIZE - HARDWARE_CONFIG_SIZE)
#endif

    #define EEPROM_ADDRESS_SIZE 2
    #define EEPROM_PAGE_SIZE 64
    #define EEPROM_BUFFER_SIZE (EEPROM_ADDRESS_SIZE + EEPROM_PAGE_SIZE)

// Typedefs:

    typedef enum {
        EepromOperation_Read,
        EepromOperation_Write,
    } storage_operation_t;

// Variables:

    extern volatile bool IsStorageBusy;

// Functions:

#ifndef __ZEPHYR__
    void EEPROM_Init(void);
    status_t EEPROM_LaunchTransfer(storage_operation_t operation, config_buffer_id_t config_buffer_id, void (*successCallback));
#endif
    bool IsEepromOperationValid(storage_operation_t operation);

#endif
