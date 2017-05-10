#include "main.h"
#include "init_clock.h"
#include "init_peripherials.h"

key_matrix_t keyMatrix = {
    .colNum = KEYBOARD_MATRIX_COLS_NUM,
    .rowNum = KEYBOARD_MATRIX_ROWS_NUM,
#if UHK_PCB_MAJOR_VERSION >= 7
    .cols = (key_matrix_pin_t[]){
        {PORTB, GPIOB, kCLOCK_PortB, 11},
        {PORTA, GPIOA, kCLOCK_PortA, 6},
        {PORTA, GPIOA, kCLOCK_PortA, 8},
        {PORTB, GPIOB, kCLOCK_PortB, 0},
        {PORTB, GPIOB, kCLOCK_PortB, 6},
        {PORTA, GPIOA, kCLOCK_PortA, 3},
        {PORTA, GPIOA, kCLOCK_PortA, 12}
    },
    .rows = (key_matrix_pin_t[]){
        {PORTB, GPIOB, kCLOCK_PortB, 7},
        {PORTB, GPIOB, kCLOCK_PortB, 10},
        {PORTA, GPIOA, kCLOCK_PortA, 5},
        {PORTA, GPIOA, kCLOCK_PortA, 7},
        {PORTA, GPIOA, kCLOCK_PortA, 4}
    }
#else
    .cols = (key_matrix_pin_t[]){
        {PORTB, GPIOB, kCLOCK_PortB, 11},
        {PORTA, GPIOA, kCLOCK_PortA, 6},
        {PORTA, GPIOA, kCLOCK_PortA, 8},
        {PORTB, GPIOB, kCLOCK_PortB, 0},
        {PORTB, GPIOB, kCLOCK_PortB, 6},
        {PORTA, GPIOA, kCLOCK_PortA, 3},
        {PORTB, GPIOB, kCLOCK_PortB, 5}
    },
    .rows = (key_matrix_pin_t[]){
        {PORTB, GPIOB, kCLOCK_PortB, 7},
        {PORTB, GPIOB, kCLOCK_PortB, 10},
        {PORTA, GPIOA, kCLOCK_PortA, 5},
        {PORTA, GPIOA, kCLOCK_PortA, 7},
        {PORTA, GPIOA, kCLOCK_PortA, 4}
    }
#endif
};

volatile bool DisableKeyMatrixScanState;

typedef struct {
    uint32_t tag;                          // Magic number to verify bootloader configuration is valid. Must be set to 'kcfg'.
    uint32_t reserved[3];
    uint8_t enabledPeripherals;            // Bitfield of peripherals to enable.
                                           // bit 0 - LPUART, bit 1 - I2C, bit 2 - SPI, bit 3 - CAN, bit 4 - USB
    uint8_t i2cSlaveAddress;               // If not 0xFF, used as the 7-bit I2C slave address.
                                           // If 0xFF, defaults to 0x10 for I2C slave address.
    uint16_t peripheralDetectionTimeoutMs; // Timeout in milliseconds for active peripheral detection.
                                           // If 0xFFFF, defaults to 5 seconds.
    uint16_t reserved2[2];
    uint32_t reserved3;
    uint8_t clockFlags;   // The flags in the clockFlags configuration field are enabled if the corresponding bit is cleared (0).
                          // bit 0 - HighSpeed Enable high speed mode (i.e., 48 MHz).
    uint8_t clockDivider; // Inverted value of the divider to use for core and bus clocks when in high speed mode.
} bootloader_config_t;

__attribute__((section(".BootloaderConfig"))) const bootloader_config_t BootloaderConfig = {
    .tag = 0x6766636B,                    // Magic Number
    .enabledPeripherals = 0xE2,           // Enabled Peripheral: I2C
    .i2cSlaveAddress = 0x10,              // Use user-defined I2C address
    .peripheralDetectionTimeoutMs = 1000, // Use user-defined timeout (ms)
    .clockFlags = 0xFF,                   // Disable High speed mode
    .clockDivider = 0xFF,                 // Use clock divider (0)
};

void JumpToBootloader() {
    uint32_t runBootloaderAddress;
    void (*runBootloader)(void * arg);
    // Read the function address from the ROM API tree.
    runBootloaderAddress = **(uint32_t **)(0x1c00001c);
    runBootloader = (void (*)(void * arg))runBootloaderAddress;
    // Start the bootloader.
    runBootloader(NULL);
}

int main(void)
{
    InitClock();
    InitPeripherials();
    KeyMatrix_Init(&keyMatrix);

    while (1) {
        if (!DisableKeyMatrixScanState) {
            KeyMatrix_Scan(&keyMatrix);
        }
        asm("wfi");
    }
}
