#ifndef __USB_STATE_H__
#define __USB_STATE_H__

// Includes:

    #include <stdbool.h>
    #include <stdint.h>


// Variables:

    extern bool UsbState_TransportUp;
    extern bool UsbState_HostIsSuspended;
    extern bool UsbState_WakeupUnanswered;
    extern uint32_t UsbState_FailedWakeups;

// Functions:

    void UsbState_SetUsbTransportUp(bool up);
    void UsbState_SetHostSuspended(bool suspended);
    void UsbState_Delivered(void);

#endif
