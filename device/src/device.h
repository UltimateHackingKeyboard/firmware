#ifndef __DEVICE_H__
#define __DEVICE_H__

// Includes:

    #include "autoconf.h"

// Macros:

    #define DEVICE_ID_UHK60V1_RIGHT 1
    #define DEVICE_ID_UHK60V2_RIGHT 2
    #define DEVICE_ID_UHK80_LEFT 3
    #define DEVICE_ID_UHK80_RIGHT 4
    #define DEVICE_ID_UHK_DONGLE 5

    #if CONFIG_DEVICE_ID == DEVICE_ID_UHK60V1_RIGHT
        #define DEVICE_NAME "UHK 60 v1"
        #define USB_DEVICE_PRODUCT_ID 0x6122
        #define KEY_MATRIX_ROWS 5
        #define KEY_MATRIX_COLS 7
    #elif CONFIG_DEVICE_ID == DEVICE_ID_UHK60V2_RIGHT
        #define DEVICE_NAME "UHK 60 v2"
        #define USB_DEVICE_PRODUCT_ID 0x6124
        #define KEY_MATRIX_ROWS 5
        #define KEY_MATRIX_COLS 7
    #elif CONFIG_DEVICE_ID == DEVICE_ID_UHK80_LEFT
        #define DEVICE_NAME "UHK 80 left half"
        #define USB_DEVICE_PRODUCT_ID 0xffff // TODO
        #define KEY_MATRIX_ROWS 6
        #define KEY_MATRIX_COLS 7
    #elif CONFIG_DEVICE_ID == DEVICE_ID_UHK80_RIGHT
        #define DEVICE_NAME "UHK 80 right half"
        #define USB_DEVICE_PRODUCT_ID 0xffff // TODO
        #define KEY_MATRIX_ROWS 6
        #define KEY_MATRIX_COLS 10
    #elif CONFIG_DEVICE_ID == DEVICE_ID_DONGLE
        #define DEVICE_NAME "UHK dongle"
        #define USB_DEVICE_PRODUCT_ID 0xffff // TODO
        #define KEY_MATRIX_ROWS 0
        #define KEY_MATRIX_COLS 0
    #endif

    #if CONFIG_DEVICE_ID == DEVICE_ID_UHK80_LEFT
        #define DEVICE_HAS_MERGE_SENSE
    #endif

    #if CONFIG_DEVICE_ID == DEVICE_ID_UHK80_RIGHT
        #define DEVICE_HAS_OLED
    #endif

    #if CONFIG_DEVICE_ID != DEVICE_ID_UHK60V1_RIGHT && CONFIG_DEVICE_ID != DEVICE_ID_UHK60V2_RIGHT
        #define DEVICE_HAS_NRF
    #endif

    #if defined(DEVICE_HAS_NRF) && CONFIG_DEVICE_ID != DEVICE_ID_UHK_DONGLE
        #define DEVICE_HAS_BATTERY
    #endif

#endif // __DEVICE_H__
