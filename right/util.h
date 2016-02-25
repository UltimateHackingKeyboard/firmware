#ifndef __UTIL_H__
#define __UTIL_H__

// Macros:

    #define GET_MSB_OF_WORD(word) ((uint8_t)(word >> 8))
    #define GET_LSB_OF_WORD(word) ((uint8_t)(word & 0xff))

#endif
