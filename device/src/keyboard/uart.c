#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>
#include "legacy/timer.h"
#include "uart.h"

// Thread definitions

#define THREAD_STACK_SIZE 1000
#define THREAD_PRIORITY 5

static K_THREAD_STACK_DEFINE(stack_area, THREAD_STACK_SIZE);
static struct k_thread thread_data;

#define END_BYTE 0xFE
#define ESCAPE_BYTE 0xFD

// UART definitions

const struct device *uart_dev = DEVICE_DT_GET(DT_NODELABEL(uart1));

#define BUF_SIZE 64
uint8_t *rxbuf;
uint8_t rxbuf1[BUF_SIZE];
uint8_t rxbuf2[BUF_SIZE];

#define TX_BUF_SIZE UART_MAX_PACKET_LENGTH*2+1
uint8_t txBuffer[TX_BUF_SIZE];
uint16_t txPosition = 0;
K_SEM_DEFINE(txBufferBusy, 1, 1);

#define RX_BUF_SIZE UART_MAX_PACKET_LENGTH
uint8_t rxBuffer[RX_BUF_SIZE];
uint16_t rxPosition = 0;

uint32_t lastPingTime = 0;

static void appendRxByte(uint8_t byte);

static void rxPacketReceived()
{
    lastPingTime = CurrentTime;
    if (rxPosition == 0) {
        printk("UART pinged\n");
    } else {
        appendRxByte(0);
        printk("UART received: %s\n", rxBuffer);
        //TODO: process packet
    }
    rxPosition = 0;
}

static void appendRxByte(uint8_t byte)
{
    if (rxPosition < RX_BUF_SIZE) {
        rxBuffer[rxPosition++] = byte;
    } else {
        printk("Uart error: too long message in rx buffer\n");
    }
}

static void processIncomingByte(uint8_t byte)
{
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

static void uart_callback(const struct device *dev, struct uart_event *evt, void *user_data)
{
    int err;

    switch (evt->type) {
    case UART_TX_DONE:
        k_sem_give(&txBufferBusy);
        break;

    case UART_TX_ABORTED:
        uart_tx(uart_dev, txBuffer, txPosition, 10000);
        printk("Tx aborted, retrying\n");
        break;

    case UART_RX_RDY:
        for (uint8_t i = 0; i < evt->data.rx.len; i++) {
            uint8_t byte = evt->data.rx.buf[evt->data.rx.offset+i];
            processIncomingByte(byte);
        }
        break;

    case UART_RX_BUF_REQUEST:
    {
        printk("UART_RX_BUF_REQUEST\n");
        rxbuf = (rxbuf == rxbuf1) ? rxbuf2 : rxbuf1;
        err = uart_rx_buf_rsp(uart_dev, rxbuf, BUF_SIZE);
        __ASSERT(err == 0, "Failed to provide new buffer");
        break;
    }

    case UART_RX_BUF_RELEASED:
        printk("UART_RX_BUF_RELEASED\n");
        break;

    case UART_RX_DISABLED:
        printk("UART_RX_DISABLED\n");
        break;

    case UART_RX_STOPPED:
        printk("UART_RX_STOPPED\n");
        break;
    }
}


static void appendTxByte(uint8_t byte)
{
    if (txPosition < TX_BUF_SIZE) {
        txBuffer[txPosition++] = byte;
    } else {
        printk("Uart error: too long message in tx buffer\n");
    }
}

static void processOutgoingByte(uint8_t byte)
{
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
    k_sem_take(&txBufferBusy, K_FOREVER);

    txPosition = 0;

    for (uint16_t i = 0; i < len; i++) {
        processOutgoingByte(data[i]);
    }

    appendTxByte(END_BYTE);

    uart_tx(uart_dev, txBuffer, txPosition, 10000);

}

static void ping() {
    Uart_SendPacket(NULL, 0);
}

bool Uart_IsConnected() {
    return (CurrentTime - lastPingTime) < UART_TIMEOUT;
}

void testUart() {
    const char* hello = "Hello world!";
    const char* bye = "Good bye!";
    while (1) {
        ping();
        k_sleep(K_MSEC(5000));
        Uart_SendPacket((uint8_t*)hello, strlen(hello)+1);
        k_sleep(K_MSEC(5000));
        Uart_SendPacket((uint8_t*)bye, strlen(bye)+1);
        k_sleep(K_MSEC(5000));
    }
}

void InitUart(void) {
    uart_callback_set(uart_dev, uart_callback, NULL);
    rxbuf = rxbuf1;
    uart_rx_enable(uart_dev, rxbuf, BUF_SIZE, 10000);

    k_thread_create(
        &thread_data, stack_area,
        K_THREAD_STACK_SIZEOF(stack_area),
        testUart,
        NULL, NULL, NULL,
        THREAD_PRIORITY, 0, K_NO_WAIT
    );
    k_thread_name_set(&thread_data, "test_uart");
}
