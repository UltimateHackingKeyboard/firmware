#ifndef __SLEEP_MODE_H__
#define __SLEEP_MODE_H__

// Includes:

    #include <stdbool.h>

// Macros:

    #define SLEEP_MODE_UPDATE_DELAY 500

// Typedefs:

// Variables:

    extern bool SleepModeActive;

// Functions:

    void SleepMode_SetUsbAwake(bool awake);
    void SleepMode_Update();
    void SleepMode_Enter();
    void SleepMode_Exit();
    void SleepMode_WakeHost();

#endif
