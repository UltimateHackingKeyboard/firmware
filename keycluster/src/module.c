#include "module.h"
#include "blackberry_trackball.h"

pointer_delta_t PointerDelta;

key_vector_t keyVector = {
    .itemNum = KEYBOARD_VECTOR_ITEMS_NUM,
    .items = (key_vector_pin_t[]) {
        {PORTB, GPIOB, kCLOCK_PortB, 10}, // top key
        {PORTA, GPIOA, kCLOCK_PortA,  6}, // left key
        {PORTB, GPIOB, kCLOCK_PortB,  2}, // right key
        {PORTA, GPIOA, kCLOCK_PortA,  5}, // left microswitch
        {PORTB, GPIOB, kCLOCK_PortB,  7}, // trackball microswitch
        {PORTA, GPIOA, kCLOCK_PortA,  8}, // right microswitch
    },
};

void Module_Init(void)
{
    KeyVector_Init(&keyVector);
    BlackberryTrackball_Init();
}

void Module_Loop(void)
{
    BlackberryTrackball_Update();
}
