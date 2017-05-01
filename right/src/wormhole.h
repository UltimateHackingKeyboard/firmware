#ifndef __WORMHOLE_H__
#define __WORMHOLE_H__

// Includes:

    #include <stdint.h>

// Macros:

    #define WORMHOLE_MAGIC_NUMBER 0x3b04cd9e94521f9a
    #define NO_INIT_GCC __attribute__ ((section (".noinit")))

// Typedefs:

    typedef struct {
      uint64_t magicNumber;
      uint8_t enumerationMode;
    } wormhole_t;

// Variables:

    extern wormhole_t *Wormhole NO_INIT_GCC;

#endif
