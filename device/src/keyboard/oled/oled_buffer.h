#ifndef __OLED_BUFFER_H__
#define __OLED_BUFFER_H__

// Includes:

    #include <inttypes.h>
    #include <stdbool.h>
    #include "framebuffer.h"

// Macros:


// Typedefs:

// Variables:

    extern framebuffer_t* OledBuffer;

// Functions:

    void OledBuffer_Init();

#endif
