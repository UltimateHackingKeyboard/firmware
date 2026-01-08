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
        MessageId_SyncableProperty = 2,
        MessageId_Log = 3,
        MessageId_Ping = 4,
        MessageId_RoundTripTest = 5,
        MessageId_ResendRequest = 6,
        MessageId_Command = 7,
    } message_id_t;

    typedef enum {
        MessengerCommand_Reboot = 0,
        MessengerCommand_Count,
    } messenger_command_t;

    typedef enum {
        MessageOffset_Src = 0,
        MessageOffset_Dst = 1,
        MessageOffset_Wm = 2,
        MessageOffset_MsgId1 = 3,
        MessageOffset_Payload = MessageOffset_MsgId1,
    } message_offset_t;

    // the point of message_t is to reduce the number of times we need to copy the message.
    // for this reason we also serialize multiple protocol layers in a single go later.
    typedef struct {
        const uint8_t* data;
        uint16_t len;
        uint8_t messageId[2];
        uint8_t idsUsed;
        uint8_t src;
        uint8_t dst;
        uint8_t wm;
        uint8_t connectionId;
    } ATTR_PACKED message_t;

// Functions:

    uint16_t Messenger_GetMissedMessages(device_id_t dst);

    void Messenger_SendMessage(message_t* message);
    void Messenger_Send(device_id_t dst, uint8_t messageId, const uint8_t* data, uint16_t len);
    void Messenger_Send2(device_id_t dst, uint8_t messageId, uint8_t messageId2, const uint8_t* data, uint16_t len);
    void Messenger_Send2Via(device_id_t dst, connection_id_t connectionId, uint8_t messageId, uint8_t messageId2, const uint8_t* data, uint16_t len);

    void Messenger_Enqueue(uint8_t srcConnectionId, uint8_t src, const uint8_t* data, uint16_t len, uint8_t offset);
    void Messenger_ProcessQueue();
    void Messenger_GetMessageDescription(uint8_t* data, uint8_t offset, const char** out1, const char** out2);

    void Messenger_Init();

#endif // __MESSENGER_H__
