#ifndef __OLED_BUFFER_H__
#define __OLED_BUFFER_H__

// Includes:

    #include <inttypes.h>
    #include <stdbool.h>
    #include "framebuffer.h"

// Macros:

    #define DISPLAY_WIDTH 256
    #define DISPLAY_HEIGHT 64

    #define DISPLAY_SHIFTING_MARGIN 4

    #define DISPLAY_USABLE_WIDTH (DISPLAY_WIDTH-DISPLAY_SHIFTING_MARGIN)
    #define DISPLAY_USABLE_HEIGHT (DISPLAY_HEIGHT-DISPLAY_SHIFTING_MARGIN)

// Typedefs:

// Variables:

    extern framebuffer_t* OledBuffer;

// Functions:

    void OledBuffer_Init();

#endif
