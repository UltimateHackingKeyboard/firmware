#ifndef __MESSENGER_H__
#define __MESSENGER_H__

// Includes:

    #include "shared/attributes.h"
#include <zephyr/bluetooth/conn.h>
    #include <inttypes.h>

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

    // the point of message_t is to reduce the number of times we need to copy the message.
    typedef struct {
        const uint8_t* data;
        uint16_t len;
        uint8_t messageId[2];
        uint8_t idsUsed;
    } ATTR_PACKED message_t;

// Functions:

    void Messenger_Receive(uint8_t src, const uint8_t* data, uint16_t len);
    void Messenger_SendMessage(uint8_t dst, message_t message);
    void Messenger_Send(uint8_t dst, uint8_t messageId, const uint8_t* data, uint16_t len);
    void Messenger_Send2(uint8_t dst, uint8_t messageId, uint8_t messageId2, const uint8_t* data, uint16_t len);
    bool Messenger_Availability(uint8_t dst, messenger_availability_op_t operation);

    void Messenger_Enqueue(uint8_t src, const uint8_t* data, uint16_t len);
    void Messenger_ProcessQueue();

    void Messenger_Init();

#endif // __MESSENGER_H__
