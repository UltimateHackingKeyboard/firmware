#include "fsl_gpio.h"
#include "fsl_port.h"
#include "module.h"

#define BLACKBERRY_TRACKBALL_LEFT_PORT PORTE
#define BLACKBERRY_TRACKBALL_LEFT_GPIO GPIOE
#define BLACKBERRY_TRACKBALL_LEFT_IRQ PORTE_IRQn
#define BLACKBERRY_TRACKBALL_LEFT_CLOCK kCLOCK_PortE
#define BLACKBERRY_TRACKBALL_LEFT_PIN 0

#define BLACKBERRY_TRACKBALL_RIGHT_PORT PORTE
#define BLACKBERRY_TRACKBALL_RIGHT_GPIO GPIOE
#define BLACKBERRY_TRACKBALL_RIGHT_IRQ PORTE_IRQn
#define BLACKBERRY_TRACKBALL_RIGHT_CLOCK kCLOCK_PortE
#define BLACKBERRY_TRACKBALL_RIGHT_PIN 17

#define BLACKBERRY_TRACKBALL_UP_PORT PORTE
#define BLACKBERRY_TRACKBALL_UP_GPIO GPIOE
#define BLACKBERRY_TRACKBALL_UP_IRQ PORTE_IRQn
#define BLACKBERRY_TRACKBALL_UP_CLOCK kCLOCK_PortE
#define BLACKBERRY_TRACKBALL_UP_PIN 1

#define BLACKBERRY_TRACKBALL_DOWN_PORT PORTE
#define BLACKBERRY_TRACKBALL_DOWN_GPIO GPIOE
#define BLACKBERRY_TRACKBALL_DOWN_IRQ PORTE_IRQn
#define BLACKBERRY_TRACKBALL_DOWN_CLOCK kCLOCK_PortE
#define BLACKBERRY_TRACKBALL_DOWN_PIN 16

pointer_delta_t PointerDelta;

key_vector_t keyVector = {
    .itemNum = KEYBOARD_VECTOR_ITEMS_NUM,
    .items = (key_vector_pin_t[]) {
        {PORTC, GPIOC, kCLOCK_PortC,  5}, // top key
        {PORTC, GPIOC, kCLOCK_PortC,  6}, // left key
        {PORTD, GPIOD, kCLOCK_PortD,  7}, // right key
        {PORTC, GPIOC, kCLOCK_PortC,  7}, // left microswitch
        {PORTA, GPIOA, kCLOCK_PortA, 19}, // trackball microswitch
        {PORTD, GPIOD, kCLOCK_PortD,  6}, // right microswitch
    },
};

void BlackberryTrackball_Init(void)
{
    CLOCK_EnableClock(BLACKBERRY_TRACKBALL_LEFT_CLOCK);
    PORT_SetPinConfig(BLACKBERRY_TRACKBALL_LEFT_PORT, BLACKBERRY_TRACKBALL_LEFT_PIN,
        &(port_pin_config_t){.pullSelect=kPORT_PullUp, .mux=kPORT_MuxAsGpio});

    CLOCK_EnableClock(BLACKBERRY_TRACKBALL_RIGHT_CLOCK);
    PORT_SetPinConfig(BLACKBERRY_TRACKBALL_RIGHT_PORT, BLACKBERRY_TRACKBALL_RIGHT_PIN,
        &(port_pin_config_t){.pullSelect=kPORT_PullUp, .mux=kPORT_MuxAsGpio});

    CLOCK_EnableClock(BLACKBERRY_TRACKBALL_UP_CLOCK);
    PORT_SetPinConfig(BLACKBERRY_TRACKBALL_UP_PORT, BLACKBERRY_TRACKBALL_UP_PIN,
        &(port_pin_config_t){.pullSelect=kPORT_PullUp, .mux=kPORT_MuxAsGpio});

    CLOCK_EnableClock(BLACKBERRY_TRACKBALL_DOWN_CLOCK);
    PORT_SetPinConfig(BLACKBERRY_TRACKBALL_DOWN_PORT, BLACKBERRY_TRACKBALL_DOWN_PIN,
        &(port_pin_config_t){.pullSelect=kPORT_PullUp, .mux=kPORT_MuxAsGpio});
}

bool oldLeft, oldRight, oldUp, oldDown;

void BlackberryTrackball_Update(void)
{
    uint8_t newLeft = GPIO_ReadPinInput(BLACKBERRY_TRACKBALL_LEFT_GPIO, BLACKBERRY_TRACKBALL_LEFT_PIN);
    if (oldLeft != newLeft) {
        PointerDelta.x--;
        oldLeft = newLeft;
    }

    uint8_t newRight = GPIO_ReadPinInput(BLACKBERRY_TRACKBALL_RIGHT_GPIO, BLACKBERRY_TRACKBALL_RIGHT_PIN);
    if (oldRight != newRight) {
        PointerDelta.x++;
        oldRight = newRight;
    }

    uint8_t newUp = GPIO_ReadPinInput(BLACKBERRY_TRACKBALL_UP_GPIO, BLACKBERRY_TRACKBALL_UP_PIN);
    if (oldUp != newUp) {
        PointerDelta.y--;
        oldUp = newUp;
    }

    uint8_t newDown = GPIO_ReadPinInput(BLACKBERRY_TRACKBALL_DOWN_GPIO, BLACKBERRY_TRACKBALL_DOWN_PIN);
    if (oldDown != newDown) {
        PointerDelta.y++;
        oldDown = newDown;
    }
}

void Module_Init(void)
{
    KeyVector_Init(&keyVector);
    BlackberryTrackball_Init();
}

void Module_Loop(void)
{
    BlackberryTrackball_Update();
}
