#ifndef __MESSENGER_H__
#define __MESSENGER_H__

// Includes:

    #include "shared/attributes.h"
    #include <zephyr/bluetooth/conn.h>
    #include <inttypes.h>
    #include "device.h"
    #include "connections.h"

// Typedefs:

    typedef enum {
        MessengerAvailabilityOp_InquireOneEmpty,
        MessengerAvailabilityOp_InquireAllEmpty,
        MessengerAvailabilityOp_BlockTillOneEmpty,
    } messenger_availability_op_t;

    typedef enum {
        MessageId_StateSync = 1,
        MessageId_SyncableProperty,
        MessageId_Log,
        MessageId_Ping,
    } message_id_t;

typedef enum {
    MessageOffset_Src = 0,
    MessageOffset_Dst = 1,
    MessageOffset_MsgId1 = 2,
} message_offset_t;

    // the point of message_t is to reduce the number of times we need to copy the message.
    typedef struct {
        const uint8_t* data;
        uint16_t len;
        uint8_t messageId[2];
        uint8_t idsUsed;
        uint8_t src;
        uint8_t dst;
        uint8_t connectionId;
    } ATTR_PACKED message_t;

// Functions:

    void Messenger_SendMessage(message_t message);
    void Messenger_Send(device_id_t dst, uint8_t messageId, const uint8_t* data, uint16_t len);
    void Messenger_Send2(device_id_t dst, uint8_t messageId, uint8_t messageId2, const uint8_t* data, uint16_t len);
    void Messenger_Send2Via(device_id_t dst, connection_id_t connectionId, uint8_t messageId, uint8_t messageId2, const uint8_t* data, uint16_t len);
    bool Messenger_Availability(device_id_t dst, messenger_availability_op_t operation);

    void Messenger_Enqueue(uint8_t srcConnectionId, uint8_t src, const uint8_t* data, uint16_t len, uint8_t offset);
    void Messenger_ProcessQueue();

    void Messenger_Init();

#endif // __MESSENGER_H__
