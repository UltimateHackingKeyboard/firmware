#include "messenger_queue.h"
#include "shared/attributes.h"
#include "device.h"
#include "link_protocol.h"
#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include "logger.h"
#include "thread_stats.h"
#include "trace.h"

uint16_t MessengerQueue_DroppedMessageCount = 0;

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

#define POOL_STATE(NAME, POOL_SIZE, SEGMENT_SIZE) \
{ \
    uint8_t count = 0; \
    for (uint8_t i = 0; i < POOL_SIZE; i++) { \
        if (NAME.segmentTaken[i]) { \
            count++; \
        } \
    } \
    printk("%i/%i segments taken\n", count, POOL_SIZE); \
} \

// Define the structures

#define POOL_SIZE 16
#define POOL_REGION_SIZE MAX_LINK_PACKET_LENGTH
#define QUEUE_REGION_SIZE sizeof(messenger_queue_record_t)

static uint8_t blackholeBuffer[MAX_LINK_PACKET_LENGTH];
uint8_t* MessengerQueue_BlackholeBuffer = blackholeBuffer;

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
    ThreadStats_Snap();
    Trace_Printf("E0");
    Trace_Print(LogTarget_ErrorBuffer, "Messenger pool space ran out!");
    ThreadStats_Print();
    LogUOS("Messenger message pool space ran out!\n");
    return blackholeBuffer;
}

void MessengerQueue_FreeMemory(const uint8_t* segment) {
    if (segment != blackholeBuffer) {
        POOL_FREE(segment, regionPool, POOL_SIZE, POOL_REGION_SIZE);
    }
}

// handle queues

static uint8_t* allocateQueueSegment() {
    for (uint8_t tries = 0; tries < POOL_SIZE; tries++) {
        POOL_ALLOCATE(queuePool, POOL_SIZE, QUEUE_REGION_SIZE);
    }
    LogUOS("Messager queue node space ran out!\n");
    return NULL;
}

void freeQueueSegment(const uint8_t* segment) {
    POOL_FREE(segment, queuePool, POOL_SIZE, QUEUE_REGION_SIZE);
}

void MessengerQueue_Put(device_id_t src, const uint8_t* data, uint16_t len, uint8_t offset) {
    if (data == blackholeBuffer) {
        MessengerQueue_DroppedMessageCount++;
        return;
    }

    messenger_queue_record_t* record = (messenger_queue_record_t*)allocateQueueSegment();

    if (record == NULL) {
        MessengerQueue_DroppedMessageCount++;
        return;
    }

    record->src = src;
    record->len = len;
    record->data = data;
    record->offset = offset;
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

ATTR_UNUSED uint8_t MessengerQueue_GetOccupiedCount() {
    uint8_t count = 0;
    for (uint8_t i = 0; i < POOL_SIZE; i++) {
        if (regionPool.segmentTaken[i]) {
            count++;
        }
    }
    return count;
}

void MessengerQueue_Init() {
    k_fifo_init(&messageQueue);
}
