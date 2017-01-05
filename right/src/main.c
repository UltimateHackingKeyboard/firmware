#include "main.h"
#include "init_clock.h"
#include "init_peripherials.h"
#include "usb_composite_device.h"
#include "led_driver.h"

key_matrix_t KeyMatrix = {
    .colNum = KEYBOARD_MATRIX_COLS_NUM,
    .rowNum = KEYBOARD_MATRIX_ROWS_NUM,
#if UHK_PCB_MAJOR_VERSION == 7
    .cols = (key_matrix_pin_t[]){
        {PORTA, GPIOA, kCLOCK_PortA, 5},
        {PORTB, GPIOB, kCLOCK_PortB, 16},
        {PORTB, GPIOB, kCLOCK_PortB, 17},
        {PORTB, GPIOB, kCLOCK_PortB, 18},
        {PORTB, GPIOB, kCLOCK_PortB, 19},
        {PORTA, GPIOA, kCLOCK_PortA, 1},
        {PORTB, GPIOB, kCLOCK_PortB, 1}
    },
    .rows = (key_matrix_pin_t[]){
        {PORTA, GPIOA, kCLOCK_PortA, 12},
        {PORTA, GPIOA, kCLOCK_PortA, 13},
        {PORTC, GPIOC, kCLOCK_PortC, 1},
        {PORTC, GPIOC, kCLOCK_PortC, 0},
        {PORTD, GPIOD, kCLOCK_PortD, 5}
    }
#else
    .cols = (key_matrix_pin_t[]){
        {PORTA, GPIOA, kCLOCK_PortA, 5},
        {PORTB, GPIOB, kCLOCK_PortB, 3},
        {PORTB, GPIOB, kCLOCK_PortB, 16},
        {PORTB, GPIOB, kCLOCK_PortB, 17},
        {PORTB, GPIOB, kCLOCK_PortB, 18},
        {PORTA, GPIOA, kCLOCK_PortA, 1},
        {PORTB, GPIOB, kCLOCK_PortB, 0}
    },
    .rows = (key_matrix_pin_t[]){
        {PORTA, GPIOA, kCLOCK_PortA, 12},
        {PORTA, GPIOA, kCLOCK_PortA, 13},
        {PORTC, GPIOC, kCLOCK_PortC, 0},
        {PORTB, GPIOB, kCLOCK_PortB, 19},
        {PORTD, GPIOD, kCLOCK_PortD, 6}
    }
#endif
};

uint8_t PreviousKeyStates[SLOT_COUNT][MAX_KEY_COUNT_PER_MODULE];
uint8_t CurrentKeyStates[SLOT_COUNT][MAX_KEY_COUNT_PER_MODULE];

void main() {
    InitPeripherials();
    InitClock();
    LedDriver_InitAllLeds(1);
    KeyMatrix_Init(&KeyMatrix);
    UsbKeyboadTask();
    InitUsb();

    while (1) {
        UsbKeyboadTask();
        asm("wfi");
    }
}
