#ifndef __CONTEXT_H__
#define __CONTEXT_H__

#include "command.h"

typedef struct _bootloaderContext {
    const peripheral_descriptor_t *activePeripheral;
} bootloader_context_t;

extern bootloader_context_t g_bootloaderContext;

#endif
