#ifndef __UART_LINK_H__
#define __UART_LINK_H__

// Includes:

    #include "uart_defs.h"
    #include <zephyr/kernel.h>
    #include <zephyr/drivers/gpio.h>

// Macros:

    // Master switch for the low-power scheme: RX is disabled while the link is idle and
    // woken by a GPIO edge on RXD, senders prepend a sacrificial wake byte.
    #ifndef UART_LOWPOWER
        #define UART_LOWPOWER 1
    #endif

    #ifndef UART_BRIDGE_DEBUG
        #define UART_BRIDGE_DEBUG 0
    #endif

    #if UART_BRIDGE_DEBUG
        #define BridgeDbg(...) LogU(__VA_ARGS__)
    #else
        #define BridgeDbg(...) ((void)0)
    #endif

    // How long the initiator waits after the wake byte before sending the real frame. The
    // peer's edge -> rx-up chain takes 250-350us typically, but the control thread can be
    // preempted, with an observed tail of ~700us. Undershooting this brings the peer's RX
    // up mid-frame, which tears it down again with a framing error.
    #define UART_WAKE_DISPATCH_DELAY_US 1500

    // How long the link stays awake after any activity, so that the receiver doesn't
    // re-sleep in the wake-byte -> frame gap.
    #define UART_LP_IDLE_HOLDOFF_MS 10

// Typedefs:

    typedef enum {
        UartLp_Active = 0,   // RX enabled, normal operation
        UartLp_Sleeping,     // RX disabled, GPIO edge-sense armed on RXD
    } uart_lp_state_t;

    typedef struct {
        void (*receiveBytes)(void* state, const uint8_t* data, uint16_t len);
        void* userArg;

        const struct device *device;
        uint8_t *rxbuf;
        uint8_t rxbuf1[UART_MAX_SERIALIZED_MESSAGE_LENGTH];
        uint8_t rxbuf2[UART_MAX_SERIALIZED_MESSAGE_LENGTH];

        struct k_sem txControlBusy;
        bool enabled;

        // Low-power (UART_LOWPOWER) state
        struct gpio_dt_spec rxWakePin;      // RXD as a GPIO; .port == NULL disables LP
        struct gpio_callback rxWakeCb;
        volatile uart_lp_state_t lpState;
        // Serializes SleepRx/WakeRx across the control thread and off-thread senders. The
        // edge ISR does NOT take it - it only kicks onWake and touches no lpState/gpio.
        struct k_mutex lpLock;
        void (*onWake)(void* arg);          // called from the edge ISR to wake the owner
        void* onWakeArg;
        // Called from the UART_RX_DISABLED event (ISR context) whenever RX goes down. The
        // owner decides whether to bring it back up - it must not while intentionally
        // slept. Without this the link goes permanently deaf after a driver-initiated
        // teardown (on an RX error the nrfx driver disables RX and expects a re-enable).
        void (*onRxDisabled)(void* arg);    // shares onWakeArg
        // Sleep eligibility, re-checked under lpLock right before RX is disabled - the
        // owner's own check is otherwise a TOCTOU against off-thread senders. Gets onWakeArg.
        bool (*canSleep)(void* arg);
    } uart_link_t;

// Functions:

    void UartLink_Init(uart_link_t *uartState, const struct device* dev, void (*receiveBytes)(void* state, const uint8_t* data, uint16_t len), void* userArg);
    void UartLink_Enable(uart_link_t *uartState);
    void UartLink_Reset(uart_link_t *uartState);

    void UartLink_LockBusy(uart_link_t *uartState);
    int UartLink_Send(uart_link_t *uartState, uint8_t* data, uint16_t len);

    // Low-power API; no-ops unless UART_LOWPOWER and a wake pin is configured. SleepRx
    // disables our RX and arms the edge sense, WakeRx is the inverse (thread context only).
    void UartLink_InitWake(uart_link_t *uartState, struct gpio_dt_spec rxPin, void (*onWake)(void*), void* onWakeArg, bool (*canSleep)(void*), void (*onRxDisabled)(void*));
    void UartLink_SleepRx(uart_link_t *uartState);
    void UartLink_WakeRx(uart_link_t *uartState);
    bool UartLink_IsAsleep(uart_link_t *uartState);

    // Wake the *peer*: emit the sacrificial wake byte and wait out its RX bring-up.
    void UartLink_SendWakeByte(uart_link_t *uartState);

#endif // __UART_LINK_H__
