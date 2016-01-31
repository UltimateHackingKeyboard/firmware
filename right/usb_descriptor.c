#include "usb_device_config.h"
#include "usb.h"
#include "usb_device_stack_interface.h"
#include "usb_descriptor.h"
#include "mouse.h"
#include "composite_app.h"

///////////////////////////////// Keyboard endpoint and interface /////////////////////////////////

usb_ep_struct_t keyboard_endpoint[HID_DESC_ENDPOINT_COUNT] =
{
    {
        HID_ENDPOINT,
        USB_INTERRUPT_PIPE,
        USB_SEND,
        FS_INTERRUPT_OUT_ENDP_PACKET_SIZE,
    }
};

usb_endpoints_t keyboard_endpoints =
{
    HID_DESC_ENDPOINT_COUNT,
    keyboard_endpoint
};

static usb_if_struct_t keyboard_interface[1];

/////////////////////////////////// Mouse endpoint and interface ///////////////////////////////////

usb_ep_struct_t mouse_endpoint[HID_DESC_ENDPOINT_COUNT] =
{
    {
        HID_ENDPOINT,
        USB_INTERRUPT_PIPE,
        USB_SEND,
        FS_INTERRUPT_OUT_ENDP_PACKET_SIZE,
    }
};

usb_endpoints_t mouse_endpoints =
{
    HID_DESC_ENDPOINT_COUNT,
    mouse_endpoint
};

static usb_if_struct_t mouse_interface[1];

////////////////////////////////////// USB class descriptors //////////////////////////////////////

usb_class_struct_t usb_interfaces[2] =
{
    {
        USB_CLASS_HID,
        {
            1,
            keyboard_interface
        }
    },
    {
        USB_CLASS_HID,
        {
            1,
            mouse_interface
        }
    },
};

static usb_composite_info_struct_t usb_composite_info =
{
    2,
    usb_interfaces
};

uint8_t g_device_descriptor[DEVICE_DESCRIPTOR_SIZE] =
{
    DEVICE_DESCRIPTOR_SIZE,               /* "Device Descriptor Size        */
    USB_DEVICE_DESCRIPTOR,                /* "Device" Type of descriptor    */
    0x00,0x02,                            /*  BCD USB version               */
    0x00,                                 /*  Device Class is indicated in
                                             the interface descriptors     */
    0x00,                                 /*  Device Subclass is indicated
                                             in the interface descriptors  */
    0x00,                                 /*  Device Protocol               */
    CONTROL_MAX_PACKET_SIZE,              /*  Max Packet size               */
    0xa2,0x15,                            /*  Vendor ID   */
    0x00, 0x02,                           /*  Product ID  */
    0x00, 0x02,                           /*  BCD Device version */
    0x01,                                 /*  Manufacturer string index     */
    0x02,                                 /*  Product string index          */
    0x00,                                 /*  Serial number string index    */
    0x01                                  /*  Number of configurations      */
};

uint8_t g_config_descriptor[CONFIG_DESC_SIZE] =
{
    CONFIG_ONLY_DESC_SIZE,           /*  Configuration Descriptor Size - always 9 bytes*/
    USB_CONFIG_DESCRIPTOR,           /* "Configuration" type of descriptor */
    CONFIG_DESC_SIZE, 0x00,          /*  Total length of the Configuration descriptor */
    0x02,                            /*  NumInterfaces */
    0x01,                            /*  Configuration Value */
    0,                               /*  Configuration Description String Index*/
    /*  Attributes.support RemoteWakeup and self power */
    (USB_DESC_CFG_ATTRIBUTES_D7_POS) | (USBCFG_DEV_SELF_POWER << USB_DESC_CFG_ATTRIBUTES_SELF_POWERED_SHIFT) | (USBCFG_DEV_REMOTE_WAKEUP << USB_DESC_CFG_ATTRIBUTES_REMOTE_WAKEUP_SHIFT),
    /* S08/CFv1 are both self powered (its compulsory to set bus powered)*/
    /*Attributes.support RemoteWakeup and self power*/
    0x32,                            /*  Current draw from bus */

    /* Keyboard Interface Descriptor */
    IFACE_ONLY_DESC_SIZE,
    USB_IFACE_DESCRIPTOR,
    0x00,
    0x00,
    HID_DESC_ENDPOINT_COUNT,
    0x03,
    0x01,
    0x01, /* 0x01 for keyboard */
    0x00,

    /* Keyboard HID descriptor */
    HID_ONLY_DESC_SIZE,
    USB_HID_DESCRIPTOR,
    0x00, 0x01,
    0x00,
    0x01,
    0x22,
    0x3F, 0x00,

    /* Keyboard Endpoint descriptor */
    ENDP_ONLY_DESC_SIZE,
    USB_ENDPOINT_DESCRIPTOR,
    HID_ENDPOINT |(USB_SEND << 7),
    USB_INTERRUPT_PIPE,
    FS_INTERRUPT_OUT_ENDP_PACKET_SIZE, 0x00,
    FS_INTERRUPT_OUT_ENDP_INTERVAL,

    /* Mouse Interface Descriptor */
    IFACE_ONLY_DESC_SIZE,
    USB_IFACE_DESCRIPTOR,
    0x00,
    0x00,
    HID_DESC_ENDPOINT_COUNT,
    0x03,
    0x01,
    0x02,
    0x00,

    /* Mouse HID descriptor */
    HID_ONLY_DESC_SIZE,
    USB_HID_DESCRIPTOR,
    0x00,0x01,
    0x00,
    0x01,
    0x22,
    0x34,0x00,

    /* Mouse Endpoint descriptor */
    ENDP_ONLY_DESC_SIZE,
    USB_ENDPOINT_DESCRIPTOR,
    HID_ENDPOINT | (USB_SEND << 7),
    USB_INTERRUPT_PIPE,
    FS_INTERRUPT_OUT_ENDP_PACKET_SIZE, 0x00,
    FS_INTERRUPT_OUT_ENDP_INTERVAL
};

uint8_t USB_STR_0[USB_STR_0_SIZE+USB_STR_DESC_SIZE] =
{
    sizeof(USB_STR_0),
    USB_STRING_DESCRIPTOR,
    0x09,
    0x04/*equivalent to 0x0409*/
};

uint8_t USB_STR_1[USB_STR_1_SIZE+USB_STR_DESC_SIZE] =
{
    sizeof(USB_STR_1),
    USB_STRING_DESCRIPTOR,
    'F',0,
    'R',0,
    'E',0,
    'E',0,
    'S',0,
    'C',0,
    'A',0,
    'L',0,
    'E',0,
    ' ',0,
    'S',0,
    'E',0,
    'M',0,
    'I',0,
    'C',0,
    'O',0,
    'N',0,
    'D',0,
    'U',0,
    'C',0,
    'T',0,
    'O',0,
    'R',0,
    ' ',0,
    'I',0,
    'N',0,
    'C',0,
    '.',0
};

uint8_t USB_STR_2[USB_STR_2_SIZE+USB_STR_DESC_SIZE] =
{
    sizeof(USB_STR_2),
    USB_STRING_DESCRIPTOR,
    'U',0,
    'S',0,
    'B',0,
    ' ',0,
    'A',0,
    'U',0,
    'D',0,
    'I',0,
    'O',0,
    '+',0,
    'H',0,
    'I',0,
    'D',0,
    ' ',0,
    'D',0,
    'E',0,
    'M',0,
    'O',0,
};

uint8_t USB_STR_n[USB_STR_n_SIZE+USB_STR_DESC_SIZE] =
{
    sizeof(USB_STR_n),
    USB_STRING_DESCRIPTOR,
    'B',0,
    'A',0,
    'D',0,
    ' ',0,
    'S',0,
    'T',0,
    'R',0,
    'I',0,
    'N',0,
    'G',0,
    ' ',0,
    'I',0,
    'N',0,
    'D',0,
    'E',0,
    'X',0
};

uint8_t keyboard_hid_report_descriptor[KEYBOARD_REPORT_DESC_SIZE] =
{
    0x05, 0x01, /* USAGE_PAGE (Generic Desktop) */
    0x09, 0x06, /* USAGE (Keyboard) */
    0xa1, 0x01, /* COLLECTION (Application) */
    0x05, 0x07, /*   USAGE_PAGE (Keyboard) */
    0x19, 0xe0, /*   USAGE_MINIMUM (Keyboard LeftControl) */
    0x29, 0xe7, /*   USAGE_MAXIMUM (Keyboard Right GUI) */
    0x15, 0x00, /*   LOGICAL_MINIMUM (0) */
    0x25, 0x01, /*   LOGICAL_MAXIMUM (1) */
    0x75, 0x01, /*   REPORT_SIZE (1) */
    0x95, 0x08, /*   REPORT_COUNT (8) */
    0x81, 0x02, /*   INPUT (Data,Var,Abs) modifier keys (CTRL, ALT, etc...*/
    0x95, 0x01, /*   REPORT_COUNT (1) */
    0x75, 0x08, /*   REPORT_SIZE (8) */
    0x81, 0x01, /*   INPUT (Cnst,Var,Abs) filupp to byte boundary */
    0x95, 0x05, /*   REPORT_COUNT (5) */
    0x75, 0x01, /*   REPORT_SIZE (1) */
    0x05, 0x08, /*   USAGE_PAGE (LEDs) */
    0x19, 0x01, /*   USAGE_MINIMUM (Num Lock) */
    0x29, 0x05, /*   USAGE_MAXIMUM (Kana) */
    0x91, 0x02, /*   OUTPUT (Data,Var,Abs) pc->kbd */
    0x95, 0x01, /*   REPORT_COUNT (1) */
    0x75, 0x03, /*   REPORT_SIZE (3 */
    0x91, 0x01, /*   OUTPUT (Cnst,Var,Abs) filupp to byte boundary */
    0x95, 0x06, /*   REPORT_COUNT (6) */
    0x75, 0x08, /*   REPORT_SIZE (8) */
    0x15, 0x00, /*   LOGICAL_MINIMUM (0) */
    0x25, 0x65, /*   LOGICAL_MAXIMUM (101) */
    0x05, 0x07, /*   USAGE_PAGE (Keyboard) */
    0x19, 0x00, /*   USAGE_MINIMUM (Reserved (no event indicated)) */
    0x29, 0x65, /*   USAGE_MAXIMUM (Keyboard Application) */
    0x81, 0x00, /*   INPUT (Data,Ary,Abs) array for pressed keys */
    0xc0 /* END_COLLECTION */
};

uint8_t mouse_hid_report_descriptor[MOUSE_REPORT_DESC_SIZE] =
{
    0x05, 0x01,   /* Usage Page (Generic Desktop)*/
    0x09, 0x02,   /* Usage (Mouse) */
    0xA1, 0x01,   /* Collection (Application) */
    0x09, 0x01,   /* Usage (Pointer) */

    0xA1, 0x00,   /* Collection (Physical) */
    0x05, 0x09,   /* Usage Page (Buttons) */
    0x19, 0x01,   /* Usage Minimum (01) */
    0x29, 0x03,   /* Usage Maximum (03) */

    0x15, 0x00,   /* logical Minimum (0) */
    0x25, 0x01,   /* logical Maximum (1) */
    0x95, 0x03,   /* Report Count (3) */
    0x75, 0x01,   /* Report Size (1) */

    0x81, 0x02,   /* Input(Data, Variable, Absolute) 3 button bits */
    0x95, 0x01,   /* Report count (1) */
    0x75, 0x05,   /* Report Size (5) */
    0x81, 0x01,   /* Input (Constant), 5 bit padding */

    0x05, 0x01,   /* Usage Page (Generic Desktop) */
    0x09, 0x30,   /* Usage (X) */
    0x09, 0x31,   /* Usage (Y) */
    0x09, 0x38,   /* Usage (Z) */

    0x15, 0x81,   /* Logical Minimum (-127) */
    0x25, 0x7F,   /* Logical Maximum (127) */
    0x75, 0x08,   /* Report Size (8) */
    0x95, 0x03,   /* Report Count (2) */

    0x81, 0x06,   /* Input(Data, Variable, Relative), 2 position bytes (X & Y)*/
    0xC0,         /* end collection */
    0xC0          /* end collection */
};

uint32_t g_std_desc_size[USB_MAX_STD_DESCRIPTORS+1] =
{
    0,
    DEVICE_DESCRIPTOR_SIZE,
    CONFIG_DESC_SIZE,
    0, /* String */
    0, /* Interface */
    0, /* Endpoint */
    0, /* Device Qualifier */
    0 /* other speed config */
};

uint8_t *g_std_descriptors[USB_MAX_STD_DESCRIPTORS+1] =
{
    NULL,
    g_device_descriptor,
    g_config_descriptor,
    NULL, /* String */
    NULL, /* Interface */
    NULL, /* Endpoint */
    NULL, /* Device Qualifier */
    NULL /* Other speed config */
};

uint8_t g_string_desc_size[USB_MAX_STRING_DESCRIPTORS] =
{
    sizeof(USB_STR_0),
    sizeof(USB_STR_1),
    sizeof(USB_STR_2),
    sizeof(USB_STR_n)
};

uint8_t *g_string_descriptors[USB_MAX_STRING_DESCRIPTORS+1] =
{
    USB_STR_0,
    USB_STR_1,
    USB_STR_2,
    USB_STR_n
};

usb_language_t usb_lang[USB_MAX_SUPPORTED_LANGUAGES] =
{
    {
       (uint16_t)0x0409,
       g_string_descriptors,
       g_string_desc_size
    }
};

usb_all_languages_t g_languages =
{
   USB_STR_0, sizeof(USB_STR_0),
   USB_MAX_SUPPORTED_LANGUAGES,
   usb_lang
};

uint8_t g_valid_config_values[USB_MAX_CONFIG_SUPPORTED+1]={0,1};

/****************************************************************************
 * Global Variables
 ****************************************************************************/
static uint8_t g_alternate_interface[USB_MAX_SUPPORTED_INTERFACES];

/**************************************************************************//*!
 *
 * @name  USB_Desc_Get_Descriptor
 *
 * @brief The function returns the corresponding descriptor
 *
 * @param controller_ID : Controller ID
 * @param type          : Type of descriptor requested
 * @param sub_type      : String index for string descriptor
 * @param index         : String descriptor language Id
 * @param descriptor    : Output descriptor pointer
 * @param size          : Size of descriptor returned
 *
 * @return USB_OK                              When Success
 *         USBERR_INVALID_REQ_TYPE             when Error
 *****************************************************************************
 * This function is used to pass the pointer of the requested descriptor
 *****************************************************************************/
uint8_t USB_Desc_Get_Descriptor
(
    uint32_t handle,
    uint8_t type,
    uint8_t str_num,
    uint16_t index,
    uint8_t** descriptor,
    uint32_t* size
)
{
    /* string descriptors are handled separately */
    if (type == USB_STRING_DESCRIPTOR) {
        if (index == 0) {
            /* return the string and size of all languages */
            *descriptor = (uint8_t *)g_languages.languages_supported_string;
            *size = g_languages.languages_supported_size;
        } else {
            uint8_t lang_id=0;
            uint8_t lang_index=USB_MAX_LANGUAGES_SUPPORTED;

            for (; lang_id< USB_MAX_LANGUAGES_SUPPORTED; lang_id++) {
                /* check whether we have a string for this language */
                if (index == g_languages.usb_language[lang_id].language_id) {
                    /* check for max descriptors */
                    if (str_num < USB_MAX_STRING_DESCRIPTORS) {
                        /* setup index for the string to be returned */
                        lang_index = str_num;
                    }
                    break;
                }
            }
            /* set return val for descriptor and size */
            *descriptor = (uint8_t *)g_languages.usb_language[lang_id].lang_desc[str_num];
            *size = g_languages.usb_language[lang_id].lang_desc_size[lang_index];
        }
    } else if (type < USB_MAX_STD_DESCRIPTORS+1) {
        /* set return val for descriptor and size*/
        *descriptor = (uint8_t *)g_std_descriptors[type];

        /* if there is no descriptor then return error */
        if (*descriptor == NULL) {
            return USBERR_INVALID_REQ_TYPE;
        }
        *size = g_std_desc_size[type];
    } else /* invalid descriptor */ {
        if (type == USB_REPORT_DESCRIPTOR) {
            type = USB_MAX_STD_DESCRIPTORS;
            *descriptor = (uint8_t *)g_std_descriptors[type];
            *size = g_std_desc_size[type];
        } else {
            return USBERR_INVALID_REQ_TYPE;
        }
    }

    return USB_OK;
}

/**************************************************************************//*!
 *
 * @name  USB_Desc_Get_Interface
 *
 * @brief The function returns the alternate interface
 *
 * @param controller_ID : Controller ID
 * @param interface     : Interface number
 * @param alt_interface : Output alternate interface
 *
 * @return USB_OK                              When Success
 *         USBERR_INVALID_REQ_TYPE             when Error
 *****************************************************************************
 *This function is called by the framework module to get the current interface
 *****************************************************************************/
uint8_t USB_Desc_Get_Interface(uint32_t handle, uint8_t interface, uint8_t *alt_interface) {
    if (interface < USB_MAX_SUPPORTED_INTERFACES) { /* if interface valid */
        /* get alternate interface */
        *alt_interface = g_alternate_interface[interface];
        return USB_OK;
    }
    return USBERR_INVALID_REQ_TYPE;
}

/**************************************************************************//*!
 *
 * @name  USB_Desc_Set_Interface
 *
 * @brief The function sets the alternate interface
 *
 * @param handle        : handle
 * @param interface     : Interface number
 * @param alt_interface : Input alternate interface
 *
 * @return USB_OK                              When Success
 *         USBERR_INVALID_REQ_TYPE             when Error
 *****************************************************************************
 *This function is called by the framework module to set the interface
 *****************************************************************************/
uint8_t USB_Desc_Set_Interface(uint32_t handle, uint8_t interface, uint8_t alt_interface) {
   if (interface < USB_MAX_SUPPORTED_INTERFACES) { /* if interface valid */
      /* set alternate interface*/
      g_alternate_interface[interface] = alt_interface;
      return USB_OK;
   }
   return USBERR_INVALID_REQ_TYPE;
}

/**************************************************************************//*!
 *
 * @name  USB_Desc_Valid_Configation
 *
 * @brief The function checks whether the configuration parameter
 *        input is valid or not
 *
 * @param handle        : handle
 * @param config_val    : Configuration value
 *
 * @return TRUE           When Valid
 *         FALSE          When Error
 *****************************************************************************
 * This function checks whether the configuration is valid or not
 *****************************************************************************/
bool USB_Desc_Valid_Configation
(
   uint32_t handle,/*[IN] Controller ID */
   uint16_t config_val   /*[IN] Configuration value */
)
{
   uint8_t loop_index=0;

   UNUSED_ARGUMENT(handle)

   /* check with only supported val right now */
   while(loop_index < (USB_MAX_CONFIG_SUPPORTED+1)) {
      if(config_val == g_valid_config_values[loop_index]) {
         return TRUE;
      }
      loop_index++;
   }
   return FALSE;
}
/**************************************************************************//*!
 *
 * @name  USB_Desc_Valid_Interface
 *
 * @brief The function checks whether the interface parameter
 *        input is valid or not
 *
 * @param handle        : handle
 * @param interface     : Target interface
 *
 * @return TRUE           When Valid
 *         FALSE          When Error
 *****************************************************************************
 * This function checks whether the interface is valid or not
 *****************************************************************************/
bool USB_Desc_Valid_Interface
(
   uint32_t handle,       /*[IN] Controller ID */
   uint8_t interface      /*[IN] Target interface */
)
{
   uint8_t loop_index=0;
   UNUSED_ARGUMENT(handle)
   /* check with only supported val right now */
   while(loop_index < USB_MAX_SUPPORTED_INTERFACES) {
      if(interface == g_alternate_interface[loop_index]) {
          return TRUE;
      }
      loop_index++;
   }
   return FALSE;
}
/**************************************************************************//*!
 *
 * @name  USB_Desc_Remote_Wakeup
 *
 * @brief The function checks whether the remote wakeup is supported or not
 *
 * @param handle : handle
 *
 * @return REMOTE_WAKEUP_SUPPORT (TRUE) - If remote wakeup supported
 *****************************************************************************
 * This function returns remote wakeup is supported or not
 *****************************************************************************/
bool USB_Desc_Remote_Wakeup(uint32_t handle      /* [IN] Controller ID */) {
   return USBCFG_DEV_REMOTE_WAKEUP;
}

/* ****************************************************************************
 * Local available
 * Stock Array value of Parameter controller
 ******************************************************************************/

/**************************************************************************//*!
 *
 * @name  USB_Set_Configuration
 *
 * @brief The function set the configuration value of device
 *
 *
 * @param handle          handle
 * @param config_val      configuration value
 *
 * @return TRUE           When Valid
 *         FALSE          When Error
 *****************************************************************************/
uint8_t USB_Set_Configuration(audio_handle_t handle, uint8_t config) {
    return USB_OK; /* It's that simple since the device only has one configuration. */
}

/**************************************************************************//*!
 *
 * @name  USB_Desc_Get_Entity
 *
 * @brief The function retrieves the entity specified by type.
 *
 * @param handle            handle
 *
 * @return USB_OK  - if success
 *****************************************************************************/
uint8_t USB_Desc_Get_Entity(audio_handle_t handle, entity_type type, uint32_t *object)
{
    switch (type) {
        case USB_CLASS_INFO:
            break;
        case USB_CLASS_INTERFACE_INDEX_INFO:
            *object = 0xff;
            if (handle == (uint32_t)g_composite_device.hid_keyboard.app_handle) {
                *object = (uint32_t)HID_KEYBOARD_INTERFACE_INDEX;
                break;
            } else if (handle == (uint32_t)g_composite_device.hid_mouse.app_handle) {
                *object = (uint32_t)HID_MOUSE_INTERFACE_INDEX;
                break;
            }
            break;
         case USB_COMPOSITE_INFO:
            keyboard_interface[0].index = 0;
            keyboard_interface[0].endpoints = keyboard_endpoints;
            mouse_interface[0].index = 1;
            mouse_interface[0].endpoints = mouse_endpoints;
            *object = (unsigned long)&usb_composite_info;
            break;
        default:
            break;
    }
    return USB_OK;

}

/**************************************************************************//*!
 *
 * @name  USB_Desc_Set_Speed
 *
 * @brief The function is used to set device speed
 *
 * @param handle:         handle
 * @param speed:          speed
 *
 * @return USB_OK                              When Success
 *         USBERR_INVALID_REQ_TYPE             when Error
 *****************************************************************************/
uint8_t USB_Desc_Set_Speed(uint32_t handle, uint16_t speed) {
    descriptor_union_t ptr1, ptr2;

    ptr1.pntr = g_config_descriptor;
    ptr2.pntr = g_config_descriptor + sizeof(g_config_descriptor);

    if (handle == g_composite_device.hid_keyboard.app_handle) {
        while (ptr1.word < ptr2.word) {
            if (ptr1.common->bDescriptorType == USB_DESC_TYPE_EP) {
                if (USB_SPEED_HIGH == speed) {
                    ptr1.ndpt->iInterval = HS_INTERRUPT_OUT_ENDP_INTERVAL;
                    ptr1.ndpt->wMaxPacketSize[0] = USB_uint_16_low(HS_INTERRUPT_OUT_ENDP_PACKET_SIZE);
                    ptr1.ndpt->wMaxPacketSize[1] = USB_uint_16_high(HS_INTERRUPT_OUT_ENDP_PACKET_SIZE);
                } else {
                    ptr1.ndpt->iInterval = FS_INTERRUPT_OUT_ENDP_INTERVAL;
                    ptr1.ndpt->wMaxPacketSize[0] = USB_uint_16_low(FS_INTERRUPT_OUT_ENDP_PACKET_SIZE);
                    ptr1.ndpt->wMaxPacketSize[1] = USB_uint_16_high(FS_INTERRUPT_OUT_ENDP_PACKET_SIZE);
                }
            }
            ptr1.word += ptr1.common->bLength;
        }

        for (int i = 0; i < HID_DESC_ENDPOINT_COUNT; i++) {
            if (USB_SPEED_HIGH == speed) {
                keyboard_endpoint[i].size = HS_INTERRUPT_OUT_ENDP_PACKET_SIZE;
            } else {
                keyboard_endpoint[i].size = FS_INTERRUPT_OUT_ENDP_PACKET_SIZE;
            }
        }
    } else if (handle == g_composite_device.hid_mouse.app_handle) {
        while (ptr1.word < ptr2.word) {
            if (ptr1.common->bDescriptorType == USB_DESC_TYPE_EP) {
                if ((ptr1.ndpt->bmAttributes & 0x03) == USB_INTERRUPT_PIPE) {
                    if (USB_SPEED_HIGH == speed) {
                        ptr1.ndpt->iInterval = HS_INTERRUPT_OUT_ENDP_INTERVAL;
                        ptr1.ndpt->wMaxPacketSize[0] = USB_uint_16_low(HS_INTERRUPT_OUT_ENDP_PACKET_SIZE);
                        ptr1.ndpt->wMaxPacketSize[1] = USB_uint_16_high(HS_INTERRUPT_OUT_ENDP_PACKET_SIZE);
                    } else {
                        ptr1.ndpt->iInterval = FS_INTERRUPT_OUT_ENDP_INTERVAL;
                        ptr1.ndpt->wMaxPacketSize[0] = USB_uint_16_low(FS_INTERRUPT_OUT_ENDP_PACKET_SIZE);
                        ptr1.ndpt->wMaxPacketSize[1] = USB_uint_16_high(FS_INTERRUPT_OUT_ENDP_PACKET_SIZE);
                    }
                }
            }
            ptr1.word += ptr1.common->bLength;
        }

        for (int i = 0; i < HID_DESC_ENDPOINT_COUNT; i++) {
            if (USB_SPEED_HIGH == speed) {
                mouse_endpoint[i].size = HS_INTERRUPT_OUT_ENDP_PACKET_SIZE;
            } else {
                mouse_endpoint[i].size = FS_INTERRUPT_OUT_ENDP_PACKET_SIZE;
            }
        }
    }

    return USB_OK;
}

usb_desc_request_notify_struct_t desc_callback =
{
    USB_Desc_Get_Descriptor,
    USB_Desc_Get_Interface,
    USB_Desc_Set_Interface,
    USB_Set_Configuration,
    USB_Desc_Get_Entity
};
