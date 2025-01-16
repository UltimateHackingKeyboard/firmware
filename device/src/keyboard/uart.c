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
#include "crc16.h"
#include "macros/status_buffer.h"

// Thread definitions

#define THREAD_STACK_SIZE 1000
#define THREAD_PRIORITY 5

static K_THREAD_STACK_DEFINE(stack_area, THREAD_STACK_SIZE);
static struct k_thread thread_data;

#define START_BYTE 0b1010100
#define END_BYTE 0b1010101
#define ESCAPE_BYTE 0b1010110

// UART definitions

const struct device *uart_dev = DEVICE_DT_GET(DT_NODELABEL(uart1));

#define CRC_SALT 0x1234
#define CRC_LEN 2
#define CRC_BUF_LEN CRC_LEN*2

//*2 for escapes
#define START_END_BYTE_LEN 2
#define TX_BUF_SIZE UART_MAX_PACKET_LENGTH*2+START_END_BYTE_LEN+CRC_BUF_LEN
uint8_t txBuffer[TX_BUF_SIZE];
uint16_t txPosition = 0;

#define UART_SLOTS 1
K_SEM_DEFINE(txBufferBusy, UART_SLOTS, UART_SLOTS);

#define RX_BUF_SIZE UART_MAX_PACKET_LENGTH + CRC_LEN
uint8_t* rxBuffer = NULL;
uint16_t rxPosition = 0;

#define BUF_SIZE TX_BUF_SIZE
uint8_t *rxbuf;
uint8_t rxbuf1[BUF_SIZE];
uint8_t rxbuf2[BUF_SIZE];

uint32_t lastPingTime = -2*UART_TIMEOUT;

/* UART message format:
 * [START_BYTE,crc16,escaped(messengerPacket), ENDBYTE]
 * crcMessage = 4 bytes = CRC16 in format [ESCAPE_BYTE,byte1,ESCAPE_BYTE,byte2]
 * escaped(data) = if (dataByte == escape byte or end byte) {ESCAPE_BYTE,dataByte] else [ dataByte ]
 * messengerPacket = [src, dst, messageIds ..., data ...]
 *
 * We serialize both uart-level and messenger-level packets at the same place to avoid unnecessary copying.
 * */

static void appendRxByte(uint8_t byte) {
    if (rxPosition < RX_BUF_SIZE) {
        rxBuffer[rxPosition++] = byte;
    } else {
        printk("Uart error: too long message in rx buffer, length: %i, begins with [%i, %i, ...]\n", rxPosition, rxBuffer[0], rxBuffer[1]);
    }
}

static bool isCrcValid(uint8_t* buf, uint16_t len) {
    if (len < CRC_LEN) {
        return false;
    }

    uint16_t dataLen = len - CRC_LEN;

    uint16_t crc = (buf[0] | (buf[1] << 8)) ^ CRC_SALT;

    crc16_message_t msg = {
        .length = dataLen,
        .crc = crc,
        .data = &buf[CRC_LEN]
    };

    return CRC16_IsMessageValidExt(&msg);
}


static void rxPacketReceived() {
    uint16_t len = rxPosition;

    if (len >= CRC_LEN && isCrcValid(rxBuffer, len)) {
        lastPingTime = k_uptime_get();
        len -= CRC_LEN;
    } else {
        printk("Invalid UART message received!\n");
        Macros_ReportErrorPrintf(NULL, "Invalid UART message received!");
        rxPosition = 0;
        return;
    }

    if (len == 0) {
        // ping
        lastPingTime = k_uptime_get();
    } else {
        // message
        uint8_t* oldPacket = rxBuffer;

        rxBuffer = MessengerQueue_AllocateMemory();
        rxPosition = 0;

        connection_id_t connectionId = DEVICE_IS_UHK80_LEFT ? ConnectionId_UartRight : ConnectionId_UartLeft;

        if (DEVICE_IS_UHK80_RIGHT) {
            Messenger_Enqueue(connectionId, DeviceId_Uhk80_Left, oldPacket, len, CRC_LEN);
        } else if (DEVICE_IS_UHK80_LEFT) {
            Messenger_Enqueue(connectionId, DeviceId_Uhk80_Right, oldPacket, len, CRC_LEN);
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
        case START_BYTE:
            if (escaping) {
                appendRxByte(byte);
                escaping = false;
            } else {
                rxPosition = 0;
            }
            break;
        default:
            escaping = false;
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

static void setEscapedTxByte(uint8_t idx, uint8_t byte) {
    txBuffer[idx] = ESCAPE_BYTE;
    txBuffer[idx+1] = byte;
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

static void processOutgoingByteWithCrc(uint8_t byte, crc16_data_t* crcState) {
    processOutgoingByte(byte);
    crc16_update(crcState, &byte, 1);
}

static void finalizeCrc(crc16_data_t* crcState) {
    uint16_t crc;
    crc16_finalize(crcState, &crc);
    crc = crc ^ CRC_SALT;
    setEscapedTxByte(1, crc & 0xFF);
    setEscapedTxByte(3, crc >> 8);
}

void Uart_SendPacket(const uint8_t* data, uint16_t len) {
    SEM_TAKE(&txBufferBusy);

    crc16_data_t crcState;
    crc16_init(&crcState);

    appendTxByte(START_BYTE);
    txPosition = CRC_BUF_LEN+1;

    if (len > 0) {
        processOutgoingByteWithCrc(DEVICE_ID, &crcState);
        processOutgoingByteWithCrc(DEVICE_ID == DeviceId_Uhk80_Right ? DeviceId_Uhk80_Left : DeviceId_Uhk80_Right, &crcState);
    }

    for (uint16_t i = 0; i < len; i++) {
        processOutgoingByte(data[i]);
    }
    crc16_update(&crcState, data, len);

    appendTxByte(END_BYTE);

    finalizeCrc(&crcState);

    uart_tx(uart_dev, txBuffer, txPosition, UART_TIMEOUT);
}

void Uart_SendMessage(message_t msg) {
    SEM_TAKE(&txBufferBusy);

    crc16_data_t crcState;
    crc16_init(&crcState);

    appendTxByte(START_BYTE);
    txPosition = CRC_BUF_LEN+1;

    processOutgoingByteWithCrc(msg.src, &crcState);
    processOutgoingByteWithCrc(msg.dst, &crcState);

    for (uint8_t id = 0; id < msg.idsUsed; id++) {
        processOutgoingByteWithCrc(msg.messageId[id], &crcState);
    }

    for (uint16_t i = 0; i < msg.len; i++) {
        processOutgoingByte(msg.data[i]);
    }
    crc16_update(&crcState, msg.data, msg.len);

    appendTxByte(END_BYTE);

    finalizeCrc(&crcState);

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
        k_sleep(K_MSEC(UART_PING_DELAY));
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
