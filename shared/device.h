#ifndef __DEVICE_H__
#define __DEVICE_H__

// Macros:

    #define DEVICE_ID_UHK60V1 1
    #define DEVICE_ID_UHK60V2 2

    #define DEVICE_COUNT 2

#ifndef DEVICE_ID
    #error "DEVICE_ID is undefined!"
#endif

#if (1-DEVICE_ID-1+DEVICE_ID-0 == 0) //ensures that DEVICE_ID is "defined" with a specific value
#if !(DEVICE_ID == DEVICE_ID_UHK60V1 || DEVICE_ID == DEVICE_ID_UHK60V2)
    #error "DEVICE_ID is invalid! See shared/device.h for valid values."
#endif
#endif

#endif
