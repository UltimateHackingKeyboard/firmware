#include "module.h"
#include "trackball.h"

pointer_delta_t PointerDelta;

key_vector_t keyVector = {
    .itemNum = KEYBOARD_VECTOR_ITEMS_NUM,
    .items = (key_vector_pin_t[]) {
        {PORTA, GPIOA, kCLOCK_PortA, 3}, // left button
        {PORTA, GPIOA, kCLOCK_PortA, 5}, // right button
    },
};

void Module_Init(void)
{
    KeyVector_Init(&keyVector);
    Trackball_Init();
}
