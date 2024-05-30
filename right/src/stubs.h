#ifndef __STUBS_H__
#define __STUBS_H__

// Includes:

    #include <stdbool.h>
    #include <stdint.h>

// Macros:
// Variables:
// Functions:


#ifndef __ZEPHYR__

    static bool Power_RunningOnBattery() { return false; };

#endif


#endif
