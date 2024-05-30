#ifndef __POWER_H__
#define __POWER_H__

// Includes:

    #include <stdbool.h>

// Variables:


    extern bool HavePower;

// Functions:

    bool Power_RunningOnBattery();

    void InitPower(void);

#endif // __POWER_H__

