#include "messenger_queue.h"
#include "shared/attributes.h"
#include "device.h"
#include "link_protocol.h"
#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

// define pool helpers
#define POOL(NAME, POOL_SIZE, SEGMENT_SIZE) \
    struct { \
        ATTR_ALIGNED uint8_t segments[POOL_SIZE][SEGMENT_SIZE]; \
        ATTR_ALIGNED bool segmentTaken[POOL_SIZE]; \
        uint8_t idx; \
    } NAME;

#define POOL_ALLOCATE(NAME, POOL_SIZE, SEGMENT_SIZE) \
        uint8_t consumedIdx = NAME.idx; \
        NAME.idx = (NAME.idx+1) % POOL_SIZE; \
        if (__sync_bool_compare_and_swap(&NAME.segmentTaken[consumedIdx], false, true)) { \
            return &NAME.segments[consumedIdx][0]; \
        }

#define POOL_FREE(ARG1, NAME, POOL_SIZE, SEGMENT_SIZE)  \
    uint8_t idx = (ARG1 - &NAME.segments[0][0]) / SEGMENT_SIZE; \
    NAME.segmentTaken[idx] = false;

// Define the structures

#define POOL_SIZE 16
#define POOL_REGION_SIZE MAX_LINK_PACKET_LENGTH
#define QUEUE_REGION_SIZE sizeof(messenger_queue_record_t)

POOL(regionPool, POOL_SIZE, POOL_REGION_SIZE);
POOL(queuePool, POOL_SIZE, QUEUE_REGION_SIZE);

#define FIFO_RETRIES 10
struct k_fifo messageQueue;

struct {
    messenger_queue_record_t* first;
    messenger_queue_record_t* last;
} fifo = { .first = NULL, .last = NULL } ;


static void panic(const char* reason) {
    printk("%s", reason);
    k_panic();
}

static void fifoPut(messenger_queue_record_t* node) {
    for (uint8_t tries = 0; tries < FIFO_RETRIES; tries++) {
        if (fifo.first == NULL) {
            node->fifo_reserved = NULL;
            if (!__sync_bool_compare_and_swap((intptr_t*)&fifo.first, NULL, node)) {
                continue;
            }
            fifo.last = fifo.first;
            return;
        } else {
            node->fifo_reserved = NULL;
            if (!__sync_bool_compare_and_swap((intptr_t*)&fifo.last->fifo_reserved, NULL, (intptr_t)node)) {
                continue;
            }
            while ( fifo.last->fifo_reserved != NULL ) {
                fifo.last = fifo.last->fifo_reserved;
            }
            return;
        }
    }
    panic("failed to insert message into fifo\n");
}

static messenger_queue_record_t* fifoTake() {
    for (uint8_t tries = 0; tries < FIFO_RETRIES; tries++) {
        if (fifo.first == NULL) {
            return NULL;
        } else {
            messenger_queue_record_t* node = fifo.first;
            messenger_queue_record_t* next = node->fifo_reserved;
            if (!__sync_bool_compare_and_swap((intptr_t*)&fifo.first, (intptr_t)node, (intptr_t)next)) {
                continue;
            }
            return node;
        }
    }
    panic("failed to retrieve next message from fifo\n");
    return NULL;
}


// handle region pool

uint8_t* MessengerQueue_AllocateMemory() {
    for (uint8_t tries = 0; tries < POOL_SIZE; tries++) {
        POOL_ALLOCATE(regionPool, POOL_SIZE, POOL_REGION_SIZE);
    }
    panic("Message segment queue space ran out!\n");
    return NULL;
}

void MessengerQueue_FreeMemory(const uint8_t* segment) {
    POOL_FREE(segment, regionPool, POOL_SIZE, POOL_REGION_SIZE);
}

// handle queues

uint8_t* allocateQueueSegment() {
    for (uint8_t tries = 0; tries < POOL_SIZE; tries++) {
        POOL_ALLOCATE(queuePool, POOL_SIZE, QUEUE_REGION_SIZE);
    }
    panic("Message segment queue space ran out!\n");
    return NULL;
}

void freeQueueSegment(const uint8_t* segment) {
    POOL_FREE(segment, queuePool, POOL_SIZE, QUEUE_REGION_SIZE);
}

void MessengerQueue_Put(device_id_t src, const uint8_t* data, uint16_t len) {
    messenger_queue_record_t* record = (messenger_queue_record_t*)allocateQueueSegment();
    record->src = src;
    record->len = len;
    record->data = data;
    fifoPut(record);
}

messenger_queue_record_t MessengerQueue_Take() {
    messenger_queue_record_t* record = fifoTake();

    if (record != NULL) {
        messenger_queue_record_t res = *record;
        freeQueueSegment((const uint8_t*)record);
        return res;
    } else {
        messenger_queue_record_t res = {
            .data = NULL,
            .len = 0,
        };
        return res;
    }
}

void MessengerQueue_Init() {
    k_fifo_init(&messageQueue);
}
