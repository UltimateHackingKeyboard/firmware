#ifndef __USB_COMMAND_TOGGLE_LAYER_H__
#define __USB_COMMAND_TOGGLE_LAYER_H__

// Functions:

    void UsbCommand_ToggleLayer(void);

// Typedefs:

    typedef enum {
        UsbStatusCode_ToggleLayer_InvalidLayer = 2,
        UsbStatusCode_ToggleLayer_LayerNotDefinedByKeymap = 3,
    } usb_status_code_toggle_layer_t;

#endif
