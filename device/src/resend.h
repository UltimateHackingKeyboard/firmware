#ifndef __RESEND_H__
#define __RESEND_H__

// Includes:

    #include <stdbool.h>
    #include <stdint.h>
    #include "device.h"
    #include "connections.h"
    #include "messenger.h"

// Macros:


// Typedefs:


// Variables:


// Functions:

    void Resend_ResendRequestReceived(device_id_t src, connection_id_t connectionId, const uint8_t* data, uint16_t len);
    void Resend_RequestResendSync();
    void Resend_RequestResendAsync(device_id_t dst, connection_id_t connectionId, uint8_t messageWatermark);
    void Resend_RegisterMessageAndUpdateWatermarks(message_t* msg);

#endif
