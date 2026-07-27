#include "uart_link.h"
#include "event_scheduler.h"
#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/gpio.h>
#include "config_manager.h"
#include "shared/uart_parser.h"

#define UART_RESET_DELAY 10

void UartLink_Reset(uart_link_t *uartState) {
    // This will probably not reset uart, but at least will give main thread some time to run
    uart_rx_disable(uartState->device);
    EventScheduler_Schedule(k_uptime_get() + UART_RESET_DELAY, EventSchedulerEvent_ReenableUart, "reenable uart");
}

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
        uartState->enabled = false;
        // Every RX teardown lands here, including driver-initiated ones (framing/break
        // errors are routine when RX is enabled mid-byte after a GPIO wake). Let the owner
        // re-arm RX if it wants it up - otherwise the link stays deaf.
        if (uartState->onRxDisabled != NULL) {
            uartState->onRxDisabled(uartState->onWakeArg);
        }
        break;

    case UART_RX_STOPPED:
        // reason: 1=overrun 2=parity 4=framing 8=break (uart.h uart_rx_stop_reason)
        BridgeDbg("BRIDGE RX_STOPPED reason %d\n", evt->data.rx_stop.reason);
        uartState->enabled = false;
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
#if UART_LOWPOWER
    // While RX is intentionally slept, suppress re-enables (including the ReenableUart
    // event our own uart_rx_disable schedules). UartLink_WakeRx re-enables explicitly.
    if (uartState->rxWakePin.port != NULL && uartState->lpState != UartLp_Active) {
        return;
    }
#endif
    int err = uart_rx_enable(uartState->device, uartState->rxbuf, UART_MAX_SERIALIZED_MESSAGE_LENGTH, UART_BRIDGE_TIMEOUT);
    if (err == 0) {
        uartState->enabled = true;
    } else if (err == -EBUSY) {
        // The previous uart_rx_disable is still draining, or RX is already up. Harmless
        // either way - the pending UART_RX_DISABLED event re-arms via the onRxDisabled hook.
        BridgeDbg("UART RX enable deferred (-EBUSY)\n");
    } else {
        LogU("Failed to enable UART RX because %d\n", err);
    }
}

#if UART_LOWPOWER

// One byte-time on the wire at 115200-8N1 (~87us), rounded up.
#define UART_BYTE_TIME_US 90

// Edge-sense ISR: an incoming start bit on the slept RXD pin. Deliberately minimal - it
// only kicks the owning thread and touches no lpState/gpio, so it cannot race
// SleepRx/WakeRx. It re-fires on each edge of the wake byte until WakeRx disarms it;
// the extra kicks are harmless.
static void rxWakeIsr(const struct device *port, struct gpio_callback *cb, uint32_t pins) {
    ARG_UNUSED(port);
    ARG_UNUSED(pins);
    uart_link_t *uartState = CONTAINER_OF(cb, uart_link_t, rxWakeCb);
    if (uartState->onWake != NULL) {
        uartState->onWake(uartState->onWakeArg);
    }
}

void UartLink_InitWake(uart_link_t *uartState, struct gpio_dt_spec rxPin, void (*onWake)(void*), void* onWakeArg, bool (*canSleep)(void*), void (*onRxDisabled)(void*)) {
    uartState->rxWakePin = rxPin;
    uartState->onWake = onWake;
    uartState->onWakeArg = onWakeArg;
    uartState->canSleep = canSleep;
    uartState->onRxDisabled = onRxDisabled;
    uartState->lpState = UartLp_Active;
    k_mutex_init(&uartState->lpLock);

    if (rxPin.port == NULL || !device_is_ready(rxPin.port)) {
        uartState->rxWakePin.port = NULL;
        return;
    }
    gpio_init_callback(&uartState->rxWakeCb, rxWakeIsr, BIT(rxPin.pin));
}

void UartLink_SleepRx(uart_link_t *uartState) {
    if (uartState->rxWakePin.port == NULL) {
        return;
    }

    k_mutex_lock(&uartState->lpLock, K_FOREVER);
    if (uartState->lpState != UartLp_Active) {
        k_mutex_unlock(&uartState->lpLock);
        return;
    }

    // Re-check sleep eligibility atomically with the sleep. Off-thread senders take this
    // same lpLock in WakeRx, so a send started after the owner's own check is seen here
    // and we bail instead of disabling RX from under the in-flight exchange.
    if (uartState->canSleep != NULL && !uartState->canSleep(uartState->onWakeArg)) {
        k_mutex_unlock(&uartState->lpLock);
        return;
    }

    // A frame in flight is invisible to the eligibility state: the UARTE delivers no RX
    // events during a continuous frame, so a large one (~12ms on the wire) outlives the
    // idle hold-off and sleeping here would deterministically cut its tail off. Probe the
    // line instead - activity within one byte-time means a byte is streaming right now.
    for (uint32_t probedUs = 0; probedUs < UART_BYTE_TIME_US; probedUs += 3) {
        if (gpio_pin_get_dt(&uartState->rxWakePin) > 0) {
            k_mutex_unlock(&uartState->lpLock);
            return;
        }
        k_busy_wait(3);
    }

    // Mark ourselves sleeping before the disable, so that the onRxDisabled hook that the
    // resulting UART_RX_DISABLED event runs sees this as an intentional teardown.
    uartState->lpState = UartLp_Sleeping;

    // Disable RX first, then arm the sense, back-to-back: the UARTE teardown reconfigures
    // RXD, so arming before it lets the disable clobber our pin setup. Any delay in
    // between would be a window that loses an incoming wake byte.
    uart_rx_disable(uartState->device);

    // RXD idles high; a start bit pulls it low. With GPIO_ACTIVE_LOW that is "to active".
    int e1 = gpio_pin_configure_dt(&uartState->rxWakePin, GPIO_INPUT);
    int e2 = gpio_add_callback(uartState->rxWakePin.port, &uartState->rxWakeCb);
    int e3 = gpio_pin_interrupt_configure_dt(&uartState->rxWakePin, GPIO_INT_EDGE_TO_ACTIVE);
    if (e1 != 0 || e2 != 0 || e3 != 0) {
        LogU("UART wake arm failed: cfg %d cb %d int %d\n", e1, e2, e3);
    }

    // Close the disable->arm gap: if the line is low right now, a byte is in flight and
    // its falling edge may have slipped in before the sense was armed. Treat it as a wake
    // instead of sleeping through the frame that follows it.
    if (gpio_pin_get_dt(&uartState->rxWakePin) > 0 && uartState->onWake != NULL) {
        uartState->onWake(uartState->onWakeArg);
    }

    k_mutex_unlock(&uartState->lpLock);
}

// Longest time a frame can occupy the wire: a fully escaped max-size frame is ~23ms.
#define UART_MAX_FRAME_TIME_US UART_BYTE_TIME_US*UART_MAX_SERIALIZED_MESSAGE_LENGTH

// Enabling RX mid-byte makes the UARTE sample a torn byte. So wait for the line to be continuously idle for one byte-time.
static void waitForIdleLine(uart_link_t *uartState) {
    uint32_t waitedUs = 0;

    while (waitedUs < UART_MAX_FRAME_TIME_US) {
        uint32_t idleUs = 0;
        while (idleUs < UART_BYTE_TIME_US) {
            if (gpio_pin_get_dt(&uartState->rxWakePin) > 0) {
                break;   // line low = a byte is in flight right now
            }
            k_busy_wait(3);
            idleUs += 3;
        }
        if (idleUs >= UART_BYTE_TIME_US) {
            return;
        }
        // While a frame is streaming poll coarsely rather than spin.
        k_usleep(200);
        waitedUs += 200 + idleUs;
    }
}

void UartLink_WakeRx(uart_link_t *uartState) {
    if (uartState->rxWakePin.port == NULL) {
        return;
    }

    k_mutex_lock(&uartState->lpLock, K_FOREVER);

    if (uartState->lpState != UartLp_Active) {
        gpio_pin_interrupt_configure_dt(&uartState->rxWakePin, GPIO_INT_DISABLE);
        gpio_remove_callback(uartState->rxWakePin.port, &uartState->rxWakeCb);
        uartState->lpState = UartLp_Active;
    }

    if (!uartState->enabled) {
        waitForIdleLine(uartState);
        UartLink_Enable(uartState);
    }

    k_mutex_unlock(&uartState->lpLock);
}

void UartLink_SendWakeByte(uart_link_t *uartState) {
    if (uartState->rxWakePin.port == NULL || uartState->device == NULL) {
        return;
    }

    uint8_t wake = UartControlByte_Wake;
    UartLink_LockBusy(uartState);
    int err = uart_tx(uartState->device, &wake, 1, UART_BRIDGE_TIMEOUT);
    if (err != 0) {
        k_sem_give(&uartState->txControlBusy);
    }
    // Give the peer time to bring RX up before the frame follows.
    k_usleep(UART_WAKE_DISPATCH_DELAY_US);
}

bool UartLink_IsAsleep(uart_link_t *uartState) {
    return uartState->rxWakePin.port != NULL && uartState->lpState != UartLp_Active;
}

#else // !UART_LOWPOWER

void UartLink_InitWake(uart_link_t *uartState, struct gpio_dt_spec rxPin, void (*onWake)(void*), void* onWakeArg, bool (*canSleep)(void*), void (*onRxDisabled)(void*)) {
    ARG_UNUSED(rxPin); ARG_UNUSED(onWake); ARG_UNUSED(canSleep);
    // Keep the RX auto-re-arm even with the low-power scheme compiled out: a driver-
    // initiated RX teardown otherwise leaves the link deaf forever.
    uartState->onWakeArg = onWakeArg;
    uartState->onRxDisabled = onRxDisabled;
}
void UartLink_SleepRx(uart_link_t *uartState) { ARG_UNUSED(uartState); }
void UartLink_WakeRx(uart_link_t *uartState) { UartLink_Enable(uartState); }
void UartLink_SendWakeByte(uart_link_t *uartState) { ARG_UNUSED(uartState); }
bool UartLink_IsAsleep(uart_link_t *uartState) { ARG_UNUSED(uartState); return false; }

#endif // UART_LOWPOWER


void UartLink_LockBusy(uart_link_t *uartState) {
    SEM_TAKE(&uartState->txControlBusy);
}


int UartLink_Send(uart_link_t *uartState, uint8_t* data, uint16_t len) {
    return uart_tx(uartState->device, data, len, UART_BRIDGE_TIMEOUT);
}


