#ifndef __SETTINGS_H__
#define __SETTINGS_H__

// Includes

    #include <stdbool.h>
    #include <stdint.h>

// Typedefs:

    typedef enum {
        UartDebugMode_NoDebug = 0,
        UartDebugMode_DebugOverBridge = 1,
        UartDebugMode_DebugOverModules = 2,
        UartDebugMode_I2CMode = 3,
    } uart_debug_mode_t;

// Functions:

    void InitSettings(void);
    void Settings_Reload(void);
    void Settings_Erase(const char* reason);


// Variables:


    extern uart_debug_mode_t Settings_UartDebugMode;
    extern bool RightAddressIsSet;

#endif // __SETTINGS_H__
