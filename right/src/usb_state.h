#ifndef __USB_STATE_H__
#define __USB_STATE_H__

// Includes:

    #include <stdbool.h>


// Variables:

    extern bool UsbState_TransportUp;
    extern bool UsbState_HostIsSuspended;

// Functions:

    void UsbState_SetUsbTransportUp(bool up);
    void UsbState_SetHostSuspended(bool suspended);
    void UsbState_Delivered(void);

#endif
