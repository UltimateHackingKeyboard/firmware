#ifndef __OLED_DISPLAY_H__
#define __OLED_DISPLAY_H__

// Macros:

    #define DISPLAY_WIDTH 256
    #define DISPLAY_HEIGHT 64

    #define DISPLAY_SHIFTING_MARGIN 4
    #define DISPLAY_SHIFTING_PERIOD 180000

    #define DISPLAY_MIN_GAP_TO_SKIP 8

    #define DISPLAY_USABLE_WIDTH (DISPLAY_WIDTH-DISPLAY_SHIFTING_MARGIN)
    #define DISPLAY_USABLE_HEIGHT (DISPLAY_HEIGHT-DISPLAY_SHIFTING_MARGIN)

// Typedefs:

typedef enum {
    OledCommand_SetDisplayOn = 0xaf,
    OledCommand_SetContrast = 0x81,
    OledCommand_SetRowAddress = 0xb0,
    OledCommand_SetColumnLow = 0x00,
    OledCommand_SetColumnHigh = 0x10,
    OledCommand_SetScanDirectionDown = 0xc8,
} oled_commands_t;

#endif // __OLED_DISPLAY_H__
