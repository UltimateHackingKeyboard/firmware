#include "usb_protocol_handler.h"
#include "usb_commands/usb_command_set_led_segments.h"
#include "led_display.h"

void UsbCommand_SetLedSegments_Raw() {
    LedDisplay_SetRawSegment(0, GetUsbRxBufferUint16(1));
    LedDisplay_SetRawSegment(1, GetUsbRxBufferUint16(3));
    LedDisplay_SetRawSegment(2, GetUsbRxBufferUint16(5));
}

void UsbCommand_SetLedSegments_Text() {
    char text[3];
    text[0] = GetUsbRxBufferUint8(1);
    text[1] = GetUsbRxBufferUint8(2);
    text[2] = GetUsbRxBufferUint8(3);
    LedDisplay_SetText(3, text);
}