#include "module.h"

pointer_delta_t PointerDelta;

key_matrix_t KeyMatrix = {
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
    },
    .keyStates = {0}
};

void Module_Init(void)
{
    KeyMatrix_Init(&KeyMatrix);
}

void Module_Loop(void)
{
}

void Module_ModuleSpecificCommand(module_specific_command_t command)
{
}

void Module_OnScan()
{
}
