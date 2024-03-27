#ifndef __STORAGE_H__
#define __STORAGE_H__

// Includes:

#include "config_parser/config_globals.h"
#include "eeprom.h"

// Functions:

uint8_t Flash_LaunchTransfer(eeprom_operation_t operation, config_buffer_id_t config_buffer_id, void (*successCallback));

#endif // __STORAGE_H__
