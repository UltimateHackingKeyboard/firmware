#ifndef __MESSENGER_QUEUE_H__
#define __MESSENGER_QUEUE_H__

// Includes:

    #include "connections.h"
    #include "shared/attributes.h"
    #include <inttypes.h>
    #include "device.h"

// Typedefs:

    typedef struct {
        __attribute__((aligned)) void *fifo_reserved;   /* 1st word reserved for use by FIFO */
        uint16_t len;
        device_id_t src;
        uint8_t offset;
        const uint8_t* data;
    } messenger_queue_record_t;


// Functions:
    void MessengerQueue_Init();

    uint8_t* MessengerQueue_AllocateMemory();
    void MessengerQueue_FreeMemory(const uint8_t* segment);

    void MessengerQueue_Put(device_id_t src, const uint8_t* data, uint16_t len, uint8_t offset);
    messenger_queue_record_t MessengerQueue_Take();

#endif // __MESSENGER_QUEUE_H__
