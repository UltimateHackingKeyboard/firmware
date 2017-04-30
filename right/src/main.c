#include "main.h"
#include "init_clock.h"
#include "init_peripherials.h"
#include "usb_composite_device.h"
#include "led_driver.h"
#include "deserialize.h"
#include "action.h"
#include "bridge_protocol_scheduler.h"
#include "peripherals/test_led.h"
#include "usb_interfaces/usb_interface_basic_keyboard.h"
#include "usb_interfaces/usb_interface_media_keyboard.h"
#include "bus_pal_hardware.h"
#include "bootloader_config.h"
#include "command.h"
#include "test_states.h"

key_matrix_t KeyMatrix = {
    .colNum = KEYBOARD_MATRIX_COLS_NUM,
    .rowNum = KEYBOARD_MATRIX_ROWS_NUM,
#if UHK_PCB_MAJOR_VERSION >= 7
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

uint8_t CurrentKeyStates[SLOT_COUNT][MAX_KEY_COUNT_PER_MODULE];

static const uint8_t testData[] =
  {0x03, 0x00, 0x01, 0x22, 0x02, 0x24, 0x02, 0x25, 0x02, 0x26, 0x02, 0x27, 0x02, 0x2d, 0x02, 0x2e, 0x02, 0x2a, 0x02,
   0x1c, 0x02, 0x18, 0x02, 0x0c, 0x02, 0x12, 0x04, 0x13, 0x22, 0x02, 0x2f, 0x02, 0x30, 0x02, 0x31, 0x02, 0x0b, 0x02,
   0x0d, 0x02, 0x0e, 0x02, 0x0f, 0x02, 0x33, 0x02, 0x34, 0x02, 0x28, 0x02, 0x11, 0x02, 0x10, 0x02, 0x36, 0x02, 0x37,
   0x02, 0x38, 0x0a, 0x03, 0x02, 0x2c, 0x02, 0x1c, 0x02, 0x1b, 0x02, 0x1a, 0x02, 0x14, 0x02, 0x1d, 0x01, 0x01, 0x1f,
   0x02, 0x35, 0x02, 0x1e, 0x02, 0x1f, 0x02, 0x20, 0x02, 0x21, 0x02, 0x22, 0x02, 0x23, 0x06, 0x29, 0x09, 0x02, 0x14,
   0x02, 0x1a, 0x02, 0x08, 0x02, 0x15, 0x02, 0x17, 0x02, 0x2b, 0x02, 0x04, 0x02, 0x16, 0x02, 0x07, 0x02, 0x09, 0x02,
   0x0a, 0x02, 0x2d, 0x02, 0x1d, 0x02, 0x1b, 0x02, 0x06, 0x02, 0x19, 0x02, 0x05, 0x02, 0x04, 0x02, 0x05, 0x02, 0x06,
   0x02, 0x07, 0x02, 0x08, 0x02, 0x09, 0x02, 0x02, 0x00 };

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

typedef struct {
  uint32_t magicNumber;
  uint8_t enumerationMode;
} wormhole_t;

wormhole_t *message_ptr __attribute__ ((__section__ (".noinit")));

void* getSP(void)
{
    void *sp;
    __asm__ __volatile__ ("mov %0, sp" : "=r"(sp));
    return sp;
}

void __attribute__ ((__section__ (".init3"), __naked__)) move_sp(void) {
    void* SP = getSP();
    SP -= sizeof(wormhole_t);
    message_ptr = (wormhole_t*)(SP + 1);
}

void main() {
    InitPeripherials();
    InitClock();
#ifdef ENABLE_BUSPAL
    init_hardware();
    handleUsbBusPalCommand();
#else
    LedDriver_InitAllLeds(1);
    InitBridgeProtocolScheduler();
    KeyMatrix_Init(&KeyMatrix);
    UpdateUsbReports();
    InitUsb();

    // deserialize_Layer(testData, 0);

    while (1) {
        UpdateUsbReports();
        asm("wfi");
    }
#endif
}
