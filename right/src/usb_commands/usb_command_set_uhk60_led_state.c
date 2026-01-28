#include "usb_protocol_handler.h"
#include "usb_commands/usb_command_set_uhk60_led_state.h"
#include "led_manager.h"
#include "led_display.h"

void UsbCommand_SetUhk60LedState(const uint8_t *GenericHidOutBuffer, uint8_t *GenericHidInBuffer)
{
#ifndef __ZEPHYR__
    const uint8_t *data = GenericHidOutBuffer + 1;

    // Process 6 LED values
    for (uint8_t i = 0; i < UHK60_LED_COUNT; i++) {
        uint8_t value = data[i];
        if (value != LED_VALUE_UNCHANGED) {
            Uhk60LedState.leds[i] = value;
        }
    }

    // Process 42 segment display values (3 chars * 14 segments)
    for (uint8_t i = 0; i < UHK60_SEGMENT_DISPLAY_SIZE; i++) {
        uint8_t value = data[UHK60_LED_COUNT + i];
        if (value != LED_VALUE_UNCHANGED) {
            Uhk60LedState.segments[i] = value;
        }
    }

    LedDisplay_UpdateAll();
#endif
}
