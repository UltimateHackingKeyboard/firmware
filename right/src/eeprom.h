#ifndef __EEPROM_H__
#define __EEPROM_H__

// Includes:

    #include "config_parser/config_globals.h"

// Macros:

    #define EEPROM_SIZE (32*1024)
    #define HARDWARE_CONFIG_SIZE 64
    #define USER_CONFIG_SIZE (EEPROM_SIZE - HARDWARE_CONFIG_SIZE)

    #define EEPROM_ADDRESS_LENGTH 2
    #define EEPROM_PAGE_SIZE 64
    #define EEPROM_BUFFER_SIZE (EEPROM_ADDRESS_LENGTH + EEPROM_PAGE_SIZE)

// Typedefs:

    typedef enum {
        EepromOperation_Read,
        EepromOperation_Write,
    } eeprom_operation_t;

// Variables:

    extern bool IsEepromBusy;
    extern status_t EepromTransferStatus;

// Functions:

    void EEPROM_Init(void);
    status_t EEPROM_LaunchTransfer(eeprom_operation_t operation, config_buffer_id_t config_buffer_id, void (*successCallback));

#endif
