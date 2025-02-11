#include "round_trip_test.h"
#include "event_scheduler.h"
#include <stdbool.h>
#include <stdint.h>
#include "messenger.h"

uint16_t RoundTripTime = 0;

typedef struct {
    uint8_t msgIdPlacholder;
    uint32_t timestamp;
} ATTR_PACKED rtt_ping_t;

void RoundTripTest_Init() {
    if (DEVICE_IS_UHK80_RIGHT && DEBUG_TEST_RTT) {
        EventScheduler_Schedule(CurrentTime+10000, EventSchedulerEvent_RoundTripTest, "Event scheduler loop");
    }
}

void RoundTripTest_Run() {
    if (!DEVICE_IS_UHK80_RIGHT) {
        return;
    }

    rtt_ping_t p = { .timestamp = CurrentTime };

    Messenger_Send(DeviceId_Uhk80_Left, MessageId_RoundTripTest, (uint8_t*)&p, sizeof(rtt_ping_t));
}

void RoundTripTest_Receive(const uint8_t* data, uint16_t len) {
    uint8_t ATTR_UNUSED messageId = *(data++);

    if (DEVICE_IS_UHK80_LEFT) {
        Messenger_Send(DeviceId_Uhk80_Right, MessageId_RoundTripTest, data, sizeof(rtt_ping_t));
    }

    if (DEVICE_IS_UHK80_RIGHT) {
        rtt_ping_t* p = (rtt_ping_t*)data;
        RoundTripTime = CurrentTime - p->timestamp;
    }
}


