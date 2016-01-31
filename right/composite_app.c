#include "usb_device_config.h"
#include "usb.h"
#include "usb_device_stack_interface.h"
#include "mouse.h"
#include "keyboard.h"
#include "usb_class_composite.h"
#include "composite_app.h"
#include "fsl_device_registers.h"
#include "fsl_clock_manager.h"
#include "board.h"
#include "fsl_debug_console.h"
#include "fsl_port_hal.h"
#include <stdio.h>
#include <stdlib.h>

composite_device_struct_t               g_composite_device;
uint16_t                                g_composite_speed;
extern usb_desc_request_notify_struct_t desc_callback;

extern void USB_Keyboard_App_Device_Callback(uint8_t event_type, void* val, void* arg);
extern void Hid_USB_Mouse_App_Device_Callback(uint8_t event_type, void* val, void* arg);
extern uint8_t USB_Keyboard_App_Class_Callback(
    uint8_t request, uint16_t value, uint8_t ** data, uint32_t* size, void* arg);
extern uint8_t Hid_USB_Mouse_App_Class_Callback(
    uint8_t request, uint16_t value, uint8_t ** data, uint32_t* size, void* arg);

void APP_init(void) {
    USB_PRINTF("initializing...\n");

    class_config_struct_t* keyboard_config_callback_handle;
    keyboard_config_callback_handle = &g_composite_device.composite_device_config_list[HID_KEYBOARD_INTERFACE_INDEX];
    keyboard_config_callback_handle->composite_application_callback.callback = USB_Keyboard_App_Device_Callback;
    keyboard_config_callback_handle->composite_application_callback.arg = &g_composite_device.hid_keyboard;
    keyboard_config_callback_handle->class_specific_callback.callback = USB_Keyboard_App_Class_Callback;
    keyboard_config_callback_handle->class_specific_callback.arg = &g_composite_device.hid_keyboard;
    keyboard_config_callback_handle->board_init_callback.callback = usb_device_board_init;
    keyboard_config_callback_handle->board_init_callback.arg = CONTROLLER_ID;
    keyboard_config_callback_handle->desc_callback_ptr = &desc_callback;
    keyboard_config_callback_handle->type = USB_CLASS_HID;
    OS_Mem_zero(&g_composite_device.hid_keyboard, sizeof(keyboard_global_variable_struct_t));

    /* hid mouse device */
    class_config_struct_t* mouse_config_callback_handle;
    mouse_config_callback_handle = &g_composite_device.composite_device_config_list[HID_MOUSE_INTERFACE_INDEX];
    mouse_config_callback_handle->composite_application_callback.callback = Hid_USB_Mouse_App_Device_Callback;
    mouse_config_callback_handle->composite_application_callback.arg = &g_composite_device.hid_mouse;
    mouse_config_callback_handle->class_specific_callback.callback = Hid_USB_Mouse_App_Class_Callback;
    mouse_config_callback_handle->class_specific_callback.arg = &g_composite_device.hid_mouse;
    mouse_config_callback_handle->board_init_callback.callback = usb_device_board_init;
    mouse_config_callback_handle->board_init_callback.arg = CONTROLLER_ID;
    mouse_config_callback_handle->desc_callback_ptr = &desc_callback;
    mouse_config_callback_handle->type = USB_CLASS_HID;
    OS_Mem_zero(&g_composite_device.hid_mouse, sizeof(hid_mouse_struct_t));

    g_composite_device.composite_device_config_callback.count = 2;
    g_composite_device.composite_device_config_callback.class_app_callback = g_composite_device.composite_device_config_list;

    /* Initialize the USB interface */
    USB_Composite_Init(CONTROLLER_ID, &g_composite_device.composite_device_config_callback, &g_composite_device.composite_device);

    g_composite_device.hid_keyboard.app_handle = (hid_handle_t) g_composite_device.composite_device_config_list[HID_KEYBOARD_INTERFACE_INDEX].class_handle;
    g_composite_device.hid_mouse.app_handle = (hid_handle_t) g_composite_device.composite_device_config_list[HID_MOUSE_INTERFACE_INDEX].class_handle;

    hid_keyboard_init(&g_composite_device.hid_keyboard);
    hid_mouse_init(&g_composite_device.hid_mouse);
}

void main(void) {
    hardware_init();
    OSA_Init();  // No RTOS is used but the USB device doesn't enumerate without this call.
    dbg_uart_init();
    APP_init();
    OSA_Start();  // Shouldn't be needed but OSA_Init() is needed so let's leave it in.
}
