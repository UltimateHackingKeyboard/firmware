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
#define UART_RESEND_DELAY 64
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

    uint8_t* rxBuffer;
    uint8_t txBuffer[UART_MAX_BRIDGE_SERIALIZED_MESSAGE_LENGTH];

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


static void receivePacket(void *state, uart_control_t messageKind, const uint8_t* data, uint16_t len) {
    uart_state_t *uartState = (uart_state_t *)state;
    uartState->lastLinkActivity = k_uptime_get();
    switch (messageKind) {
        case UartControl_Ack:
            if (uartState->txState == UartTxState_WaitingForAck) {
                uartState->resendTries = 0;
                uartState->txState = UartTxState_Idle;
                k_sem_give(&uartState->txBufferBusy);
            }
            break;
        case UartControl_Nack:
            if (uartState->txState == UartTxState_WaitingForAck) {
                uartState->txState = UartTxState_Resend;
                wakeControlThread(uartState);
            }
            break;
        case UartControl_Ping:
            uartState->lastPingTime = k_uptime_get();
            break;
        case UartControl_ValidMessage:
            {
                uartState->lastPingTime = k_uptime_get();
                len -= UART_CRC_LEN;
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
                Messenger_GetMessageDescription(uartState->rxBuffer, UART_CRC_LEN, &out1, &out2);
                LogUO("Crc-invalid UART message received! %s %s ", out1, out2 == NULL ? "" : out2);

                for (uint16_t i = 0; i < uartState->parser.rxPosition; i++) {
                    LogU("%i ", uartState->rxBuffer[i]);
                }
                LogU("\n");

                setRxState(uartState, UartRxState_Nack);

                UartParser_SetRxBuffer(&uartState->parser, uartState->rxBuffer, UART_MAX_BRIDGE_PAYLOAD_LENGTH);
            }
            break;
        case UartControl_Unexpected:
#if UART_LOWPOWER
            // Out-of-frame garbage is expected here: enabling RX mid-byte after a GPIO wake
            // yields a partial byte or the tail of the wake byte. The parser resyncs on the
            // next Start byte, whereas resetting RX (10ms of deafness, wake sense unarmed)
            // exactly when the real frame is inbound turns one garbled byte into a resend storm.
            BridgeDbg("BRIDGE RX unexpected byte\n");
#else
            UartLink_Reset(&uartState->core);
#endif
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

    err = UartLink_Send(&uartState->core, uartState->parser.txBuffer, uartState->parser.txPosition);
    if (err != 0) {
        k_sem_give(&uartState->core.txControlBusy);
    }

    uartState->lastMessageSentTime = k_uptime_get();
    uartState->lastLinkActivity = uartState->lastMessageSentTime;
    wakeControlThread(uartState);

    return err;
}

static void sendControl(uart_state_t *uartState, uint8_t byte) {
    UartLink_LockBusy(&uartState->core);
    int err = UartLink_Send(&uartState->core, &byte, 1);
    if (err != 0) {
        // No transfer started -> no TX_DONE -> return the slot ourselves.
        k_sem_give(&uartState->core.txControlBusy);
    }
}

// wakePeer: a nack-triggered resend skips the wake handshake, since the peer just parsed
// our garbled frame and is provably awake; a timeout-triggered one redoes it, because
// after 64ms+ of silence the peer has almost certainly slept again. This must not
// k_sleep - it runs on the control thread, where blocking makes us blind to wake edges,
// acks and pings, which used to cascade into a disconnect + BLE-fallback feedback loop.
static void resend(uart_state_t *uartState, bool wakePeer) {
    if (uartState->resendTries++ > UART_RESEND_COUNT) {
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
        int err = UartLink_Send(&uartState->core, uartState->parser.txBuffer, uartState->parser.txPosition);
        if (err != 0) {
            // No transfer started -> no TX_DONE -> return the slot ourselves.
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
            sendControl(uartState, UartControlByte_Ping);
            lastPingSentTime = currentTime;
        }

        uint32_t wakeTime = lastPingSentTime + UART_BRIDGE_PING_INTERVAL;

        if (Connections_IsReady(uartState->connectionId)) {
            switch (uartState->rxState) {
                case UartRxState_Ack:
                    sendControl(uartState, UartControlByte_Ack);
                    uartState->rxState = UartRxState_Idle;
                    break;
                case UartRxState_Nack:
                    sendControl(uartState, UartControlByte_Nack);
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
    UartParser_InitParser(&uartState->parser, &receivePacket, (void*)uartState);

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
