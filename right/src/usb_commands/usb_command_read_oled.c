#include "device.h"
#include "keyboard/oled/oled_buffer.h"
#include "keyboard/oled/framebuffer.h"
#include <zephyr/sys/printk.h>
#include "macros/core.h"
#include "usb_commands/usb_command_read_oled.h"
#include "usb_interfaces/usb_interface_generic_hid.h"
#include "usb_protocol_handler.h"
#include "eeprom.h"
#include "utils.h"
#include <string.h>
#include "debug.h"
#include <stdint.h>

void UsbCommand_ReadOled(const uint8_t *GenericHidOutBuffer, uint8_t *GenericHidInBuffer)
{
#if defined(__ZEPHYR__) && DEVICE_HAS_OLED
    uint16_t offset = GetUsbRxBufferUint16(1);

    // Calculate the maximum number of pixels we can return
    // USB buffer size minus status code and length field
    const uint16_t maxPixels = USB_GENERIC_HID_IN_BUFFER_LENGTH - 3;

    // Calculate how many pixels to read from the offset
    uint16_t pixelsToRead = DISPLAY_WIDTH * DISPLAY_HEIGHT - offset;
    if (pixelsToRead > maxPixels) {
        pixelsToRead = maxPixels;
    }

    // Set status code
    SetUsbTxBufferUint8(0, UsbStatusCode_Success);

    // Set the length of data to follow
    SetUsbTxBufferUint8(1, pixelsToRead);

    // Read pixels and store them in the response buffer
    for (uint16_t i = 0; i < pixelsToRead; i++) {
        uint16_t pixelIndex = offset + i;
        uint16_t x = pixelIndex % DISPLAY_WIDTH;
        uint16_t y = pixelIndex / DISPLAY_WIDTH;

        uint8_t pixelValue = Framebuffer_GetPixelValue(OledBuffer, x, y);
        SetUsbTxBufferUint8(2 + i, pixelValue);
    }
#else
    SetUsbTxBufferUint8(0, UsbStatusCode_InvalidCommand);
#endif
}
