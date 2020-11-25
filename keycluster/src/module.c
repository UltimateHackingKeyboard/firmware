#include "fsl_gpio.h"
#include "fsl_port.h"
#include "module.h"

#define BLACKBERRY_TRACKBALL_LEFT_PORT PORTA
#define BLACKBERRY_TRACKBALL_LEFT_GPIO GPIOA
#define BLACKBERRY_TRACKBALL_LEFT_IRQ PORTA_IRQn
#define BLACKBERRY_TRACKBALL_LEFT_CLOCK kCLOCK_PortA
#define BLACKBERRY_TRACKBALL_LEFT_PIN 9

#define BLACKBERRY_TRACKBALL_RIGHT_PORT PORTB
#define BLACKBERRY_TRACKBALL_RIGHT_GPIO GPIOB
#define BLACKBERRY_TRACKBALL_RIGHT_IRQ PORTB_IRQn
#define BLACKBERRY_TRACKBALL_RIGHT_CLOCK kCLOCK_PortB
#define BLACKBERRY_TRACKBALL_RIGHT_PIN 6

#define BLACKBERRY_TRACKBALL_UP_PORT PORTB
#define BLACKBERRY_TRACKBALL_UP_GPIO GPIOB
#define BLACKBERRY_TRACKBALL_UP_IRQ PORTB_IRQn
#define BLACKBERRY_TRACKBALL_UP_CLOCK kCLOCK_PortB
#define BLACKBERRY_TRACKBALL_UP_PIN 13

#define BLACKBERRY_TRACKBALL_DOWN_PORT PORTA
#define BLACKBERRY_TRACKBALL_DOWN_GPIO GPIOA
#define BLACKBERRY_TRACKBALL_DOWN_IRQ PORTA_IRQn
#define BLACKBERRY_TRACKBALL_DOWN_CLOCK kCLOCK_PortA
#define BLACKBERRY_TRACKBALL_DOWN_PIN 12

pointer_delta_t PointerDelta;

key_vector_t keyVector = {
    .itemNum = KEYBOARD_VECTOR_ITEMS_NUM,
    .items = (key_vector_pin_t[]) {
        {PORTB, GPIOB, kCLOCK_PortB, 10}, // top key
        {PORTA, GPIOA, kCLOCK_PortA,  6}, // left key
        {PORTB, GPIOB, kCLOCK_PortB,  2}, // right key
        {PORTB, GPIOB, kCLOCK_PortB,  1}, // left microswitch
        {PORTB, GPIOB, kCLOCK_PortB,  7}, // trackball microswitch
        {PORTA, GPIOA, kCLOCK_PortA,  8}, // right microswitch
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
