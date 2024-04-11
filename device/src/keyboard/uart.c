#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>

// Thread definitions

#define THREAD_STACK_SIZE 1000
#define THREAD_PRIORITY 5

static K_THREAD_STACK_DEFINE(stack_area, THREAD_STACK_SIZE);
static struct k_thread thread_data;

// UART definitions

const struct device *uart_dev = DEVICE_DT_GET(DT_NODELABEL(uart1));

#define BUF_SIZE 64
uint8_t *rxbuf;
uint8_t rxbuf1[BUF_SIZE];
uint8_t rxbuf2[BUF_SIZE];

static void uart_callback(const struct device *dev, struct uart_event *evt, void *user_data)
{
    int err;

    switch (evt->type) {
    case UART_TX_DONE:
        // printk("Tx sent %d bytes\n", evt->data.tx.len);
        break;

    case UART_TX_ABORTED:
        printk("Tx aborted\n");
        break;

    case UART_RX_RDY:
        printk("Received data %d bytes\n", evt->data.rx.len);
        break;

    case UART_RX_BUF_REQUEST:
    {
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

void testUart() {
    uint8_t txbuf[5] = {1, 2, 3, 4, 5};
    while (1) {
        uart_tx(uart_dev, txbuf, sizeof(txbuf), 10000);
        k_sleep(K_MSEC(1000));
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
