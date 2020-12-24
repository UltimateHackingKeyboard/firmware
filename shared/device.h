#ifndef __DEVICE_H__
#define __DEVICE_H__

// Macros:

    #define DEVICE_ID_UHK60V1 0
    #define DEVICE_ID_UHK60V2 1

#ifndef DEVICE_ID
    #error "DEVICE_ID is undefined!"
#endif

#if !(DEVICE_ID == DEVICE_ID_UHK60V1 || DEVICE_ID == DEVICE_ID_UHK60V2)
    #error "DEVICE_ID is invalid! See shared/device.h for valid values."
#endif

#endif
