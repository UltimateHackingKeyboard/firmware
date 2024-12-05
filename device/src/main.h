#ifndef __MAIN_H__
#define __MAIN_H__

// Includes:

    #include <inttypes.h>
    #include <stdbool.h>
    #include <zephyr/kernel.h>

// Macros:

// Typedefs:

// Variables:

    extern k_tid_t Main_ThreadId;

// Functions:

    void Main_Wake();

#endif // __MAIN_H__

