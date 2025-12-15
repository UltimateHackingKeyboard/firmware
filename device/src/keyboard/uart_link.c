#include "uart_link.h"
#include "event_scheduler.h"
#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>

#define UART_RESET_DELAY 10

void UartLink_Reset(uart_link_t *uartState) {
    // This will probably not reset uart, but at least will give main thread some time to run
    uart_rx_disable(uartState->device);
    EventScheduler_Schedule(k_uptime_get() + UART_RESET_DELAY, EventSchedulerEvent_ReenableUart, "reenable uart");
}

// move to uart_link
static void uart_callback(const struct device *dev, struct uart_event *evt, void *user_data) {
    uart_link_t *uartState = (uart_link_t *)user_data;
    int err;

    switch (evt->type) {
    case UART_TX_DONE:
        k_sem_give(&uartState->txControlBusy);
        break;

    case UART_TX_ABORTED:
        // TODO: is this needed?
        // uart_tx(uartState->device, uartState->txBuffer, uartState->txPosition, UART_TIMEOUT);
        LogU("Tx aborted. Please report this!\n");
        break;

    case UART_RX_RDY:
        uartState->receiveBytes(uartState->userArg, &evt->data.rx.buf[evt->data.rx.offset], evt->data.rx.len);
        break;

    case UART_RX_BUF_REQUEST:
    {
        uartState->rxbuf = (uartState->rxbuf == uartState->rxbuf1) ? uartState->rxbuf2 : uartState->rxbuf1;

        err = uart_rx_buf_rsp(uartState->device, uartState->rxbuf, UART_MAX_SERIALIZED_MESSAGE_LENGTH);
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

void UartLink_Init(uart_link_t *uartState, const struct device* dev, void (*receiveBytes)(void* state, const uint8_t* data, uint16_t len), void* userArg) {
    uartState->rxbuf = uartState->rxbuf1;
    uartState->receiveBytes = receiveBytes;
    uartState->userArg = userArg;

    k_sem_init(&uartState->txControlBusy, UART_LINK_SLOTS, UART_LINK_SLOTS);

    uartState->device = dev;

    uart_callback_set(uartState->device, uart_callback, uartState);
}


void UartLink_Enable(uart_link_t *uartState) {
    if (uartState == NULL || uartState->enabled || uartState->device == NULL) {
        return;
    }
    LogU("Enabling UART\n");
    int err = uart_rx_enable(uartState->device, uartState->rxbuf, UART_MAX_SERIALIZED_MESSAGE_LENGTH, UART_BRIDGE_TIMEOUT);
    if (err != 0) {
        LogS("Failed to enable UART RX because %d\n", err);
    }
}


void UartLink_LockBusy(uart_link_t *uartState) {
    SEM_TAKE(&uartState->txControlBusy);
}


int UartLink_Send(uart_link_t *uartState, uint8_t* data, uint16_t len) {
    return uart_tx(uartState->device, data, len, UART_BRIDGE_TIMEOUT);
}


