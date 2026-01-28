#ifndef __USB_COMMAND_SET_PER_KEY_RGB_H__
#define __USB_COMMAND_SET_PER_KEY_RGB_H__

// Includes:

    #include <stdint.h>

// Functions:

    void UsbCommand_SetPerKeyRgb(const uint8_t *GenericHidOutBuffer, uint8_t *GenericHidInBuffer);

#endif
