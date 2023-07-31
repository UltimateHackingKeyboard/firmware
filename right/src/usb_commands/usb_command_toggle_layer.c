#include "layer.h"
#include "layer_switcher.h"
#include "usb_protocol_handler.h"
#include "usb_commands/usb_command_toggle_layer.h"
#include "keymap.h"
#include "debug.h"

void UsbCommand_ToggleLayer(void)
{
    layer_id_t layerId = GetUsbRxBufferUint8(1);

    if (!LayerConfig[layerId].layerIsDefined) {
        SetUsbTxBufferUint8(0, UsbStatusCode_ToggleLayer_LayerNotDefinedByKeymap);
    } else if (layerId >= LayerId_Count || layerId == LayerId_Base) {
        SetUsbTxBufferUint8(0, UsbStatusCode_ToggleLayer_InvalidLayer);
    } else {
        LayerSwitcher_ToggleLayer(layerId);
    }
}
