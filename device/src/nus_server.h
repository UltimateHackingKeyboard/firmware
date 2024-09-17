#ifndef __NUS_SERVER_H__
#define __NUS_SERVER_H__

// Includes:
//
    #include "link_protocol.h"
    #include "messenger.h"

// Functions:

    extern int NusServer_Init(void);
    extern void NusServer_Disconnected();
    extern void NusServer_Send(const uint8_t *data, uint16_t len);
    extern void NusServer_SendMessage(message_t msg);
    extern bool NusServer_Availability(messenger_availability_op_t operation);

#endif // __NUS_SERVER_H__
