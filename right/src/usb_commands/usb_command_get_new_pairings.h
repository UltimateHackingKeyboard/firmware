#ifndef __USB_COMMAND_GET_NEW_PAIRINGS_H__
#define __USB_COMMAND_GET_NEW_PAIRINGS_H__

// Includes:

    #include <stdint.h>

#ifdef __ZEPHYR__

// Typedefs:

// Functions:

    void UsbCommand_GetNewPairings(const uint8_t *GenericHidOutBuffer, uint8_t *GenericHidInBuffer);

#endif

#endif
