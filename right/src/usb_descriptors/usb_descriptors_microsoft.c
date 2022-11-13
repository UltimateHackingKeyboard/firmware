/**
  ******************************************************************************
  * @author  Benedek Kupper
  * @date    2018-01-31
  * @brief   Universal Serial Bus Device Driver
  *          USB descriptors provision
  *
  * Copyright (c) 2018 Benedek Kupper
  *
  * Licensed under the Apache License, Version 2.0 (the "License");
  * you may not use this file except in compliance with the License.
  * You may obtain a copy of the License at
  *
  *     http://www.apache.org/licenses/LICENSE-2.0
  *
  * Unless required by applicable law or agreed to in writing, software
  * distributed under the License is distributed on an "AS IS" BASIS,
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  */
#include "usb_api.h"
#include "usb_composite_device.h"
#include "usb_microsoft_os.h"


/** @brief USB device capability types */
typedef enum
{
    USB_DEVCAP_USB_2p0_EXT  = 0x02, /*!< USB 2.0 extension (for LPM) */
    USB_DEVCAP_PLATFORM     = 0x05, /*!< Microsoft OS 2.0 descriptors */
}USB_DeviceCapabilityType;

/** @brief Binary device Object Store descriptor (header only) */
typedef PACKED(struct)
{
    uint8_t  bLength;               /*!< Size of Descriptor in Bytes */
    uint8_t  bDescriptorType;       /*!< BOS Descriptor (0x0F) */
    uint16_t wTotalLength;          /*!< Total length in bytes of data returned */
    uint8_t  bNumDeviceCaps;        /*!< Number of device capabilities to follow */
}USB_BOSDescType;

/** @brief USB device capability descriptor structure */
typedef PACKED(struct)
{
    uint8_t  bLength;               /*!< Size of Descriptor in Bytes */
    uint8_t  bDescriptorType;       /*!< Device Capability Descriptor (0x10) */
    uint8_t  bDevCapabilityType;    /*!< Capability type: USB 2.0 EXTENSION (0x02) */
    uint32_t bmAttributes;          /*!< Bit 0 Reserved (set to 0)
                                         Bit 1 Link Power Management support
                                         Bit 2 BESL and alternate HIRD definitions support */
}USB_DevCapabilityDescType;

/** @brief USB Binary device Object Store (BOS) Descriptor structure */
typedef PACKED(struct) {
    USB_BOSDescType bos;                    /*!< BOS base */
    USB_DevCapabilityDescType devCap;       /*!< Device capabilities */
#if (USBD_MS_OS_DESC_VERSION == 2)
    USB_MsPlatformCapabilityDescType winPlatform;
#endif
}USBD_BOSType;

#if (USBD_MS_OS_DESC_VERSION == 2)
typedef PACKED(struct)
{
    PACKED(struct) {
        USB_MsDescSetHeaderType header;
        PACKED(struct) {
            USB_MsConfSubsetHeaderType header;
            PACKED(struct) {
                USB_MsFuncSubsetHeaderType header;
                USB_MsCompatIdDescType compatId;
            } functions[1];
        } config;
    } device;
} USB_MSOS_DescType;
#endif

/** @brief USB Binary device Object Store (BOS) */
static USB_DESC_STORAGE_TYPE(USBD_BOSType) usbd_bosDesc = {
    .bos = {
        .bLength            = sizeof(USB_BOSDescType),
        .bDescriptorType    = USB_DESCRIPTOR_TYPE_BINARY_OBJECT_STORE,
        .wTotalLength       = sizeof(usbd_bosDesc),
#if (USBD_MS_OS_DESC_VERSION == 2)
        .bNumDeviceCaps     = 2,
#else
        .bNumDeviceCaps     = 1,
#endif /* (USBD_MS_OS_DESC_VERSION == 2) */
    },
    .devCap = {
        .bLength            = sizeof(usbd_bosDesc.devCap),
        .bDescriptorType    = USB_DESCRIPTOR_TYPE_DEVICE_CAPABILITY,
        .bDevCapabilityType = USB_DEVCAP_USB_2p0_EXT,
        .bmAttributes       = 0,
    },
#if (USBD_MS_OS_DESC_VERSION == 2)
    .winPlatform = {
        .bLength            = sizeof(usbd_bosDesc.winPlatform),
        .bDescriptorType    = USB_DESCRIPTOR_TYPE_DEVICE_CAPABILITY,
        .bDevCapabilityType = USB_DEVCAP_PLATFORM,
        .PlatformCapabilityUUID = {
            0xDF, 0x60, 0xDD, 0xD8, 0x89, 0x45, 0xC7, 0x4C,
            0x9C, 0xD2, 0x65, 0x9D, 0x9E, 0x64, 0x8A, 0x9F
        },
        .CapabilityData.DescInfoSet = {
            .dwWindowsVersion = USB_MS_OS_2P0_MIN_WINDOWS_VERSION,
            .wMSOSDescriptorSetTotalLength = 0,
            .bMS_VendorCode = USB_REQ_MICROSOFT_OS,
            .bAltEnumCode = 1,
        },
    },
#endif /* (USBD_MS_OS_DESC_VERSION == 2) */
};

#if (USBD_MS_OS_DESC_VERSION == 2)
static USB_DESC_STORAGE_TYPE(USB_MSOS_DescType) usbd_msosDesc = {
    .device = {
        .header = {
            .wLength            = sizeof(USB_MsDescSetHeaderType),
            .wDescriptorType    = USB_MS_OS_2p0_SET_HEADER_DESCRIPTOR,
            .dwWindowsVersion   = USB_MS_OS_2P0_MIN_WINDOWS_VERSION,
            .wTotalLength       = sizeof(usbd_msosDesc)
        },
        .config = {
            .header = {
                .wLength                = sizeof(USB_MsConfSubsetHeaderType),
                .wDescriptorType        = USB_MS_OS_2p0_SUBSET_HEADER_CONFIGURATION,
                .bConfigurationValue    = 0,
                .bReserved              = 0,
                .wTotalLength           = sizeof(usbd_msosDesc.device.config)
            },
            .functions = {
                {
                    .header = {
                        .wLength                = sizeof(USB_MsFuncSubsetHeaderType),
                        .wDescriptorType        = USB_MS_OS_2p0_SUBSET_HEADER_FUNCTION,
                        .bFirstInterface        = USB_DEVICE_CONFIG_HID - 1, /* last HID interface reused as XInput */
                        .bReserved              = 0,
                        .wSubsetLength          = sizeof(usbd_msosDesc.device.config.functions[0])
                    },
                    .compatId = {
                        .wLength                = sizeof(USB_MsCompatIdDescType),
                        .wDescriptorType        = USB_MS_OS_2p0_FEATURE_COMPATIBLE_ID,
                        .CompatibleID           = "XUSB20",
                    },
                },
            }
        }
    }
};
#endif /* (USBD_MS_OS_DESC_VERSION == 2) */

usb_status_t USB_DeviceGetBosDescriptor(
    usb_device_handle handle, usb_device_get_descriptor_common_struct_t *descriptor)
{
    descriptor->buffer = (uint8_t*)&usbd_bosDesc;
    descriptor->length = sizeof(usbd_bosDesc);
    return kStatus_USB_Success;
}

usb_status_t USB_DeviceGetMsOsDescriptor(
    usb_device_handle handle, usb_device_control_request_struct_t *controlRequest)
{
#if (USBD_MS_OS_DESC_VERSION == 2)
    controlRequest->buffer = (uint8_t*)NULL;
    controlRequest->length = 0;
    return kStatus_USB_Success;
#else
    return kStatus_USB_InvalidRequest;
#endif /* (USBD_MS_OS_DESC_VERSION == 2) */
}
