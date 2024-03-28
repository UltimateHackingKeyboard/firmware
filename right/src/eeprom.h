#ifndef __EEPROM_H__
#define __EEPROM_H__

// Includes:

    #include "storage.h"
    #include "config_parser/config_globals.h"

// Macros:

    #define EEPROM_SIZE (32*1024)
    #define EEPROM_ADDRESS_SIZE 2
    #define EEPROM_PAGE_SIZE 64
    #define EEPROM_BUFFER_SIZE (EEPROM_ADDRESS_SIZE + EEPROM_PAGE_SIZE)

    #define HARDWARE_CONFIG_SIZE 64
    #define USER_CONFIG_SIZE (EEPROM_SIZE - HARDWARE_CONFIG_SIZE)

// Functions:

#ifndef __ZEPHYR__
    void EEPROM_Init(void);
    status_t EEPROM_LaunchTransfer(storage_operation_t operation, config_buffer_id_t config_buffer_id, void (*successCallback));
#endif

#endif
