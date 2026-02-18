#include "shell_transport_uhk.h"
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/serial/uart_async_rx.h>
#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/logging/log.h>
#include <string.h>
#include "logger.h"
#include "usb_log_buffer.h"
#include "wormhole.h"
#include "config_manager.h"
#include "sinks.h"
#include "macros/status_buffer.h"

#define LOG_MODULE_NAME shell_transport_uhk
LOG_MODULE_REGISTER(shell_transport_uhk);

// --- Configuration ---

#define UART_RX_BUF_COUNT 4
#define UART_RX_BUF_SIZE 16
#define UART_RX_TOTAL_SIZE (UART_RX_BUF_COUNT * \
    (UART_RX_BUF_SIZE + UART_ASYNC_RX_BUF_OVERHEAD))

// --- UART transport state ---

enum vt100_state {
    VT100_NORMAL,
    VT100_ESC_START,
    VT100_CSI_PARAMS,
};

struct uart_transport_data {
    const struct device *dev;
    shell_transport_handler_t handler;
    void *context;
    bool blocking_tx;
    bool uartEnabled;

    // TX
    struct k_sem tx_sem;

    // VT100 stripping state
    enum vt100_state vt100State;

    // Async RX
    struct uart_async_rx async_rx;
    struct uart_async_rx_config async_rx_config;
    atomic_t pending_rx_req;
    uint8_t rx_data[UART_RX_TOTAL_SIZE];
};

static struct uart_transport_data uartTransportData;

// --- UART async callbacks ---

static int uartRxEnable(const struct device *dev, uint8_t *buf, size_t len)
{
    return uart_rx_enable(dev, buf, len, 10000);
}

static void uartAsyncCallback(const struct device *dev, struct uart_event *evt, void *user_data)
{
    struct uart_transport_data *data = (struct uart_transport_data *)user_data;

    switch (evt->type) {
    case UART_TX_DONE:
        k_sem_give(&data->tx_sem);
        break;
    case UART_RX_RDY:
        uart_async_rx_on_rdy(&data->async_rx, evt->data.rx.buf, evt->data.rx.len);
        data->handler(SHELL_TRANSPORT_EVT_RX_RDY, data->context);
        break;
    case UART_RX_BUF_REQUEST:
    {
        uint8_t *buf = uart_async_rx_buf_req(&data->async_rx);
        size_t len = uart_async_rx_get_buf_len(&data->async_rx);

        if (buf) {
            int err = uart_rx_buf_rsp(dev, buf, len);
            if (err < 0) {
                uart_async_rx_on_buf_rel(&data->async_rx, buf);
            }
        } else {
            atomic_inc(&data->pending_rx_req);
        }
        break;
    }
    case UART_RX_BUF_RELEASED:
        uart_async_rx_on_buf_rel(&data->async_rx, evt->data.rx_buf.buf);
        break;
    case UART_RX_DISABLED:
        break;
    default:
        break;
    }
}

// --- Output sink routing ---

static size_t stripVt100(struct uart_transport_data *td, const uint8_t *in, size_t inLen, uint8_t *out, size_t outSize)
{
    size_t outLen = 0;

    for (size_t i = 0; i < inLen; i++) {
        uint8_t c = in[i];

        switch (td->vt100State) {
        case VT100_NORMAL:
            if (c == 0x1B) {
                td->vt100State = VT100_ESC_START;
            } else {
                if (outLen < outSize) {
                    out[outLen++] = c;
                }
            }
            break;
        case VT100_ESC_START:
            if (c == '[') {
                td->vt100State = VT100_CSI_PARAMS;
            } else {
                td->vt100State = VT100_NORMAL;
            }
            break;
        case VT100_CSI_PARAMS:
            if (c >= 0x40 && c <= 0x7E) {
                td->vt100State = VT100_NORMAL;
            }
            break;
        }
    }

    return outLen;
}

static void outputToSinks(const shell_sinks_t *sinks, struct uart_transport_data *td, const uint8_t *data, size_t length)
{
    if (!sinks->toUsbBuffer && !sinks->toOled && !sinks->toStatusBuffer) {
        return;
    }

    uint8_t stripped[48];
    size_t strippedLen = stripVt100(td, data, length, stripped, sizeof(stripped));

    if (strippedLen == 0) {
        return;
    }

    if (sinks->toUsbBuffer) {
        UsbLogBuffer_Print(stripped, strippedLen);
    }

    if (sinks->toOled) {
        LogO("%.*s", (int)strippedLen, stripped);
    }

    if (sinks->toStatusBuffer) {
        Macros_SanitizedPut((const char *)stripped, (const char *)stripped + strippedLen);
    }
}

// --- UART TX helpers ---

static int uartTxBlocking(struct uart_transport_data *data, const uint8_t *buf, size_t length)
{
    for (size_t i = 0; i < length; i++) {
        uart_poll_out(data->dev, buf[i]);
    }
    return 0;
}

static int uartTxAsync(struct uart_transport_data *data, const uint8_t *buf, size_t length)
{
    int err = uart_tx(data->dev, buf, length, SYS_FOREVER_US);
    if (err < 0) {
        return err;
    }

    return k_sem_take(&data->tx_sem, K_FOREVER);
}

static int uartTx(struct uart_transport_data *data, const uint8_t *buf, size_t length)
{
    if (data->blocking_tx) {
        return uartTxBlocking(data, buf, length);
    } else {
        return uartTxAsync(data, buf, length);
    }
}

// --- shell_transport_api implementation ---

static int uartTransportInit(const struct shell_transport *transport,
                             const void *config,
                             shell_transport_handler_t evt_handler,
                             void *context)
{
    struct uart_transport_data *data = (struct uart_transport_data *)transport->ctx;

    data->dev = (const struct device *)config;
    data->handler = evt_handler;
    data->context = context;
    data->blocking_tx = false;
    data->uartEnabled = true;

    data->async_rx_config = (struct uart_async_rx_config){
        .buffer = data->rx_data,
        .length = UART_RX_TOTAL_SIZE,
        .buf_cnt = UART_RX_BUF_COUNT,
    };

    k_sem_init(&data->tx_sem, 0, 1);

    int err = uart_async_rx_init(&data->async_rx, &data->async_rx_config);
    __ASSERT_NO_MSG(err == 0);

    uint8_t *buf = uart_async_rx_buf_req(&data->async_rx);

    err = uart_callback_set(data->dev, uartAsyncCallback, (void *)data);
    __ASSERT_NO_MSG(err == 0);

    err = uartRxEnable(data->dev, buf, uart_async_rx_get_buf_len(&data->async_rx));
    __ASSERT_NO_MSG(err == 0);

    return 0;
}

static int uartTransportUninit(const struct shell_transport *transport)
{
    struct uart_transport_data *data = (struct uart_transport_data *)transport->ctx;

    data->uartEnabled = false;

    if (data->dev) {
        uart_rx_disable(data->dev);
    }

    return 0;
}

static int uartTransportEnable(const struct shell_transport *transport, bool blocking_tx)
{
    struct uart_transport_data *data = (struct uart_transport_data *)transport->ctx;

    data->blocking_tx = blocking_tx;

    return 0;
}

static int uartTransportWrite(const struct shell_transport *transport,
                              const void *buf, size_t length, size_t *cnt)
{
    struct uart_transport_data *data = (struct uart_transport_data *)transport->ctx;
    const uint8_t *bytes = (const uint8_t *)buf;

    // Resolve and route to output sinks
    shell_sinks_t sinks = ShellConfig_GetShellSinks();
    outputToSinks(&sinks, data, bytes, length);

    // Send to UART
    if (data->uartEnabled) {
        int err = uartTx(data, bytes, length);
        if (err < 0) {
            *cnt = 0;
            return err;
        }
    }

    *cnt = length;
    data->handler(SHELL_TRANSPORT_EVT_TX_RDY, data->context);

    return 0;
}

static int uartTransportRead(const struct shell_transport *transport,
                             void *buf, size_t length, size_t *cnt)
{
    struct uart_transport_data *data = (struct uart_transport_data *)transport->ctx;
    struct uart_async_rx *async_rx = &data->async_rx;

    uint8_t *rxBuf;
    size_t blen = uart_async_rx_data_claim(async_rx, &rxBuf, length);

    memcpy(buf, rxBuf, blen);
    bool buf_available = uart_async_rx_data_consume(async_rx, blen);
    *cnt = blen;

    if (data->pending_rx_req && buf_available) {
        uint8_t *newBuf = uart_async_rx_buf_req(async_rx);
        size_t newLen = uart_async_rx_get_buf_len(async_rx);
        int err;

        __ASSERT_NO_MSG(newBuf != NULL);
        atomic_dec(&data->pending_rx_req);
        err = uart_rx_buf_rsp(data->dev, newBuf, newLen);
        // If it is too late and RX is disabled then re-enable it.
        if (err < 0) {
            if (err == -EACCES) {
                data->pending_rx_req = 0;
                err = uartRxEnable(data->dev, newBuf, newLen);
            } else {
                return err;
            }
        }
    }

    return 0;
}

// --- Transport API struct ---

static const struct shell_transport_api uartTransportApi = {
    .init = uartTransportInit,
    .uninit = uartTransportUninit,
    .enable = uartTransportEnable,
    .write = uartTransportWrite,
    .read = uartTransportRead,
};

struct shell_transport ShellUartTransport = {
    .api = &uartTransportApi,
    .ctx = &uartTransportData,
};

// --- Public UART transport uninit/reinit ---

void ShellUartTransport_Uninit(void)
{
    struct uart_transport_data *data = &uartTransportData;

    if (data->dev) {
        uart_rx_disable(data->dev);
    }

    data->uartEnabled = false;
}

void ShellUartTransport_Reinit(void)
{
    struct uart_transport_data *data = &uartTransportData;

    if (!data->dev) {
        return;
    }

    // Defensive: force-disable UART RX to handle stale state after soft reset
    uart_rx_disable(data->dev);

    // Re-register callback and re-enable RX
    int err = uart_callback_set(data->dev, uartAsyncCallback, (void *)data);
    __ASSERT_NO_MSG(err == 0);

    uint8_t *buf = uart_async_rx_buf_req(&data->async_rx);
    err = uartRxEnable(data->dev, buf, uart_async_rx_get_buf_len(&data->async_rx));
    __ASSERT_NO_MSG(err == 0);

    data->uartEnabled = true;
}
