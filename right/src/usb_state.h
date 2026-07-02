#ifndef __USB_STATE_H__
#define __USB_STATE_H__

// Includes:

    #include <stdbool.h>


// Variables:

    extern bool UsbState_TransportUp;
    extern bool UsbState_Awake;

// Functions:

    void UsbState_SetUsbTransportUp(bool up);
    void UsbState_SetUsbAwake(bool awake);
    void UsbState_Delivered(void);

#endif
