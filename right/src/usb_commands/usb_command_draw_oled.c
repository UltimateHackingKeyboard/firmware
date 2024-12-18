#ifdef __ZEPHYR__
#include "device.h"
#include "keyboard/oled/screens/canvas_screen.h"
#include <zephyr/sys/printk.h>
#else
#include "fsl_common.h"
#endif
#include "macros/core.h"
#include "usb_commands/usb_command_draw_oled.h"
#include "usb_interfaces/usb_interface_generic_hid.h"
#include "usb_protocol_handler.h"
#include "eeprom.h"
#include "utils.h"
#include <string.h>
#include "debug.h"
#include <stdint.h>

void UsbCommand_DrawOled(const uint8_t *GenericHidOutBuffer, uint8_t *GenericHidInBuffer)
{
#if defined(__ZEPHYR__) && DEVICE_HAS_OLED
    uint8_t x = GetUsbRxBufferUint8(1);
    uint8_t y = GetUsbRxBufferUint8(2);
    uint8_t len = GetUsbRxBufferUint8(3);

    CanvasScreen_Draw(x, y, ((uint8_t*)GenericHidOutBuffer) + 4, len);
#endif
}
