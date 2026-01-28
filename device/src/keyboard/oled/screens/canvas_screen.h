#ifndef __CANVAS_SCREEN_H__
#define __CANVAS_SCREEN_H__

// Includes:

    #include <inttypes.h>
    #include <stdbool.h>
    #include "../widgets/widget.h"

// Macros:

    #define CANVAS_TIMEOUT 60000

// Typedefs:

// Variables:

    extern widget_t* CanvasScreen;

// Functions:

    void CanvasScreen_Init();
    void CanvasScreen_Draw(uint16_t x, uint16_t y, uint8_t* data, uint16_t len);
    void CanvasScreen_DrawPacked(uint16_t x, uint16_t y, uint8_t* data, uint16_t pixelCount);

#endif
