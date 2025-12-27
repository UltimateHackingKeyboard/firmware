#include "leakage_test.h"
#include <stdio.h>
#include <stdint.h>
#include "fsl_gpio.h"
#include "fsl_port.h"
#include "fsl_clock.h"
#include "str_utils.h"
#include "timer.h"
#include "key_matrix.h"
#include "macros/status_buffer.h"
#include "peripherals/reset_button.h"


static inline uint32_t get_time_ms(void) {
    return Timer_GetCurrentTime();
}

static void delay(uint32_t ms) {
    uint32_t start = get_time_ms();
    while ((get_time_ms() - start) < ms) {
    }
}

uint32_t test_gpio_leakage(GPIO_Type *gpio, PORT_Type *port, uint32_t pin,
                           clock_ip_name_t clock, uint8_t in, uint32_t timeout_ms) {
    // Save original configuration
    uint32_t original_pcr = port->PCR[pin];
    uint32_t original_pddr = gpio->PDDR;
    uint32_t original_pdor = gpio->PDOR;

//     // Enable clock
//     CLOCK_EnableClock(clock);

    uint8_t out = 1 - in;

    PORT_SetPinInterruptConfig(port, pin, kPORT_InterruptOrDMADisabled);

    // Configure as output, drive high, no pull
    PORT_SetPinConfig(port, pin, &(port_pin_config_t){.pullSelect=kPORT_PullDisable, .mux=kPORT_MuxAsGpio});
    GPIO_PinInit(gpio, pin, &(gpio_pin_config_t){kGPIO_DigitalOutput, in});

    // Wait to charge any capacitance
    delay(100);

    // Configure as input with no pull
    PORT_SetPinConfig(port, pin, &(port_pin_config_t){.pullSelect=kPORT_PullDisable, .mux=kPORT_MuxAsGpio});
    GPIO_PinInit(gpio, pin, &(gpio_pin_config_t){kGPIO_DigitalInput});

    // Start timing
    uint32_t start_ms = get_time_ms();

    // Wait for pin to discharge or timeout
    while (GPIO_PinRead(gpio, pin) != out) {
        delay(1);

        uint32_t elapsed = get_time_ms() - start_ms;
        if (elapsed >= timeout_ms) {
            break;
        }
    }

    // Restore original configuration
    PORT_ClearPinsInterruptFlags(port, (1U << pin));
    port->PCR[pin] = original_pcr;
    gpio->PDDR = original_pddr;
    gpio->PDOR = original_pdor;

    return get_time_ms() - start_ms;
}

typedef struct {
    const char* function;
    char portAbbrev;
    PORT_Type *port;
    GPIO_Type *gpio;
    clock_ip_name_t clock;
    uint32_t pin;
} pin_t;

#define MERGE_SENSOR_GPIO        GPIOB
#define MERGE_SENSOR_PORT        PORTB
#define MERGE_SENSOR_CLOCK       kCLOCK_PortB
#define MERGE_SENSOR_PIN         3

static pin_t test_pins[] = {
        {"Row5", 'D', PORTD, GPIOD, kCLOCK_PortD, 5},
        {"Row3", 'C', PORTC, GPIOC, kCLOCK_PortC, 0},
        {"Row4", 'C', PORTC, GPIOC, kCLOCK_PortC, 1},
        {"Col5", 'B', PORTB, GPIOB, kCLOCK_PortB, 19},
        {"Col4", 'B', PORTB, GPIOB, kCLOCK_PortB, 18},
        {"Col3", 'B', PORTB, GPIOB, kCLOCK_PortB, 17},
        {"Col2", 'B', PORTB, GPIOB, kCLOCK_PortB, 16},
        {"MerS", 'B', PORTB, GPIOB, kCLOCK_PortB, 3},
        {"ResB", 'B', PORTB, GPIOB, kCLOCK_PortB, 2},
        {"Col7", 'B', PORTB, GPIOB, kCLOCK_PortB, 1},
        {"Row2", 'A', PORTA, GPIOA, kCLOCK_PortA, 13},
        {"Row1", 'A', PORTA, GPIOA, kCLOCK_PortA, 12},
        {"Col1", 'A', PORTA, GPIOA, kCLOCK_PortA, 5},
        {"Col6", 'A', PORTA, GPIOA, kCLOCK_PortA, 1},
};

void TestLeakage(uint32_t timeout) {
    uint32_t uptime = Timer_GetCurrentTime();
    Macros_PrintfWithPos(NULL, "GPIO Leakage Test. Do not press any keys \n");
    Macros_PrintfWithPos(NULL, "Uptime: %dh %dm %ds\n", uptime / 1000 / 3600, (uptime / 1000 / 60) % 60, (uptime / 1000) % 60);
    Macros_PrintfWithPos(NULL, "Model: %s\n", DeviceModelName(DEVICE_ID));
    DisableIRQ(RESET_BUTTON_IRQ);
    uint8_t arraySize = sizeof(test_pins)/sizeof(pin_t);
    for (uint8_t i = 0; i < arraySize; i++) {
        pin_t pin = test_pins[i];
        uint32_t discharge_time1 = test_gpio_leakage(pin.gpio, pin.port, pin.pin, pin.clock, 1, timeout);
        uint32_t discharge_time2 = test_gpio_leakage(pin.gpio, pin.port, pin.pin, pin.clock, 0, timeout);
        Macros_PrintfWithPos(NULL, "%s %c %d: %lu / %lu ms\n", pin.function, pin.portAbbrev, pin.pin, discharge_time1, discharge_time2);
    }
    EnableIRQ(RESET_BUTTON_IRQ);
    Macros_PrintfWithPos(NULL, "GPIO Leakage Test finished.\n");
}
