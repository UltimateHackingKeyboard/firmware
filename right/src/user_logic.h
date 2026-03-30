#ifndef __USER_LOGIC_H__
#define __USER_LOGIC_H__

// Includes:

    #include <stdbool.h>
    #include <stdint.h>

// Macros:

// Typedefs:

// Variables:

    extern uint32_t UserLogic_LastEventloopTime;

// Functions:

    void RunUserLogic(void);
    void RunUhk80LeftHalfLogic();

#endif
