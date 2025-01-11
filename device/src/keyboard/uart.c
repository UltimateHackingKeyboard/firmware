#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>
#include "timer.h"
#include "uart.h"
#include "messenger.h"
#include "messenger_queue.h"
#include "device.h"
#include "device_state.h"
#include "debug.h"
#include "event_scheduler.h"
#include "connections.h"

// Thread definitions

#define THREAD_STACK_SIZE 1000
#define THREAD_PRIORITY 5

static K_THREAD_STACK_DEFINE(stack_area, THREAD_STACK_SIZE);
static struct k_thread thread_data;

#define END_BYTE 0xFE
#define ESCAPE_BYTE 0xFD

// UART definitions

const struct device *uart_dev = DEVICE_DT_GET(DT_NODELABEL(uart1));

#define TX_BUF_SIZE UART_MAX_PACKET_LENGTH*2+1
uint8_t txBuffer[TX_BUF_SIZE];
uint16_t txPosition = 0;

#define UART_SLOTS 1
K_SEM_DEFINE(txBufferBusy, UART_SLOTS, UART_SLOTS);

#define RX_BUF_SIZE UART_MAX_PACKET_LENGTH
uint8_t* rxBuffer = NULL;
uint16_t rxPosition = 0;

#define BUF_SIZE TX_BUF_SIZE
uint8_t *rxbuf;
uint8_t rxbuf1[BUF_SIZE];
uint8_t rxbuf2[BUF_SIZE];

uint32_t lastPingTime = -2*UART_TIMEOUT;

static void appendRxByte(uint8_t byte) {
    if (rxPosition < RX_BUF_SIZE) {
        rxBuffer[rxPosition++] = byte;
    } else {
        printk("Uart error: too long message in rx buffer, length: %i, begins with [%i, %i, ...]\n", rxPosition, rxBuffer[0], rxBuffer[1]);
    }
}

static void rxPacketReceived() {
    lastPingTime = k_uptime_get();
    uint16_t len = rxPosition;

    if (len == 0) {
        // we received ping
    } else {
        uint8_t* oldPacket = rxBuffer;

        rxBuffer = MessengerQueue_AllocateMemory();
        rxPosition = 0;

        connection_id_t connectionId = DEVICE_IS_UHK80_LEFT ? ConnectionId_UartRight : ConnectionId_UartLeft;

        if (DEVICE_IS_UHK80_RIGHT) {
            Messenger_Enqueue(connectionId, DeviceId_Uhk80_Left, oldPacket, len);
        } else if (DEVICE_IS_UHK80_LEFT) {
            Messenger_Enqueue(connectionId, DeviceId_Uhk80_Right, oldPacket, len);
        }
    }
}

static void processIncomingByte(uint8_t byte) {
    static bool escaping = false;
    switch (byte) {
        case END_BYTE:
            if (escaping) {
                appendRxByte(byte);
                escaping = false;
            } else {
                rxPacketReceived();
            }
            break;
        case ESCAPE_BYTE:
            if (escaping) {
                appendRxByte(byte);
                escaping = false;
            } else {
                escaping = true;
            }
            break;
        default:
            appendRxByte(byte);
            break;
    }
}

static void uart_callback(const struct device *dev, struct uart_event *evt, void *user_data) {
    int err;

    switch (evt->type) {
    case UART_TX_DONE:
        k_sem_give(&txBufferBusy);
        break;

    case UART_TX_ABORTED:
        uart_tx(uart_dev, txBuffer, txPosition, UART_TIMEOUT);
        printk("Tx aborted, retrying\n");
        break;

    case UART_RX_RDY:
        for (uint16_t i = 0; i < evt->data.rx.len; i++) {
            uint8_t byte = evt->data.rx.buf[evt->data.rx.offset+i];
            processIncomingByte(byte);
        }
        break;

    case UART_RX_BUF_REQUEST:
    {
        rxbuf = (rxbuf == rxbuf1) ? rxbuf2 : rxbuf1;

        err = uart_rx_buf_rsp(uart_dev, rxbuf, BUF_SIZE);
        if (err != 0) {
            printk("Could not provide new buffer because %i\n", err);
        }
        __ASSERT(err == 0, "Failed to provide new buffer");
        break;
    }

    case UART_RX_BUF_RELEASED:
        break;

    case UART_RX_DISABLED:
        printk("UART_RX_DISABLED\n");
        CurrentTime = k_uptime_get();
        EventScheduler_Schedule(CurrentTime + 1000, EventSchedulerEvent_ReenableUart, "reenable uart");
        break;

    case UART_RX_STOPPED:
        printk("UART_RX_STOPPED\n");
        CurrentTime = k_uptime_get();
        EventScheduler_Schedule(CurrentTime + 1000, EventSchedulerEvent_ReenableUart, "reenable uart");
        break;
    }
}


static void appendTxByte(uint8_t byte) {
    if (txPosition < TX_BUF_SIZE) {
        txBuffer[txPosition++] = byte;
    } else {
        printk("Uart error: too long message in tx buffer\n");
    }
}

static void processOutgoingByte(uint8_t byte) {
    switch (byte) {
        case END_BYTE:
            appendTxByte(ESCAPE_BYTE);
            appendTxByte(END_BYTE);
            break;
        case ESCAPE_BYTE:
            appendTxByte(ESCAPE_BYTE);
            appendTxByte(ESCAPE_BYTE);
            break;
        default:
            appendTxByte(byte);
            break;
    }
}

void Uart_SendPacket(const uint8_t* data, uint16_t len) {
    SEM_TAKE(&txBufferBusy);

    txPosition = 0;

    if (len > 0) {
        processOutgoingByte(DEVICE_ID);
        processOutgoingByte(DEVICE_ID == DeviceId_Uhk80_Right ? DeviceId_Uhk80_Left : DeviceId_Uhk80_Right);
    }

    for (uint16_t i = 0; i < len; i++) {
        processOutgoingByte(data[i]);
    }

    appendTxByte(END_BYTE);

    uart_tx(uart_dev, txBuffer, txPosition, UART_TIMEOUT);
}

void Uart_SendMessage(message_t msg) {
    SEM_TAKE(&txBufferBusy);

    txPosition = 0;

    processOutgoingByte(msg.src);
    processOutgoingByte(msg.dst);

    for (uint8_t id = 0; id < msg.idsUsed; id++) {
        processOutgoingByte(msg.messageId[id]);
    }

    for (uint16_t i = 0; i < msg.len; i++) {
        processOutgoingByte(msg.data[i]);
    }

    appendTxByte(END_BYTE);

    uart_tx(uart_dev, txBuffer, txPosition, UART_TIMEOUT);
}

static void ping() {
    Uart_SendPacket(NULL, 0);
}

static void updateConnectionState() {
    uint32_t pingDiff = (k_uptime_get() - lastPingTime);
    connection_id_t connectionId = DEVICE_IS_UHK80_LEFT ? ConnectionId_UartRight : ConnectionId_UartLeft;
    bool oldIsConnected = Connections_IsReady(connectionId);
    bool newIsConnected =  pingDiff < UART_TIMEOUT;
    if (oldIsConnected != newIsConnected) {
        Connections_SetState(connectionId, newIsConnected ? ConnectionState_Ready : ConnectionState_Disconnected);
        if (!newIsConnected) {
            k_sem_init(&txBufferBusy, UART_SLOTS, UART_SLOTS);
        }
    }
}

void testUart() {
    while (1) {
        ping();
        updateConnectionState();
        k_sleep(K_MSEC(UART_TIMEOUT/2));
    }
}

void InitUart(void) {
    rxBuffer = MessengerQueue_AllocateMemory();

    uart_callback_set(uart_dev, uart_callback, NULL);
    rxbuf = rxbuf1;
    Uart_Enable();

    k_thread_create(
        &thread_data, stack_area,
        K_THREAD_STACK_SIZEOF(stack_area),
        testUart,
        NULL, NULL, NULL,
        THREAD_PRIORITY, 0, K_NO_WAIT
    );
    k_thread_name_set(&thread_data, "test_uart");
}

bool Uart_Availability(messenger_availability_op_t operation) {
    switch (operation) {
        case MessengerAvailabilityOp_InquireOneEmpty:
            return k_sem_count_get(&txBufferBusy) > 0;
        case MessengerAvailabilityOp_InquireAllEmpty:
            return k_sem_count_get(&txBufferBusy) == UART_SLOTS;
        case MessengerAvailabilityOp_BlockTillOneEmpty:
            k_sem_take(&txBufferBusy, K_FOREVER);
            k_sem_give(&txBufferBusy);
            return true;
        default:
            return false;
    }
}

void Uart_Enable() {
    printk("Enabling UART\n");
    uart_rx_enable(uart_dev, rxbuf, BUF_SIZE, UART_TIMEOUT);
}
