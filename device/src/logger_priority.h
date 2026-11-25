#ifndef __LOGGER_PRIORITY_H__
#define __LOGGER_PRIORITY_H__

// Includes:

    #include <inttypes.h>
    #include <stdbool.h>
    #include <zephyr/kernel.h>

// Macros:

// Typedefs:

// Variables:

// Functions:

    // True while the logging thread runs at high priority (see Logger_SetPriority).
    extern bool Logger_PriorityHigh;

    void Logger_SetPriority(bool high);

#endif // __MAIN_H__

