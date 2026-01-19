#include "usb_commands/usb_command_set_per_key_rgb.h"
#include "usb_protocol_handler.h"
#include "usb_interfaces/usb_interface_generic_hid.h"
#include "keymap.h"
#include "layer.h"
#include "slot.h"
#include "event_scheduler.h"
#include <stdint.h>

#ifdef __ZEPHYR__
#include "state_sync.h"
#endif

void UsbCommand_SetPerKeyRgb(const uint8_t *GenericHidOutBuffer, uint8_t *GenericHidInBuffer)
{
    uint8_t keyCount = GenericHidOutBuffer[1];
    uint8_t offset = 2;

    for (uint8_t i = 0; i < keyCount; i++) {
        if (offset + 4 > USB_GENERIC_HID_OUT_BUFFER_LENGTH) {
            break;  // Not enough data
        }

        uint8_t keyId = GenericHidOutBuffer[offset];
        rgb_t rgb;
        rgb.red = GenericHidOutBuffer[offset + 1];
        rgb.green = GenericHidOutBuffer[offset + 2];
        rgb.blue = GenericHidOutBuffer[offset + 3];
        offset += 4;

        uint8_t slotIdx = keyId / 64;
        uint8_t inSlotIdx = keyId % 64;

        if (slotIdx >= SLOT_COUNT || inSlotIdx >= MAX_KEY_COUNT_PER_MODULE) {
            continue;  // Invalid key ID, skip
        }

        // Set the color on all layers
        for (uint8_t layerId = 0; layerId < LayerId_Count; layerId++) {
            CurrentKeymap[layerId][slotIdx][inSlotIdx].colorOverridden = true;
            CurrentKeymap[layerId][slotIdx][inSlotIdx].color = rgb;
        }
    }

#ifdef __ZEPHYR__
    // Sync all layers to the left half
    for (uint8_t layerId = 0; layerId < LayerId_Count; layerId++) {
        StateSync_UpdateLayer(layerId, true);
    }
#endif

    EventVector_Set(EventVector_LedMapUpdateNeeded);
}
