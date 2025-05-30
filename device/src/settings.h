#ifndef __SETTINGS_H__
#define __SETTINGS_H__

// Includes

    #include <stdbool.h>
    #include <stdint.h>

// Typedefs:

// Functions:

    void InitSettings(void);
    void Settings_Reload(void);
    void Settings_Erase(const char* reason);


// Variables:


    extern bool RightAddressIsSet;

#endif // __SETTINGS_H__
