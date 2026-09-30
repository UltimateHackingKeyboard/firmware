#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/pm/device.h>
#include "attributes.h"
#include "uart_bridge.h"
#include "messenger.h"
#include "messenger_queue.h"
#include "device.h"
#include "bt_manager.h"
#include "debug.h"
#include "connections.h"
#include "pin_wiring.h"
#include "keyboard/uart_link.h"
#include "shared/uart_parser.h"
#include "uart_defs.h"

// Thread definitions

#define THREAD_STACK_SIZE 2048
#define THREAD_PRIORITY -5

#define UART_FOREVER_TIMEOUT 10000
// First resend after this many ms, doubling on every retry. The ack loop takes ~3ms for a
// key-state frame and ~13ms for a maximum-length one (115200 baud), so 15ms is late enough
// not to duplicate a frame that's merely still in flight, and the backoff handles a bad link.
#define UART_RESEND_DELAY 15
#define UART_RESEND_COUNT 5

typedef enum {
    UartTxState_Idle,
    UartTxState_WaitingForAck,
    UartTxState_Resend,
} uart_tx_state_t;

typedef enum {
    UartRxState_Idle,
    UartRxState_Ack,
    UartRxState_Nack,
} uart_rx_state_t;


// UART uartState state structure
typedef struct {
    uart_link_t core;
    uart_parser_t parser;

    // State variables
    volatile uart_tx_state_t txState;
    volatile uart_rx_state_t rxState;
    volatile uint32_t lastMessageSentTime;
    volatile uint32_t lastLinkActivity;
    uint32_t lastPingTime;
    uint16_t invalidMessagesCounter;
    uint8_t resendTries;

    // Cycle stamps for the latency stats below.
    uint32_t sentCyc;   // last uart_tx of a data frame
    uint32_t ackReqCyc; // last valid frame parsed (ack requested)

    uint8_t* rxBuffer;
    uint8_t txBuffer[UART_MAX_BRIDGE_SERIALIZED_MESSAGE_LENGTH];
    // uart_tx reads its buffer by DMA after returning; it has to outlive the call. Control
    // bytes are serialized by txControlBusy, so one slot is enough.
    uint8_t controlByte;

    struct k_sem txBufferBusy;
    struct k_sem controlThreadSleeper;

    // Connection info (for external interface - TODO)
    connection_id_t connectionId;
    device_id_t remoteDeviceId;
} uart_state_t;



static K_THREAD_STACK_DEFINE(stack_area, THREAD_STACK_SIZE);
struct k_thread thread_data;

uart_state_t bridgeState = {0};

static bool bridgeSuspended = false;
static bool bridgeTxSlotHeld = false;

// Diagnostics, cumulative since boot. Printed by UartBridge_DumpStats.
#define LATENCY_BUCKETS 5

typedef struct {
    uint32_t maxUs;
    uint32_t sumUs;
    uint32_t count;
    uint16_t hist[LATENCY_BUCKETS]; // <1ms, <4ms, <16ms, <64ms, >=64ms
} latency_stats_t;

typedef struct {
    uint32_t framesSent;
    uint32_t framesReceived;
    uint16_t ackWhileIdle;   // ack arrived while we weren't waiting for one
    uint16_t nackWhileIdle;
    uint16_t nackReceived;
    uint16_t resendTimeout;
    uint16_t resendNack;
    uint16_t giveUps;
    uint16_t txSendFail;     // uart_tx returned an error
    uint16_t unexpectedBytes;
    latency_stats_t ackLoop; // sender: uart_tx of a frame -> its ack parsed
    latency_stats_t ackTurn; // receiver: frame parsed -> ack handed to uart_tx
} uart_bridge_stats_t;

static uart_bridge_stats_t stats = {0};

static void recordLatency(latency_stats_t* s, uint32_t startCyc) {
    uint32_t us = k_cyc_to_us_floor32(k_cycle_get_32() - startCyc);
    uint8_t bucket;
    if (us < 1000) {
        bucket = 0;
    } else if (us < 4000) {
        bucket = 1;
    } else if (us < 16000) {
        bucket = 2;
    } else if (us < 64000) {
        bucket = 3;
    } else {
        bucket = 4;
    }
    s->hist[bucket]++;
    s->count++;
    s->sumUs += us;
    s->maxUs = MAX(s->maxUs, us);
}

/* UART message format:
 * [START_BYTE,crc16,escaped(messengerPacket), ENDBYTE]
 * crcMessage = 4 bytes = CRC16 in format [ESCAPE_BYTE,byte1,ESCAPE_BYTE,byte2]
 * escaped(data) = if (dataByte == escape byte or end byte) {ESCAPE_BYTE,dataByte] else [ dataByte ]
 * messengerPacket = [src, dst, messageIds ..., data ...]
 *
 * We serialize both uart-level and messenger-level packets at the same place to avoid unnecessary copying.
 * */

static void wakeControlThread(uart_state_t *uartState) {
    k_sem_give(&uartState->controlThreadSleeper);
}

static void bridgeOnWakeCallback(void *arg) {
    uart_state_t *uartState = (uart_state_t *)arg;
    uartState->lastLinkActivity = k_uptime_get();
    wakeControlThread(uartState);
}

static uint32_t bridgeHoldoffMs(uart_state_t *uartState) {
    return Connections_IsReady(uartState->connectionId)
        ? UART_LP_IDLE_HOLDOFF_MS
        : UART_LP_DISCONNECTED_HOLDOFF_MS;
}

static bool bridgeCanSleep(void *arg) {
    uart_state_t *uartState = (uart_state_t *)arg;
    return uartState->txState == UartTxState_Idle && uartState->rxState == UartRxState_Idle
        && (k_uptime_get() - uartState->lastLinkActivity) >= bridgeHoldoffMs(uartState);
}

static void bridgeReceiveBytes(void *state, const uint8_t* data, uint16_t len) {
    uart_state_t *uartState = (uart_state_t *)state;
    uartState->lastLinkActivity = k_uptime_get();
    UartParser_ProcessIncomingBytes(&uartState->parser, data, len);
}

// UART_RX_DISABLED hook (ISR context), fired on every RX teardown - ours and the
// driver's. Resyncs the parser to prevent frame corruption. Then, unless we slept RX on purpose,
// kicks the control thread to re-arm it behind WakeRx's idle-line gate - re-arming here
// in ISR context used to land mid-stream and cascade framing errors.
static void bridgeOnRxDisabled(void *arg) {
    uart_state_t *uartState = (uart_state_t *)arg;

    UartParser_SetRxBuffer(&uartState->parser, uartState->rxBuffer, UART_MAX_BRIDGE_PAYLOAD_LENGTH);

    if (UartLink_IsAsleep(&uartState->core)) {
        return;
    }

    wakeControlThread(uartState);
}

static void setRxState(uart_state_t *uartState, uart_rx_state_t state) {
    uartState->rxState = state;
    wakeControlThread(uartState);
}

// Dumps a frame as a few log lines rather than one log message per byte: this runs in the
// UART ISR, and a per-byte dump floods the deferred log buffer faster than any log thread
// priority can drain it. Only the head of the frame is shown.
#define FRAME_DUMP_LINE_LEN 80
#define FRAME_DUMP_MAX_LINES 2

static void logFrameBytes(const uint8_t* data, uint16_t len) {
    char line[FRAME_DUMP_LINE_LEN];
    uint16_t pos = 0;
    uint16_t shown = len;
    uint16_t lines = 0;

    for (uint16_t i = 0; i < shown; i++) {
        int n = snprintf(line + pos, FRAME_DUMP_LINE_LEN - pos, "%02x ", data[i]);
        bool lineFull = n < 0 || pos + n >= sizeof(line) - 1;
        if (lineFull) {
            line[pos] = '\0';
            LogU("  %s\n", line);
            pos = 0;
            if (++lines >= FRAME_DUMP_MAX_LINES) {
                break;
            }
            n = snprintf(line, sizeof(line), "%02x ", data[i]);
        }
        pos += n;
    }
    if (pos > 0) {
        LogU("  %s%s\n", line, shown < len ? "..." : "");
    }
}


static void receiveMessage(void *state, uart_control_t messageKind, const uint8_t* data, uint16_t len) {
    uart_state_t *uartState = (uart_state_t *)state;
    uartState->lastLinkActivity = k_uptime_get();
    switch (messageKind) {
        case UartControl_Ack:
            if (uartState->txState == UartTxState_WaitingForAck) {
                recordLatency(&stats.ackLoop, uartState->sentCyc);
                uartState->resendTries = 0;
                uartState->txState = UartTxState_Idle;
                k_sem_give(&uartState->txBufferBusy);
            } else {
                stats.ackWhileIdle++;
            }
            break;
        case UartControl_Nack:
            if (uartState->txState == UartTxState_WaitingForAck) {
                stats.nackReceived++;
                uartState->txState = UartTxState_Resend;
                wakeControlThread(uartState);
            } else {
                stats.nackWhileIdle++;
            }
            break;
        case UartControl_Ping:
            uartState->lastPingTime = k_uptime_get();
            break;
        case UartControl_ValidMessage:
            {
                uartState->lastPingTime = k_uptime_get();
                stats.framesReceived++;
                uartState->ackReqCyc = k_cycle_get_32();
                setRxState(uartState, UartRxState_Ack);

                // message
                uint8_t* oldPacket = uartState->rxBuffer;

                uartState->rxBuffer = MessengerQueue_AllocateMemory();
                UartParser_SetRxBuffer(&uartState->parser, uartState->rxBuffer, UART_MAX_BRIDGE_PAYLOAD_LENGTH);

                connection_id_t connectionId = uartState->connectionId;
                device_id_t remoteDeviceId = uartState->remoteDeviceId;

                Messenger_Enqueue(connectionId, remoteDeviceId, oldPacket, len, 0);
            }
            break;
        case UartControl_InvalidMessage: {
                uartState->invalidMessagesCounter++;
                const char *out1, *out2;
                Messenger_GetMessageDescription(uartState->rxBuffer, 0, &out1, &out2);
                LogUO("Crc-invalid UART message received! %s %s\n", out1, out2 == NULL ? "" : out2);
                logFrameBytes(uartState->rxBuffer, uartState->parser.rxPosition);

                setRxState(uartState, UartRxState_Nack);

                UartParser_SetRxBuffer(&uartState->parser, uartState->rxBuffer, UART_MAX_BRIDGE_PAYLOAD_LENGTH);
            }
            break;
        case UartControl_Unexpected:
            // Out-of-frame garbage: a byte received while the parser is between frames. It is
            // routine after any RX teardown - bridgeOnRxDisabled resyncs the parser while the
            // peer's frame may still be streaming in, so its remaining bytes land here. The
            // parser resyncs itself on the next Start byte. Tearing RX down here instead
            // (the old UartLink_Reset) made every such byte another teardown, another mid-frame
            // re-enable, and so on until the frame ended - one lost frame per hiccup.
            stats.unexpectedBytes++;
            BridgeDbg("BRIDGE RX unexpected byte\n");
            break;
    }
}

int UartBridge_SendMessage(message_t* msg) {
    uart_state_t *uartState = &bridgeState;

    if (uartState == NULL || uartState->core.device == NULL) {
        return -1;
    }

    int err;
    err = k_sem_take(&uartState->txBufferBusy, K_MSEC(UART_FOREVER_TIMEOUT));
    if (err != 0) {
        LogUOS("Uart: failed to take txBufferBusy semaphore.\n");
    }

    // Mark the exchange outstanding before waking, so the control thread's sleep gate
    // (txState==Idle) doesn't re-sleep our RX from under us mid-send.
    uartState->lastMessageSentTime = k_uptime_get();
    uartState->lastLinkActivity = uartState->lastMessageSentTime;
    uartState->txState = UartTxState_WaitingForAck;

    // Wake handshake - our RX up to hear the ack, then the peer - inline on the caller
    // thread, whose stack has to be sized for it.
    UartLink_WakeRx(&uartState->core);
    UartLink_SendWakeByte(&uartState->core);
    UartLink_LockBusy(&uartState->core);

    Messenger_UpdateWatermarks(msg);
    UartParser_StartMessage(&uartState->parser);
    UartParser_AppendEscapedTxBytes(&uartState->parser, (uint8_t[]){msg->src, msg->dst, msg->wm}, 3);
    UartParser_AppendEscapedTxBytes(&uartState->parser, msg->messageId, msg->idsUsed);
    UartParser_AppendEscapedTxBytes(&uartState->parser, msg->data, msg->len);
    UartParser_FinalizeMessage(&uartState->parser);

    stats.framesSent++;
    uartState->sentCyc = k_cycle_get_32();
    err = UartLink_Send(&uartState->core, uartState->parser.txBuffer, uartState->parser.txPosition);
    if (err != 0) {
        stats.txSendFail++;
        k_sem_give(&uartState->core.txControlBusy);
    }

    uartState->lastMessageSentTime = k_uptime_get();
    uartState->lastLinkActivity = uartState->lastMessageSentTime;
    wakeControlThread(uartState);

    return err;
}

static void sendControl(uart_state_t *uartState, uint8_t byte, bool isAck) {
    UartLink_LockBusy(&uartState->core);
    if (isAck) {
        // Measured once we hold the TX slot: includes waiting out our own in-flight frame.
        recordLatency(&stats.ackTurn, uartState->ackReqCyc);
    }
    uartState->controlByte = byte;
    int err = UartLink_Send(&uartState->core, &uartState->controlByte, 1);
    if (err != 0) {
        // No transfer started -> no TX_DONE -> return the slot ourselves.
        stats.txSendFail++;
        k_sem_give(&uartState->core.txControlBusy);
    }
}

// wakePeer: a nack-triggered resend skips the wake handshake, since the peer just parsed
// our garbled frame and is provably awake; a timeout-triggered one redoes it, because
// after the resend delay of silence the peer may have slept again. This must not
// k_sleep - it runs on the control thread, where blocking makes us blind to wake edges,
// acks and pings, which used to cascade into a disconnect + BLE-fallback feedback loop.
static void resend(uart_state_t *uartState, bool wakePeer) {
    if (wakePeer) {
        stats.resendTimeout++;
    } else {
        stats.resendNack++;
    }
    if (uartState->resendTries++ > UART_RESEND_COUNT) {
        stats.giveUps++;
        LogU("Repeatedly failed to send a message! ");
        for (uint16_t i = 0; i < uartState->parser.txPosition; i++) {
            LogU("%i ", uartState->parser.txBuffer[i]);
        }
        LogU("\n");

        uartState->resendTries = 0;
        uartState->txState = UartTxState_Idle;
        k_sem_give(&uartState->txBufferBusy);
    } else {
        uartState->txState = UartTxState_WaitingForAck;
        if (wakePeer) {
            UartLink_WakeRx(&uartState->core);
            UartLink_SendWakeByte(&uartState->core);
        }
        UartLink_LockBusy(&uartState->core);
        uartState->sentCyc = k_cycle_get_32();
        int err = UartLink_Send(&uartState->core, uartState->parser.txBuffer, uartState->parser.txPosition);
        if (err != 0) {
            // No transfer started -> no TX_DONE -> return the slot ourselves.
            stats.txSendFail++;
            k_sem_give(&uartState->core.txControlBusy);
        }
        uartState->lastMessageSentTime = k_uptime_get();
        uartState->lastLinkActivity = uartState->lastMessageSentTime;
    }
}

static void updateConnectionState(uart_state_t *uartState) {
    uint32_t pingDiff = (k_uptime_get() - uartState->lastPingTime);
    connection_id_t connectionId = uartState->connectionId;
    bool oldIsConnected = Connections_IsReady(connectionId);
    bool newIsConnected =  pingDiff < UART_BRIDGE_TIMEOUT;
    if (oldIsConnected != newIsConnected) {
        Connections_SetStateAsync(connectionId, newIsConnected ? ConnectionState_Ready : ConnectionState_Disconnected);
        k_sem_give(&uartState->txBufferBusy);
        k_sem_give(&uartState->core.txControlBusy);
        if (DEVICE_IS_UHK80_LEFT || DEVICE_IS_UHK80_RIGHT) {
            if (newIsConnected) {
                EventScheduler_Reschedule( Timer_GetCurrentTime() + 5000, EventSchedulerEvent_CheckBleVsUart, "Left UART up — schedule BLE vs UART check");
            } else {
                EventScheduler_Reschedule( Timer_GetCurrentTime() + 0, EventSchedulerEvent_CheckBleVsUart, "Left UART down — restart advertising");
            }
        }
    }
}

static void uartLoop(void *arg1, void *arg2, void *arg3) {
    uart_state_t *uartState = (uart_state_t *)arg1;
    uint32_t lastPingSentTime = 0;
    uint32_t currentTime = 0;
    while (1) {
        currentTime = k_uptime_get();

        // Suspended for deep sleep: park until resume kicks us. Pinging or re-arming RX
        // here would run straight into a pm-suspended UARTE.
        if (bridgeSuspended) {
            k_sem_take(&uartState->controlThreadSleeper, K_FOREVER);
            lastPingSentTime = k_uptime_get();
            continue;
        }

        // If a GPIO edge woke us out of RX-sleep, bring RX back before doing anything and
        // hold off re-sleeping, so the frame that follows the wake byte lands on live RX.
        if (UartLink_IsAsleep(&uartState->core)) {
            UartLink_WakeRx(&uartState->core);
            uartState->lastLinkActivity = currentTime;
        } else if (!uartState->core.enabled) {
            // RX went down without us asking for it and bridgeOnRxDisabled kicked us.
            // Re-arm behind WakeRx's idle-line gate so we never come up mid-stream.
            UartLink_WakeRx(&uartState->core);
        }

        updateConnectionState(uartState);

        if (currentTime >= lastPingSentTime + UART_BRIDGE_PING_INTERVAL) {
            UartLink_WakeRx(&uartState->core);
            UartLink_SendWakeByte(&uartState->core);
            sendControl(uartState, UartControlByte_Ping, false);
            lastPingSentTime = currentTime;
        }

        uint32_t wakeTime = lastPingSentTime + UART_BRIDGE_PING_INTERVAL;

        if (Connections_IsReady(uartState->connectionId)) {
            switch (uartState->rxState) {
                case UartRxState_Ack:
                    sendControl(uartState, UartControlByte_Ack, true);
                    uartState->rxState = UartRxState_Idle;
                    break;
                case UartRxState_Nack:
                    sendControl(uartState, UartControlByte_Nack, false);
                    uartState->rxState = UartRxState_Idle;
                    break;
                case UartRxState_Idle:
                    break;
            }

            if (uartState->txState == UartTxState_Resend) {
                LogU("Uart: received Nack, resending\n");
                resend(uartState, false);
            }

            currentTime = k_uptime_get();
            if (uartState->txState == UartTxState_WaitingForAck) {
                uint32_t resendDelay = (UART_RESEND_DELAY << uartState->resendTries);
                uint32_t resendTime = uartState->lastMessageSentTime + resendDelay;
                if (currentTime >= resendTime) {
                    LogU("Uart: didn't receive ack %d, resending (delay %d)\n", currentTime, resendDelay);
                    resend(uartState, true);
                } else {
                    wakeTime = MIN(wakeTime, resendTime);
                }
            }
        } else {
            uartState->txState = UartTxState_Idle;
            uartState->rxState = UartRxState_Idle;
        }

        currentTime = k_uptime_get();

        uint32_t sleepEligibleAt = uartState->lastLinkActivity + bridgeHoldoffMs(uartState);
        bool idle = uartState->txState == UartTxState_Idle && uartState->rxState == UartRxState_Idle;
        if (idle && currentTime < sleepEligibleAt) {
            wakeTime = MIN(wakeTime, sleepEligibleAt);
        }

        if (wakeTime > currentTime) {
            if (idle && currentTime >= sleepEligibleAt) {
                UartLink_SleepRx(&uartState->core);
            }
            k_sem_take(&uartState->controlThreadSleeper, K_MSEC(wakeTime - currentTime));
        }
    }
}


static void initUart(
        connection_id_t connectionId,
        device_id_t remoteDeviceId,
        uart_state_t *uartState,
        const pin_wiring_dev_t* device
) {
    if (device == NULL || device->device == NULL) {
        return;
    }

    ATTR_UNUSED static uint8_t calls = 0;
    ASSERT(++calls <= 2); // otherwise we are leaking memory in MessengerQueue_AllocateMemory

    // Initialize semaphores
    k_sem_init(&uartState->txBufferBusy, UART_LINK_SLOTS, UART_LINK_SLOTS);
    k_sem_init(&uartState->controlThreadSleeper, 1, 1);

    // Initialize state
    uartState->txState = UartTxState_Idle;
    uartState->rxState = UartRxState_Idle;
    uartState->lastMessageSentTime = 0;
    uartState->lastPingTime = -2*UART_BRIDGE_TIMEOUT;
    uartState->invalidMessagesCounter = 0;
    uartState->resendTries = 0;
    uartState->remoteDeviceId = remoteDeviceId;
    uartState->connectionId = connectionId;

    // TODO: Set connectionId and remoteDeviceId from configuration
    uartState->connectionId = DEVICE_IS_UHK80_LEFT ? ConnectionId_UartRight : ConnectionId_UartLeft;
    uartState->remoteDeviceId = DEVICE_IS_UHK80_LEFT ? DeviceId_Uhk80_Right : DeviceId_Uhk80_Left;

    UartLink_Init(&uartState->core, device->device, bridgeReceiveBytes, (void*)uartState);
    UartParser_InitParser(&uartState->parser, &receiveMessage, (void*)uartState);

    uartState->rxBuffer = MessengerQueue_AllocateMemory();
    UartParser_SetRxBuffer(&uartState->parser, uartState->rxBuffer, UART_MAX_BRIDGE_PAYLOAD_LENGTH);
    UartParser_SetTxBuffer(&uartState->parser, uartState->txBuffer, UART_MAX_BRIDGE_SERIALIZED_MESSAGE_LENGTH);
}

void InitUartBridge(void) {
    if (PinWiringConfig->device_uart_bridge != NULL && PinWiringConfig->device_uart_bridge->device != NULL) {
        initUart(
                DEVICE_IS_UHK80_LEFT ? ConnectionId_UartRight : ConnectionId_UartLeft,
                DEVICE_IS_UHK80_LEFT ? DeviceId_Uhk80_Right : DeviceId_Uhk80_Left,
                &bridgeState,
                PinWiringConfig->device_uart_bridge
                );

        // Low-power: RXD wake pin (no-op / empty spec when UART_LOWPOWER is off or the
        // board defines no bridge-rx-gpios).
        UartLink_InitWake(
                &bridgeState.core,
                (struct gpio_dt_spec)GPIO_DT_SPEC_GET_OR(DT_PATH(zephyr_user), bridge_rx_gpios, {0}),
                bridgeOnWakeCallback,
                &bridgeState,
                bridgeCanSleep,
                bridgeOnRxDisabled);

        k_thread_create(
                &thread_data, stack_area,
                K_THREAD_STACK_SIZEOF(stack_area),
                uartLoop,
                &bridgeState, NULL, NULL,
                THREAD_PRIORITY, 0, K_NO_WAIT
                );
        k_thread_name_set(&thread_data, "test_uart");
    }

    UartBridge_Enable();
}


void UartBridge_Enable() {
    // While suspended, ignore re-arm requests - in particular the ReenableUart event that
    // our own uart_rx_disable schedules - so RX is never armed on a suspended UARTE.
    if (bridgeSuspended) {
        return;
    }
    UartLink_Enable(&bridgeState.core);
}

void UartBridge_Suspend(void) {
    uart_state_t *uartState = &bridgeState;
    if (uartState->core.device == NULL || bridgeSuspended) {
        return;
    }

    bridgeSuspended = true;

    // Every send takes the TX slot before uart_tx and only the UART_TX_DONE callback
    // returns it, so taking it here both waits out any in-flight TX and blocks new ones -
    // the control thread simply parks on its next send. On timeout we hold no slot and
    // must not hand one back on resume.
    bridgeTxSlotHeld = (k_sem_take(&uartState->core.txControlBusy, K_MSEC(200)) == 0);

    // The disable completes asynchronously in the UART_RX_DISABLED callback; suspending
    // while RX is still armed asserts in the driver.
    uart_rx_disable(uartState->core.device);
    k_msleep(20);

    pm_device_action_run(uartState->core.device, PM_DEVICE_ACTION_SUSPEND);

    Connections_SetStateAsync(uartState->connectionId, ConnectionState_Disconnected);
}

void UartBridge_Resume(void) {
    uart_state_t *uartState = &bridgeState;
    if (uartState->core.device == NULL || !bridgeSuspended) {
        return;
    }

    pm_device_action_run(uartState->core.device, PM_DEVICE_ACTION_RESUME);

    bridgeSuspended = false;
    UartBridge_Enable();

    if (bridgeTxSlotHeld) {
        k_sem_give(&uartState->core.txControlBusy);
        bridgeTxSlotHeld = false;
    }

    wakeControlThread(uartState);
}

static void dumpLatency(const char* label, const latency_stats_t* s) {
    uint32_t meanUs = s->count == 0 ? 0 : s->sumUs / s->count;
    LogU("  %s: n=%u mean=%uus max=%uus\n", label, (unsigned)s->count, (unsigned)meanUs, (unsigned)s->maxUs);
    LogU("    hist <1/<4/<16/<64/>=64ms: %u/%u/%u/%u/%u\n",
        (unsigned)s->hist[0], (unsigned)s->hist[1], (unsigned)s->hist[2], (unsigned)s->hist[3], (unsigned)s->hist[4]);
}

void UartBridge_DumpStats(void) {
    uart_state_t *uartState = &bridgeState;
    uart_link_t *core = &uartState->core;
    if (core->device == NULL) {
        LogU("Uart bridge: no bridge uart on this routing\n");
        return;
    }
    LogU("Uart bridge stats (t=%u ms): enabled=%d txState=%d rxState=%d resendTries=%u\n",
        (unsigned)Timer_GetCurrentTime(), (int)core->enabled, (int)uartState->txState, (int)uartState->rxState,
        (unsigned)uartState->resendTries);
    LogU("  frames: sent=%u received=%u crcInvalid=%u unexpectedBytes=%u\n",
        (unsigned)stats.framesSent, (unsigned)stats.framesReceived, (unsigned)uartState->invalidMessagesCounter,
        (unsigned)stats.unexpectedBytes);
    LogU("  acks: whileIdle=%u nack=%u nackWhileIdle=%u\n",
        (unsigned)stats.ackWhileIdle, (unsigned)stats.nackReceived, (unsigned)stats.nackWhileIdle);
    LogU("  resends: timeout=%u nack=%u giveUps=%u txSendFail=%u txAborted=%u\n",
        (unsigned)stats.resendTimeout, (unsigned)stats.resendNack, (unsigned)stats.giveUps,
        (unsigned)stats.txSendFail, (unsigned)core->txAbortedCount);
    LogU("  rx stopped: overrun=%u framing=%u break=%u other=%u disabled=%u\n",
        (unsigned)core->rxStoppedOverrun, (unsigned)core->rxStoppedFraming, (unsigned)core->rxStoppedBreak,
        (unsigned)core->rxStoppedOther, (unsigned)core->rxDisabledCount);
    dumpLatency("ackLoop (send->ack)", &stats.ackLoop);
    dumpLatency("ackTurn (rx->ack tx)", &stats.ackTurn);
}
