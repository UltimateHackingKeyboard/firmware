#ifndef __BOOTLOADER_H__
#define __BOOTLOADER_H__

#include "command.h"

enum _bootloader_status {
    kStatus_UnknownCommand = MAKE_STATUS(kStatusGroup_Bootloader, 0),
    kStatus_SecurityViolation = MAKE_STATUS(kStatusGroup_Bootloader, 1),
    kStatus_AbortDataPhase = MAKE_STATUS(kStatusGroup_Bootloader, 2),
    kStatus_Ping = MAKE_STATUS(kStatusGroup_Bootloader, 3),
    kStatus_NoResponse = MAKE_STATUS(kStatusGroup_Bootloader, 4),
    kStatus_NoResponseExpected = MAKE_STATUS(kStatusGroup_Bootloader, 5),
};

typedef struct _bootloaderContext {
    const peripheral_descriptor_t *activePeripheral;
} bootloader_context_t;

extern bootloader_context_t g_bootloaderContext;

#endif
