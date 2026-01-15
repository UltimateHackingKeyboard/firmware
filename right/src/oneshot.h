#ifndef __ONESHOT_H__
#define __ONESHOT_H__

// Includes:

    #include <stdint.h>
    #include <stdbool.h>

// Macros:

// Typedefs:

    typedef enum {
        OneShotState_Idle = 0,
        OneShotState_WoundUp = 1,
        OneShotState_Unwinding = 2,
    } oneshot_state_t;

// Variables:

    extern oneshot_state_t OneShot_State;

// Functions:

    void Oneshot_Init(void);
    void OneShot_Activate(uint32_t atTime);
    void OneShot_SignalInterrupt(void);

    void OneShot_OnTimeout(void);
    bool OneShot_ShouldPostpone(bool queueNonEmpty, bool someonePostponing);

#endif
