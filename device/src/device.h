#ifndef __DEVICE_H__
#define __DEVICE_H__

// Includes:

    #include "autoconf.h"

// Macros:

    #define DEVICE_ID CONFIG_DEVICE_ID
    #define DEVICE_COUNT 5

    #define DEVICE_ID_UHK60V1_RIGHT 1
    #define DEVICE_ID_UHK60V2_RIGHT 2
    #define DEVICE_ID_UHK80_LEFT 3
    #define DEVICE_ID_UHK80_RIGHT 4
    #define DEVICE_ID_UHK_DONGLE 5

    #define DEVICE_IS_UHK60V1_RIGHT (CONFIG_DEVICE_ID == DEVICE_ID_UHK60V1_RIGHT)
    #define DEVICE_IS_UHK60V2_RIGHT (CONFIG_DEVICE_ID == DEVICE_ID_UHK60V2_RIGHT)
    #define DEVICE_IS_UHK80_LEFT (CONFIG_DEVICE_ID == DEVICE_ID_UHK80_LEFT)
    #define DEVICE_IS_UHK80_RIGHT (CONFIG_DEVICE_ID == DEVICE_ID_UHK80_RIGHT)
    #define DEVICE_IS_UHK_DONGLE (CONFIG_DEVICE_ID == DEVICE_ID_UHK_DONGLE)

    #define DEVICE_IS_UHK80 (DEVICE_IS_UHK80_LEFT || DEVICE_IS_UHK80_RIGHT)
    #define DEVICE_IS_UHK60 (DEVICE_IS_UHK60V1_RIGHT || DEVICE_IS_UHK60V2_RIGHT)
    #define DEVICE_IS_MASTER (DEVICE_IS_UHK80_RIGHT || DEVICE_IS_UHK60V1_RIGHT || DEVICE_IS_UHK60V2_RIGHT)

    #define DEVICE_NAME CONFIG_BT_DIS_MODEL
    #if DEVICE_IS_UHK60V1_RIGHT
        #define KEY_MATRIX_ROWS 5
        #define KEY_MATRIX_COLS 7
    #elif DEVICE_IS_UHK60V2_RIGHT
        #define KEY_MATRIX_ROWS 5
        #define KEY_MATRIX_COLS 7
    #elif DEVICE_IS_UHK80_LEFT
        #define KEY_MATRIX_ROWS 6
        #define KEY_MATRIX_COLS 7
    #elif DEVICE_IS_UHK80_RIGHT
        #define KEY_MATRIX_ROWS 6
        #define KEY_MATRIX_COLS 10
    #elif DEVICE_IS_UHK_DONGLE
        #define KEY_MATRIX_ROWS 6
        #define KEY_MATRIX_COLS 10
    #endif

    #if DEVICE_IS_UHK80_LEFT
        #define DEVICE_HAS_MERGE_SENSE
    #endif

    #if DEVICE_IS_UHK80_RIGHT
        #define DEVICE_HAS_OLED
    #endif

    #if DEVICE_IS_UHK80_LEFT || DEVICE_IS_UHK80_RIGHT
        #define DEVICE_IS_KEYBOARD
    #endif

    #if !DEVICE_IS_UHK60V1_RIGHT && !DEVICE_IS_UHK60V1_RIGHT
        #define DEVICE_HAS_NRF
    #endif

    #if defined(DEVICE_HAS_NRF) && CONFIG_DEVICE_ID != DEVICE_ID_UHK_DONGLE
        #define DEVICE_HAS_BATTERY
    #endif

// Typedefs

    typedef enum {
        DeviceId_Uhk60v1_Right = DEVICE_ID_UHK60V1_RIGHT,
        DeviceId_Uhk60v2_Right = DEVICE_ID_UHK60V2_RIGHT,
        DeviceId_Uhk80_Right = DEVICE_ID_UHK80_RIGHT,
        DeviceId_Uhk80_Left = DEVICE_ID_UHK80_LEFT,
        DeviceId_Uhk_Dongle = DEVICE_ID_UHK_DONGLE,
    } device_id_t;

#endif // __DEVICE_H__
