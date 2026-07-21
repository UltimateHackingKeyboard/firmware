#ifndef __USB_STATE_H__
#define __USB_STATE_H__

// Includes:

    #include <stdbool.h>

// Functions:

    void UsbState_SetUsbTransportUp(bool up);
    void UsbState_SetUsbAwake(bool awake);
    bool UsbState_IsTransportUp(void);
    bool UsbState_IsAwake(void);

#endif
