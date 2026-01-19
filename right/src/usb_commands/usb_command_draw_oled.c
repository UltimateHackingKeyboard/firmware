#include "device.h"
#include "keyboard/oled/screens/canvas_screen.h"
#include <zephyr/sys/printk.h>
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
    uint8_t offset = 1;

    while (offset < USB_GENERIC_HID_OUT_BUFFER_LENGTH - 3) {
        uint8_t x = GenericHidOutBuffer[offset];
        if (x == 255) {
            break;  // End of data marker
        }

        uint8_t y = GenericHidOutBuffer[offset + 1];
        uint8_t pixelCount = GenericHidOutBuffer[offset + 2];
        offset += 3;

        // Payload contains 2 pixels per byte (4-bit grayscale each)
        uint8_t payloadBytes = (pixelCount + 1) / 2;

        if (offset + payloadBytes > USB_GENERIC_HID_OUT_BUFFER_LENGTH) {
            break;  // Not enough data
        }

        CanvasScreen_DrawPacked(x, y, ((uint8_t*)GenericHidOutBuffer) + offset, pixelCount);
        offset += payloadBytes;
    }
#endif
}
