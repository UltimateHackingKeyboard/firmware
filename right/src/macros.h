#ifndef __MACROS_H__
#define __MACROS_H__

// Includes:

    #include <stdint.h>

// Macros:

    #define MAX_MACRO_NUM 255

// Typedefs:

    typedef struct {
        uint16_t offset;
        uint16_t macroActionsCount;
    } macro_reference_t;

// Variables:

    extern macro_reference_t AllMacros[MAX_MACRO_NUM];
    extern uint8_t AllMacrosCount;

#endif
