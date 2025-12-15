#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>
#include "attributes.h"
#include "uart_bridge.h"
#include "messenger.h"
#include "messenger_queue.h"
#include "device.h"
#include "debug.h"
#include "connections.h"
#include "resend.h"
#include "pin_wiring.h"
#include "keyboard/uart_link.h"
#include "shared/uart_parser.h"
#include "uart_defs.h"

// Thread definitions

#define THREAD_STACK_SIZE 1000
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

static void setRxState(uart_state_t *uartState, uart_rx_state_t state) {
    uartState->rxState = state;
    wakeControlThread(uartState);
}


static void receivePacket(void *state, uart_control_t messageKind, const uint8_t* data, uint16_t len) {
    uart_state_t *uartState = (uart_state_t *)state;
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

                Messenger_Enqueue(connectionId, remoteDeviceId, oldPacket, len, UART_CRC_LEN);
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
            UartLink_Reset(&uartState->core);
            break;
    }
}

void UartBridge_SendMessage(message_t* msg) {
    uart_state_t *uartState = &bridgeState;

    if (uartState == NULL || uartState->core.device == NULL) {
        return;
    }

    int err;
    err = k_sem_take(&uartState->txBufferBusy, K_MSEC(UART_FOREVER_TIMEOUT));
    if (err != 0) {
        LogUOS("Uart: failed to take txBufferBusy semaphore.\n");
    }

    UartLink_LockBusy(&uartState->core);

    // Call this only after we have taken the semaphore.
    Resend_RegisterMessageAndUpdateWatermarks(msg);

    UartParser_StartMessage(&uartState->parser);

    uint8_t header[] = {msg->src, msg->dst, msg->wm};

    UartParser_AppendEscapedTxBytes(&uartState->parser, (uint8_t[]){msg->src, msg->dst, msg->wm}, sizeof(header));
    UartParser_AppendEscapedTxBytes(&uartState->parser, msg->messageId, msg->idsUsed);
    UartParser_AppendEscapedTxBytes(&uartState->parser, msg->data, msg->len);

    UartParser_FinalizeMessage(&uartState->parser);

    UartLink_Send(&uartState->core, uartState->parser.txBuffer, uartState->parser.txPosition);

    uartState->lastMessageSentTime = k_uptime_get();
    uartState->txState = UartTxState_WaitingForAck;
}

static void sendControl(uart_state_t *uartState, uint8_t byte) {
    UartLink_LockBusy(&uartState->core);
    UartLink_Send(&uartState->core, &byte, 1);
}

static void resend(uart_state_t *uartState) {
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
        k_sleep(K_MSEC(13));
        UartLink_LockBusy(&uartState->core);
        UartLink_Send(&uartState->core, uartState->parser.txBuffer, uartState->parser.txPosition);
        uartState->lastMessageSentTime = k_uptime_get();
    }
}

static void updateConnectionState(uart_state_t *uartState) {
    uint32_t pingDiff = (k_uptime_get() - uartState->lastPingTime);
    connection_id_t connectionId = uartState->connectionId;
    bool oldIsConnected = Connections_IsReady(connectionId);
    bool newIsConnected =  pingDiff < UART_BRIDGE_TIMEOUT;
    if (oldIsConnected != newIsConnected) {
        Connections_SetState(connectionId, newIsConnected ? ConnectionState_Ready : ConnectionState_Disconnected);
        k_sem_give(&uartState->txBufferBusy);
        k_sem_give(&uartState->core.txControlBusy);
    }
}

static void uartLoop(void *arg1, void *arg2, void *arg3) {
    uart_state_t *uartState = (uart_state_t *)arg1;
    uint32_t lastPingSentTime = 0;
    uint32_t currentTime = 0;
    while (1) {
        updateConnectionState(uartState);

        if (currentTime >= lastPingSentTime + UART_PING_DELAY) {
            sendControl(uartState, UartControlByte_Ping);
            lastPingSentTime = currentTime;
        }

        uint32_t wakeTime = lastPingSentTime + UART_PING_DELAY;

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
                resend(uartState);
            }

            currentTime = k_uptime_get();
            if (uartState->txState == UartTxState_WaitingForAck) {
                uint32_t resendDelay = (UART_RESEND_DELAY << uartState->resendTries);
                uint32_t resendTime = uartState->lastMessageSentTime + resendDelay;
                if (currentTime >= resendTime) {
                    LogU("Uart: didn't receive ack %d, resending (delay %d)\n", currentTime);
                    resend(uartState);
                } else {
                    wakeTime = MIN(wakeTime, resendTime);
                }
            }
        } else {
            uartState->txState = UartTxState_Idle;
            uartState->rxState = UartRxState_Idle;
        }

        currentTime = k_uptime_get();

        if (wakeTime > currentTime) {
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

    UartLink_Init(&uartState->core, device->device, UartParser_ProcessIncomingBytes, (void*)&uartState->parser);
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
    UartLink_Enable(&bridgeState.core);
}
