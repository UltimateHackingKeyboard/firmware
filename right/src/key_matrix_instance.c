#include "key_matrix_instance.h"

key_matrix_t KeyMatrix = {
    .colNum = KEYBOARD_MATRIX_COLS_NUM,
    .rowNum = KEYBOARD_MATRIX_ROWS_NUM,
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
};
