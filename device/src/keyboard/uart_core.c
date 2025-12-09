#include "uart_core.h"
#include "event_scheduler.h"
#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>

#define UART_RESET_DELAY 10

void UartCore_ResetUart(uart_core_t *uartState) {
    // This will probably not reset uart, but at least will give main thread some time to run
    uart_rx_disable(uartState->device);
    EventScheduler_Schedule(k_uptime_get() + UART_RESET_DELAY, EventSchedulerEvent_ReenableUart, "reenable uart");
}

// move to uart_core
static void uart_callback(const struct device *dev, struct uart_event *evt, void *user_data) {
    uart_core_t *uartState = (uart_core_t *)user_data;
    int err;

    switch (evt->type) {
    case UART_TX_DONE:
        k_sem_give(&uartState->txControlBusy);
        break;

    case UART_TX_ABORTED:
        uart_tx(uartState->device, uartState->txBuffer, uartState->txPosition, UART_TIMEOUT);
        LogU("Tx aborted, retrying\n");
        break;

    case UART_RX_RDY:
        uartState->receiveBytes(uartState->userArg, &evt->data.rx.buf[evt->data.rx.offset], evt->data.rx.len);
        break;

    case UART_RX_BUF_REQUEST:
    {
        uartState->rxbuf = (uartState->rxbuf == uartState->rxbuf1) ? uartState->rxbuf2 : uartState->rxbuf1;

        err = uart_rx_buf_rsp(uartState->device, uartState->rxbuf, UART_CORE_BUF_SIZE);
        if (err != 0) {
            LogU("Could not provide new buffer because %i\n", err);
        }
        __ASSERT(err == 0, "Failed to provide new buffer");
        break;
    }

    case UART_RX_BUF_RELEASED:
        break;

    case UART_RX_DISABLED:
        LogU("UART_RX_DISABLED\n");
        uartState->enabled = false;
        EventScheduler_Schedule(Timer_GetCurrentTime() + 1000, EventSchedulerEvent_ReenableUart, "reenable uart");
        break;

    case UART_RX_STOPPED:
        LogU("UART_RX_STOPPED, because %d\n", evt->data.rx_stop.reason);
        uartState->enabled = false;
        EventScheduler_Schedule(Timer_GetCurrentTime() + 1000, EventSchedulerEvent_ReenableUart, "reenable uart");
        break;
    }
}

void UartCore_Init(uart_core_t *uartState, const struct device* dev, void (*receiveBytes)(void* state, const uint8_t* data, uint16_t len), void* userArg) {
    uartState->txPosition = 0;
    uartState->rxbuf = uartState->rxbuf1;
    uartState->receiveBytes = receiveBytes;
    uartState->userArg = userArg;

    k_sem_init(&uartState->txControlBusy, UART_CORE_SLOTS, UART_CORE_SLOTS);

    uartState->device = dev;

    uart_callback_set(uartState->device, uart_callback, uartState);
}


void UartCore_Enable(uart_core_t *uartState) {
    if (uartState == NULL || uartState->enabled || uartState->device == NULL) {
        return;
    }
    LogU("Enabling UART\n");
    int err = uart_rx_enable(uartState->device, uartState->rxbuf, UART_CORE_BUF_SIZE, UART_TIMEOUT);
    if (err != 0) {
        LogS("Failed to enable UART RX because %d\n", err);
    }
}


void UartCore_AppendTxByte(uart_core_t *uartState, uint8_t byte) {
    if (uartState->txPosition < UART_CORE_TX_BUF_SIZE) {
        uartState->txBuffer[uartState->txPosition++] = byte;
    } else {
        LogU("Uart error: too long message in tx buffer\n");

        uart_rx_disable(uartState->device);
        uart_rx_enable(uartState->device, uartState->rxbuf, UART_CORE_BUF_SIZE, UART_TIMEOUT);
    }
}

// Used to retroactively set crc
void UartCore_SetEscapedTxByte(uart_core_t *uartState, uint8_t idx, uint8_t byte, uint8_t escape) {
    uartState->txBuffer[idx] = escape;
    uartState->txBuffer[idx+1] = byte;
}

void UartCore_TakeControl(uart_core_t *uartState) {
    SEM_TAKE(&uartState->txControlBusy);
}

void UartCore_Send(uart_core_t *uartState, uint8_t* data, uint16_t len) {
    uart_tx(uartState->device, data, len, UART_TIMEOUT);
}


