#ifndef __USB_COMMAND_REENUMERATE_H__
#define __USB_COMMAND_REENUMERATE_H__

// Includes:

    #include <stdint.h>
    #include <stdbool.h>

// Functions:

    void Reboot(bool rebootPeripherals);
    void UsbCommand_Reenumerate(const uint8_t *GenericHidOutBuffer, uint8_t *GenericHidInBuffer);

#endif
