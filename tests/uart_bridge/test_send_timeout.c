#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

// Only platform and serialization operations are mocked. The send function is
// extracted from uart_bridge.c by test_send_timeout.py.
struct k_sem { unsigned permits; };
typedef struct { const void *device; struct k_sem txControlBusy; } uart_link_t;
typedef struct { uint8_t txBuffer[128]; uint16_t txPosition; } uart_parser_t;
typedef struct {
    uart_link_t core;
    uart_parser_t parser;
    struct k_sem txBufferBusy;
    unsigned lastMessageSentTime, lastLinkActivity, txState, sentCyc;
    uint8_t txWm;
} uart_state_t;
typedef struct {
    const uint8_t *data;
    uint16_t len;
    uint8_t messageId[2], idsUsed, src, dst, wm;
} message_t;
enum { UartTxState_WaitingForAck = 1 };
static uart_state_t bridgeState;
static struct { unsigned framesSent, txSendFail; } stats;
static unsigned takeCalls, giveCalls, wakeCalls, watermarkCalls, parserCalls, sendCalls;
static unsigned controlWakes, warningCalls, requestedTimeout, nextWatermark;
static int sendResult;
#define K_MSEC(ms) (ms)

static int k_sem_take(struct k_sem *sem, unsigned timeout) {
    assert(sem == &bridgeState.txBufferBusy);
    ++takeCalls;
    requestedTimeout = timeout;
    if (!sem->permits) { return -EAGAIN; }
    --sem->permits;
    return 0;
}
static void k_sem_give(struct k_sem *sem) { ++sem->permits; ++giveCalls; }
static unsigned k_uptime_get(void) { return 123; }
static unsigned k_cycle_get_32(void) { return 456; }
static void LogWrn(const char *text) { (void)text; ++warningCalls; }
static void UartLink_WakeRx(uart_link_t *link) { (void)link; ++wakeCalls; }
static void UartLink_SendWakeByte(uart_link_t *link) { (void)link; ++wakeCalls; }
static void UartLink_LockBusy(uart_link_t *link) {
    assert(link->txControlBusy.permits == 1);
    --link->txControlBusy.permits;
}
static void Messenger_UpdateWatermarks(message_t *msg) {
    msg->wm = nextWatermark++;
    ++watermarkCalls;
}
static void UartParser_StartMessage(uart_parser_t *parser) {
    ++parserCalls;
    parser->txPosition = 0;
}
static void UartParser_AppendEscapedTxBytes(uart_parser_t *parser, const uint8_t *data, uint16_t len) {
    ++parserCalls;
    assert(parser->txPosition + len <= sizeof(parser->txBuffer));
    memcpy(parser->txBuffer + parser->txPosition, data, len);
    parser->txPosition += len;
}
static void UartParser_FinalizeMessage(uart_parser_t *parser) { (void)parser; ++parserCalls; }
static int UartLink_Send(uart_link_t *link, const uint8_t *data, uint16_t len) {
    assert(link == &bridgeState.core && data == bridgeState.parser.txBuffer && len);
    ++sendCalls;
    return sendResult;
}
static void wakeControlThread(uart_state_t *state) { (void)state; ++controlWakes; }

#include "send_under_test.inc"

static void reset(void) {
    memset(&bridgeState, 0, sizeof(bridgeState));
    memset(&stats, 0, sizeof(stats));
    bridgeState.core.device = &bridgeState;
    bridgeState.core.txControlBusy.permits = 1;
    bridgeState.txBufferBusy.permits = 1;
    takeCalls = giveCalls = wakeCalls = watermarkCalls = parserCalls = sendCalls = 0;
    controlWakes = warningCalls = requestedTimeout = 0;
    nextWatermark = 17;
    sendResult = 0;
}

int main(void) {
    const uint8_t payload[] = { 1, 2, 3 };
    message_t msg = { .data = payload, .len = sizeof(payload), .messageId = { 1 },
        .idsUsed = 1, .src = 20, .dst = 21, .wm = 9 };

    // A timed-out sender must leave an outstanding frame and its owner intact.
    reset();
    memset(bridgeState.parser.txBuffer, 0xa5, sizeof(bridgeState.parser.txBuffer));
    bridgeState.parser.txPosition = 11;
    bridgeState.txBufferBusy.permits = 0;
    bridgeState.txState = UartTxState_WaitingForAck;
    bridgeState.txWm = 8;
    bridgeState.lastMessageSentTime = 71;
    bridgeState.lastLinkActivity = 72;
    bridgeState.sentCyc = 73;
    uart_state_t before = bridgeState;
    int result = UartBridge_SendMessage(&msg);
    assert(result == -EAGAIN);
    assert(memcmp(&before, &bridgeState, sizeof(before)) == 0);
    assert(takeCalls == 1 && requestedTimeout == UART_FOREVER_TIMEOUT && warningCalls == 1);
    assert(!giveCalls && !wakeCalls && !watermarkCalls && !parserCalls && !sendCalls && !controlWakes);
    assert(!stats.framesSent && !stats.txSendFail && nextWatermark == 17 && msg.wm == 9);

    // Ordinary state-sync traffic still serializes after acquiring the buffer.
    reset();
    assert(UartBridge_SendMessage(&msg) == 0);
    const uint8_t expected[] = { 20, 21, 17, 1, 1, 2, 3 };
    assert(bridgeState.parser.txPosition == sizeof(expected));
    assert(memcmp(bridgeState.parser.txBuffer, expected, sizeof(expected)) == 0);
    assert(bridgeState.txBufferBusy.permits == 0 && bridgeState.core.txControlBusy.permits == 0);
    assert(bridgeState.txState == UartTxState_WaitingForAck && bridgeState.txWm == 17);
    assert(wakeCalls == 2 && watermarkCalls == 1 && parserCalls == 5 && sendCalls == 1);
    assert(controlWakes == 1 && stats.framesSent == 1 && !giveCalls && !stats.txSendFail);

    // Immediate driver failure returns only the acquired hardware TX slot;
    // the buffer remains owned by the existing retry/ack state machine.
    reset();
    sendResult = -EIO;
    assert(UartBridge_SendMessage(&msg) == -EIO);
    assert(bridgeState.core.txControlBusy.permits == 1 && giveCalls == 1);
    assert(bridgeState.txBufferBusy.permits == 0);
    assert(stats.framesSent == 1 && stats.txSendFail == 1 && controlWakes == 1);

    reset();
    bridgeState.core.device = NULL;
    assert(UartBridge_SendMessage(&msg) == -1 && !takeCalls && !sendCalls);
    puts("UART send: timeout isolation, successful send, driver failure and absent device passed");
}
