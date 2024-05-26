#ifndef __MESSENGER_H__
#define __MESSENGER_H__

// Includes:

    #include <zephyr/bluetooth/conn.h>
    #include <inttypes.h>

// Typedefs:

    // the point of message_t is to reduce the number of times we need to copy the message.
    typedef struct {
        const uint8_t* data;
        uint16_t len;
        uint8_t messageId;
    } message_t;

// Functions:

    void Messenger_Receive(uint8_t src, const uint8_t* data, uint16_t len);
    void Messenger_SendMessage(uint8_t dst, message_t message);
    void Messenger_Send(uint8_t dst, uint8_t messageId, const uint8_t* data, uint16_t len);

#endif // __MESSENGER_H__
