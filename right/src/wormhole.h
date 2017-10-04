#ifndef __WORMHOLE_H__
#define __WORMHOLE_H__

// Includes:

    #include <stdint.h>
    #include "attributes.h"

// Macros:

    #define WORMHOLE_MAGIC_NUMBER 0x3b04cd9e94521f9a

// Typedefs:

    typedef enum {
        EnumerationMode_Bootloader,
        EnumerationMode_NormalKeyboard,
        EnumerationMode_CompatibleKeyboard,
        EnumerationMode_BusPal,
    } enumeration_mode_t;

    typedef struct {
      uint64_t magicNumber;
      uint8_t enumerationMode;
    } wormhole_t;

// Variables:

    extern wormhole_t ATTR_NO_INIT Wormhole;

#endif
