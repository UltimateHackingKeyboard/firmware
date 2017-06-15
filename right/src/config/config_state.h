#ifndef __CONFIG_BUFFER_H__
#define __CONFIG_BUFFER_H__

// Includes:

    #include "fsl_common.h"

// Macros:

    #define EEPROM_SIZE (32*1024)

// Variables:

    extern uint8_t ConfigBuffer[EEPROM_SIZE];
    extern uint8_t *ConfigPtr;

#endif
