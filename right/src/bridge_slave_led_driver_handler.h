#ifndef __BRIDGE_SLAVE_LED_DRIVER_HANDLER_H__
#define __BRIDGE_SLAVE_LED_DRIVER_HANDLER_H__

// Includes:

    #include "fsl_common.h"

// Functions:

    extern bool BridgeSlaveLedDriverHandler(uint8_t ledDriverId);
    extern void SetLeds(uint8_t ledBrightness);

#endif
