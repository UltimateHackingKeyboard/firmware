#ifndef _USB_DESCRIPTOR_H
#define _USB_DESCRIPTOR_H

/* Includes: */

    #include "usb_class_audio.h"
    #include "usb_device_stack_interface.h"
    #include "usb_class_composite.h"
    #include "usb_class_hid.h"

/* Macro definitions: */

    #define BCD_USB_VERSION           (0x0200)

    /* descriptors codes */
    #define USB_DEVICE_DESCRIPTOR     (1)
    #define USB_CONFIG_DESCRIPTOR     (2)
    #define USB_STRING_DESCRIPTOR     (3)
    #define USB_IFACE_DESCRIPTOR      (4)
    #define USB_ENDPOINT_DESCRIPTOR   (5)

    /* Various descriptor sizes */
    #define DEVICE_DESCRIPTOR_SIZE            (18)
    #define CONFIG_DESC_SIZE                  (59)
    #define DEVICE_QUALIFIER_DESCRIPTOR_SIZE  (10)
    #define KEYBOARD_REPORT_DESC_SIZE                  (63)
    #define MOUSE_REPORT_DESC_SIZE                  (52)

    #define CONFIG_ONLY_DESC_SIZE             (9)
    #define IFACE_ONLY_DESC_SIZE              (9)
    #define ENDP_ONLY_DESC_SIZE               (7)
    #define HEADER_ONLY_DESC_SIZE             (9)
    #define INPUT_TERMINAL_ONLY_DESC_SIZE     (12)
    #define OUTPUT_TERMINAL_ONLY_DESC_SIZE    (9)
    #define FEATURE_UNIT_ONLY_DESC_SIZE       (9)

    /* Max descriptors provided by the Application */
    #define USB_MAX_STD_DESCRIPTORS               (8)
    #define USB_MAX_CLASS_SPECIFIC_DESCRIPTORS    (2)
    /* Max configuration supported by the Application */
    #define USB_MAX_CONFIG_SUPPORTED          (1)

    /* Max string descriptors supported by the Application */
    #define USB_MAX_STRING_DESCRIPTORS        (4)

    /* Max language codes supported by the USB */
    #define USB_MAX_LANGUAGES_SUPPORTED       (1)

    #define HS_ISO_OUT_ENDP_PACKET_SIZE       (8)
    #define FS_ISO_OUT_ENDP_PACKET_SIZE       (8)
    #define HS_ISO_OUT_ENDP_INTERVAL          (0x04)
    #define FS_ISO_OUT_ENDP_INTERVAL          (0x01)

    /* string descriptors sizes */
    #define USB_STR_DESC_SIZE (2)
    #define USB_STR_0_SIZE  (2)
    #define USB_STR_1_SIZE  (56)
    #define USB_STR_2_SIZE  (36)
    #define USB_STR_n_SIZE  (32)

    /* descriptors codes */
    #define USB_DEVICE_DESCRIPTOR     (1)
    #define USB_CONFIG_DESCRIPTOR     (2)
    #define USB_STRING_DESCRIPTOR     (3)
    #define USB_IFACE_DESCRIPTOR      (4)
    #define USB_ENDPOINT_DESCRIPTOR   (5)
    #define USB_DEVQUAL_DESCRIPTOR    (6)
    #define USB_REPORT_DESCRIPTOR     (0x22)

    #define USB_MAX_SUPPORTED_INTERFACES     (1)
    #define USB_MAX_SUPPORTED_LANGUAGES     (1)
    #define CONTROL_MAX_PACKET_SIZE             (64)

    #define HID_DESC_ENDPOINT_COUNT                 (1)
    #define HID_ENDPOINT                            (2)
    #define HID_DESC_INTERFACE_COUNT                (1)
    #define HID_ONLY_DESC_SIZE                      (9)
    #define USB_HID_DESCRIPTOR                      (0x21)
    #define USB_REPORT_DESCRIPTOR                   (0x22)
    #define HS_INTERRUPT_OUT_ENDP_PACKET_SIZE       (8)
    #define FS_INTERRUPT_OUT_ENDP_PACKET_SIZE       (8)
    #define HS_INTERRUPT_OUT_ENDP_INTERVAL          (0x07) /* 2^(7-1) = 8ms */
    #define FS_INTERRUPT_OUT_ENDP_INTERVAL          (0x08)

/* Function prototypes: */

    uint8_t USB_Desc_Get_Descriptor(uint32_t handle, uint8_t type, uint8_t str_num, uint16_t index, uint8_t** descriptor, uint32_t *size);
    uint8_t USB_Desc_Get_Interface(uint32_t handle, uint8_t interface, uint8_t* alt_interface);

    uint8_t USB_Desc_Set_Interface(uint32_t handle, uint8_t interface, uint8_t alt_interface);
    bool USB_Desc_Valid_Configation(uint32_t handle, uint16_t config_val);
    bool USB_Desc_Valid_Interface(uint32_t handle, uint8_t interface);
    bool USB_Desc_Remote_Wakeup(uint32_t handle);

    usb_endpoints_t* USB_Desc_Get_Endpoints(uint32_t handle);
    uint8_t USB_Desc_Set_Speed(uint32_t handle, uint16_t speed);

#endif
