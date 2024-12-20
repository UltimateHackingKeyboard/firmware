#ifndef __USB_COMMAND_GET_NEW_PAIRINGS_H__
#define __USB_COMMAND_GET_NEW_PAIRINGS_H__

// Includes:

    #include <stdint.h>

#ifdef __ZEPHYR__

// Includes:

    #include <stdint.h>

// Typedefs:

// Functions:

    void UsbCommand_UpdateNewPairingsFlag();
    void UsbCommand_GetNewPairings(uint8_t page, const uint8_t *GenericHidOutBuffer, uint8_t *GenericHidInBuffer);

#endif

#endif
