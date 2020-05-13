#include "module.h"
//#include "device.h"

pointer_delta_t PointerDelta;

key_matrix_t keyMatrix = {
    .colNum = KEYBOARD_MATRIX_COLS_NUM,
    .rowNum = KEYBOARD_MATRIX_ROWS_NUM,
    .cols = (key_matrix_pin_t[]) {
        {PORTB, GPIOB, kCLOCK_PortB, 11},
        {PORTA, GPIOA, kCLOCK_PortA,  6},
        {PORTA, GPIOA, kCLOCK_PortA,  8},
        {PORTB, GPIOB, kCLOCK_PortB,  0},
        {PORTB, GPIOB, kCLOCK_PortB,  6},
        {PORTA, GPIOA, kCLOCK_PortA,  3},
        {PORTA, GPIOA, kCLOCK_PortA, 12}
    },
    .rows = (key_matrix_pin_t[]) {
        {PORTB, GPIOB, kCLOCK_PortB,  7},
        {PORTB, GPIOB, kCLOCK_PortB, 10},
        {PORTA, GPIOA, kCLOCK_PortA,  5},
        {PORTA, GPIOA, kCLOCK_PortA,  7},
        {PORTA, GPIOA, kCLOCK_PortA,  4}
    }
};

void Module_Init(void)
{
/*#if DEVICE_ID == DEVICE_ID_UHK60V2
    CLOCK_EnableClock(LED_DRIVER_IICRST_CLOCK);
    PORT_SetPinMux(LED_DRIVER_IICRST_PORT, LED_DRIVER_IICRST_PIN, kPORT_MuxAsGpio);
    GPIO_PinInit(LED_DRIVER_IICRST_GPIO, LED_DRIVER_IICRST_PIN, &(gpio_pin_config_t){kGPIO_DigitalOutput, 0});
    GPIO_WritePinOutput(LED_DRIVER_IICRST_GPIO, LED_DRIVER_IICRST_PIN, 0);
#endif
*/
    KeyMatrix_Init(&keyMatrix);
}

void Module_Loop(void)
{
}
