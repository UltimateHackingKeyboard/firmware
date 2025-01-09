#ifndef __SETTINGS_H__
#define __SETTINGS_H__

// Includes

    #include <stdbool.h>
    #include <stdint.h>

// Functions:

    void InitSettings(void);
    void Settings_Reload(void);
    void Settings_Erase(void);

// Variables:

    extern bool RightAddressIsSet;

#endif // __SETTINGS_H__
