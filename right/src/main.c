#include "main.h"
#include "init_clock.h"
#include "init_peripherals.h"
#include "usb_composite_device.h"
#include "led_driver.h"
#include "key_action.h"
#include "slave_scheduler.h"
#include "peripherals/test_led.h"
#include "usb_interfaces/usb_interface_basic_keyboard.h"
#include "usb_interfaces/usb_interface_media_keyboard.h"
#include "bus_pal_hardware.h"
#include "bootloader_config.h"
#include "command.h"
#include "test_states.h"
#include "wormhole.h"

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

uint8_t CurrentKeyStates[SLOT_COUNT][MAX_KEY_COUNT_PER_MODULE];

void UpdateUsbReports()
{
    if (!IsUsbBasicKeyboardReportSent) {
        return;
    }

    ResetActiveUsbBasicKeyboardReport();
    ResetActiveUsbMediaKeyboardReport();
    ResetActiveUsbSystemKeyboardReport();

    if (!TestStates.disableKeyMatrixScan) {
        KeyMatrix_Scan(&KeyMatrix);
    }

    memcpy(CurrentKeyStates[SLOT_ID_RIGHT_KEYBOARD_HALF], KeyMatrix.keyStates, MAX_KEY_COUNT_PER_MODULE);
    UpdateActiveUsbReports();

    SwitchActiveUsbBasicKeyboardReport();
    SwitchActiveUsbMediaKeyboardReport();
    SwitchActiveUsbSystemKeyboardReport();

    IsUsbBasicKeyboardReportSent = false;
}

void main() {
    InitPeripherials();
    InitClock();

    if (Wormhole.magicNumber == WORMHOLE_MAGIC_NUMBER && Wormhole.enumerationMode == EnumerationMode_BusPal) {
        Wormhole.magicNumber = 0;
        init_hardware();
        handleUsbBusPalCommand();
    } else {
        LedDriver_InitAllLeds(1);
        InitSlaveScheduler();
        KeyMatrix_Init(&KeyMatrix);
        UpdateUsbReports();
        InitUsb();

        while (1) {
            UpdateUsbReports();
            asm("wfi");
        }
    }
}
