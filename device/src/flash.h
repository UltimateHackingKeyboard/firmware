#ifndef __FLASH_H__
#define __FLASH_H__

// Includes:

#include "zephyr/storage/flash_map.h"
#include "config_parser/config_globals.h"
#include "storage.h"

// Macros:

#ifdef __ZEPHYR__
    // Don't mix them with UHK60 definitions as USER_CONFIG_SIZE differs
    #define HARDWARE_CONFIG_SIZE 64
    #define USER_CONFIG_SIZE (32*1024)
#endif

// Variables:

    extern const struct flash_area *hardwareConfigArea;
    extern const struct flash_area *userConfigArea;

// Functions:

    uint8_t Flash_LaunchTransfer(storage_operation_t operation, config_buffer_id_t config_buffer_id, void (*successCallback));
    int Flash_ReadAreaSync(const struct flash_area *fa, off_t off, void *dst, size_t len);
    bool Flash_IsBusy();
    void InitFlash();

#endif // __FLASH_H__
