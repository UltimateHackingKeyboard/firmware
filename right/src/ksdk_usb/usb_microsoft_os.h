/**
  ******************************************************************************
  * @file    usb_microsoft_os.h
  * @author  Benedek Kupper
  * @version 0.1
  * @date    2020-08-24
  * @brief   Universal Serial Bus Device Driver
  *          Microsoft OS descriptor definitions
  *
  * Copyright (c) 2020 Benedek Kupper
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
#ifndef __USB_MICROSOFT_OS_H_
#define __USB_MICROSOFT_OS_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "usb_api.h"

#ifndef USBD_MS_OS_DESC_VERSION
#define USBD_MS_OS_DESC_VERSION     0
#elif (USBD_MS_OS_DESC_VERSION != 0) && (USBD_MS_OS_DESC_VERSION != 1) && (USBD_MS_OS_DESC_VERSION != 2)
#error "Invalid USBD_MS_OS_DESC_VERSION value!"
#endif

/** @ingroup USB
 * @defgroup USB_MS USB Microsoft OS descriptors
 * @{ */

/** @defgroup USB_MS_Exported_Types USB Microsoft OS Exported Types
 * @{ */

/** @brief USB vendor-specific request types */
typedef enum
{
    USB_REQ_MICROSOFT_OS = 0x01, /*!< Microsoft OS requests */
}USB_MsVendorRequestType;

#if (USBD_MS_OS_DESC_VERSION == 1)

/** @brief USB vendor-specific request index types */
typedef enum
{
    USB_MS_OS_1p0_GENRE_INDEX               = 0x01, /*!< wIndex value for Genre */
    USB_MS_OS_1p0_EXTENDED_COMPAT_ID_INDEX  = 0x04, /*!< wIndex value for Extended compat ID */
    USB_MS_OS_1p0_EXTENDED_PROPERTIES_INDEX = 0x05, /*!< wIndex value for Extended properties */
}USB_MsVendorRequestIndexType;

#elif (USBD_MS_OS_DESC_VERSION == 2)

/** @brief USB vendor-specific request index types */
typedef enum
{
    USB_MS_OS_2p0_GET_DESCRIPTOR_INDEX      = 0x07, /*!< wIndex value for GetDescriptor */
    USB_MS_OS_2p0_SET_ALT_ENUMERATION_INDEX = 0x08, /*!< wIndex value for SetAltEnumeration */
}USB_MsVendorRequestIndexType;

#endif /* USBD_MS_OS_DESC_VERSION */

/** @brief USB vendor-specific descriptor types */
typedef enum
{
    USB_MS_OS_2p0_SET_HEADER_DESCRIPTOR         = 0x00, /*!<  */
    USB_MS_OS_2p0_SUBSET_HEADER_CONFIGURATION   = 0x01, /*!<  */
    USB_MS_OS_2p0_SUBSET_HEADER_FUNCTION        = 0x02, /*!<  */
    USB_MS_OS_2p0_FEATURE_COMPATIBLE_ID         = 0x03, /*!<  */
    USB_MS_OS_2p0_FEATURE_REG_PROPERTY          = 0x04, /*!<  */
    USB_MS_OS_2p0_FEATURE_MIN_RESUME_TIME       = 0x05, /*!<  */
    USB_MS_OS_2p0_FEATURE_MODEL_ID              = 0x06, /*!<  */
    USB_MS_OS_2p0_FEATURE_CCGP_DEVICE           = 0x07, /*!<  */
    USB_MS_OS_2p0_FEATURE_VENDOR_REVISION       = 0x08, /*!<  */
}USB_MsVendorDescriptorType;

/** @brief Windows OS versions */
typedef enum
{
    USB_MS_OS_WINDOWS_VER_8p1           = 0x06030000,
    USB_MS_OS_2P0_MIN_WINDOWS_VERSION   = USB_MS_OS_WINDOWS_VER_8p1,
}USB_MsWindowsOsVersionType;

/** @brief Descriptor Information Set for Windows 8.1 or later */
typedef PACKED(struct)
{
    uint32_t dwWindowsVersion;              /*!< Minimum Windows version (at least 0x06030000) */
    uint16_t wMSOSDescriptorSetTotalLength; /*!< The length, in bytes of the MS OS 2.0 descriptor set */
    uint8_t  bMS_VendorCode;                /*!< Vendor defined code to use to retrieve this version of the MS OS 2.0 descriptor
                                                 and also to set alternate enumeration behavior on the device */
    uint8_t  bAltEnumCode;                  /*!< A non-zero value to send to the device to indicate
                                                 that the device may return non-default USB descriptors for enumeration.
                                                 If the device does not support alternate enumeration, this value shall be 0. */
}USB_MsDescInfoSetDescType;

/** @brief Microsoft OS 2.0 Platform Capability Descriptor Header */
typedef PACKED(struct)
{
    uint8_t  bLength;                       /*!< Size of Descriptor in Bytes */
    uint8_t  bDescriptorType;               /*!< Device Capability Descriptor (0x10) */
    uint8_t  bDevCapabilityType;            /*!< Capability type: PLATFORM (0x05) */
    uint8_t  bReserved;                     /*!< Keep 0 */
    uint8_t  PlatformCapabilityUUID[16];    /*!< A 128-bit number that uniquely identifies
                                                 a platform-specific capability of the device.
                                                 Refer to IETF RFC 4122 for details on generation of a UUID. */
    union
    {
        USB_MsDescInfoSetDescType DescInfoSet;
        uint8_t Raw[1];
    }CapabilityData;                        /*!< A variable-length field containing data associated with
                                                 the platform specific capability. */
}USB_MsPlatformCapabilityDescType;

/** @brief Microsoft OS 2.0 descriptor set header structure */
typedef PACKED(struct)
{
    uint16_t wLength;               /*!< Size of Header in Bytes */
    uint16_t wDescriptorType;       /*!< Descriptor Set Header (0x00) */
    uint32_t dwWindowsVersion;      /*!< Minimum Windows version (at least 0x06030000) */
    uint16_t wTotalLength;          /*!< The size of entire MS OS 2.0 descriptor set.
                                         The value shall match the value in the descriptor set information structure */
}USB_MsDescSetHeaderType;

/** @brief Microsoft OS 2.0 configuration subset header structure */
typedef PACKED(struct)
{
    uint16_t wLength;               /*!< Size of Header in Bytes */
    uint16_t wDescriptorType;       /*!< Subset Header Configuration (0x01) */
    uint8_t  bConfigurationValue;   /*!< The configuration value for the USB configuration to which this subset applies
                                         @note: In its current status, this field is actually an index of valid configuration,
                                         so it must be set to @ref USB_ConfigDescType::bConfigurationValue - 1
                                         https://social.msdn.microsoft.com/Forums/ie/en-US/ae64282c-3bc3-49af-8391-4d174479d9e7/microsoft-os-20-descriptors-not-working-on-an-interface-of-a-composite-usb-device?forum=wdk*/
    uint8_t  bReserved;             /*!< Keep 0 */
    uint16_t wTotalLength;          /*!< The size of entire configuration subset including this header */
}USB_MsConfSubsetHeaderType;

/** @brief Microsoft OS 2.0 function subset header structure */
typedef PACKED(struct)
{
    uint16_t wLength;               /*!< Size of Header in Bytes */
    uint16_t wDescriptorType;       /*!< Subset Header Function (0x02) */
    uint8_t  bFirstInterface;       /*!< The interface number for the first interface of the function to which this subset applies */
    uint8_t  bReserved;             /*!< Keep 0 */
    uint16_t wSubsetLength;         /*!< The size of entire function subset including this header */
}USB_MsFuncSubsetHeaderType;

#if (USBD_MS_OS_DESC_VERSION == 1)

/** @brief Microsoft OS 1.0 compatible ID descriptor structure */
typedef PACKED(struct)
{
    uint32_t dwLength;              /*!< Size of descriptor in Bytes */
    uint16_t bcdVersion;            /*!< Specification version (0x0100) */
    uint16_t wIndex;                /*!< Extended compat ID descriptor index (0x04) */
    uint8_t  bCount;                /*!< The number of custom property sections */
    uint8_t  _Reserved0[7];         /*!< Keep 0 */
    struct
    {
        uint8_t  bFirstInterfaceNumber; /*!< The interface or function number */
        uint8_t  _Reserved1[1];         /*!< Keep 1 */
        char     CompatibleID[8];       /*!< Compatible ID String */
        char     SubCompatibleID[8];    /*!< Sub-compatible ID String */
        uint8_t  _Reserved2[6];         /*!< Keep 0 */
    }Function[0];
}USB_MsCompatIdDescType;

#elif (USBD_MS_OS_DESC_VERSION == 2)

/** @brief Microsoft OS 2.0 compatible ID descriptor structure */
typedef PACKED(struct)
{
    uint16_t wLength;               /*!< Size of Header in Bytes */
    uint16_t wDescriptorType;       /*!< Compatible ID (0x03) */
    char     CompatibleID[8];       /*!< Compatible ID String */
    char     SubCompatibleID[8];    /*!< Sub-compatible ID String */
}USB_MsCompatIdDescType;

#endif /* USBD_MS_OS_DESC_VERSION */

/** @brief Registry property types for the Microsoft OS registry property descriptor */
typedef enum
{
    USB_MS_OS_REG_SZ                    = 0x01, /*!< A NULL-terminated Unicode String */
    USB_MS_OS_REG_EXPAND_SZ             = 0x02, /*!< A NULL-terminated Unicode String that includes environment variables */
    USB_MS_OS_REG_BINARY                = 0x03, /*!< Free-form binary */
    USB_MS_OS_REG_DWORD_LITTLE_ENDIAN   = 0x04, /*!< A little-endian 32-bit integer */
    USB_MS_OS_REG_DWORD_BIG_ENDIAN      = 0x05, /*!< A big-endian 32-bit integer */
    USB_MS_OS_REG_LINK                  = 0x06, /*!< A NULL-terminated Unicode string that contains a symbolic link */
    USB_MS_OS_REG_MULTI_SZ              = 0x07, /*!< Multiple NULL-terminated Unicode strings */
}USB_MsRegPropertyDataType;

#if (USBD_MS_OS_DESC_VERSION == 1)

/** @brief Microsoft OS 1.0 extended properties descriptor structure */
typedef PACKED(struct)
{
    uint32_t dwLength;              /*!< Size of descriptor in Bytes */
    uint16_t bcdVersion;            /*!< Specification version (0x0100) */
    uint16_t wIndex;                /*!< Extended properties descriptor index (0x05) */
    uint16_t wCount;                /*!< The number of custom property sections */
    struct
    {
        uint32_t dwDataType;    /*!< The type of registry property (@ref USB_MsRegPropertyDataType) */
        uint16_t wNameLength;   /*!< The length of the property name */
        short    Name[0];       /*!< The null-terminated Unicode name of registry property (variable size) */
        uint32_t dwDataLength;  /*!< The length of the property data */
        uint8_t  Data[0];       /*!< The data of registry property (variable size) */
    }Property[0];
}USB_MsExtendedPropDescType;

#elif (USBD_MS_OS_DESC_VERSION == 2)

/** @brief Microsoft OS 2.0 registry property descriptor structure */
typedef PACKED(struct)
{
    uint16_t wLength;               /*!< Size of Header in Bytes */
    uint16_t wDescriptorType;       /*!< Registry Property (0x04) */
    uint16_t wPropertyDataType;     /*!< The type of registry property (@ref USB_MsRegPropertyDataType) */
    uint16_t wPropertyNameLength;   /*!< The length of the property name */
    short    PropertyName[0];       /*!< The null-terminated? Unicode? name of registry property (variable size) */
    uint16_t wPropertyDataLength;   /*!< The length of the property data */
    uint8_t  PropertyData[0];       /*!< The data of registry property (variable size) */
}USB_MsRegistryPropDescType;

#endif /* USBD_MS_OS_DESC_VERSION */

/** @brief Microsoft OS 2.0 minimum USB recovery time descriptor structure
 * @details The Microsoft OS 2.0 minimum USB resume time descriptor is used to indicate to the Windows USB driver stack
 * the minimum time needed to recover after resuming from suspend, and how long the device requires resume signaling to be asserted.
 * This descriptor is used for a device operating at high, full, or low-speed.
 * It is not used for a device operating at SuperSpeed or higher.
 *
 * This descriptor allows devices to recover faster than the default 10 millisecond specified in the USB 2.0 specification.
 * It can also allow the host to assert resume signaling for less than the 20 milliseconds required in the USB 2.0 specification,
 * in cases where the timing of resume signaling is controlled by software.
 * @note The USB resume time descriptor is applied to the entire device. */
typedef PACKED(struct)
{
    uint16_t wLength;               /*!< Size of Header in Bytes */
    uint16_t wDescriptorType;       /*!< Minimum Resume Time (0x05) */
    uint8_t  bResumeRecoveryTime;   /*!< The number of milliseconds the device requires to recover from port resume [0..10] */
    uint8_t  bResumeSignalingTime;  /*!< The number of milliseconds the device requires resume signaling to be asserted [1..20] */
}USB_MsMinResumeTimeDescType;

/** @brief Microsoft OS 2.0 model ID descriptor structure
 * @details The Microsoft OS 2.0 model ID descriptor is used to uniquely identify the physical device.
 * @note The model ID descriptor is applied to the entire device. */
typedef PACKED(struct)
{
    uint16_t wLength;               /*!< Size of Header in Bytes */
    uint16_t wDescriptorType;       /*!< Model ID (0x06) */
    uint8_t  ModelID[16];           /*!< This is a 128-bit number that uniquely identifies a physical device.
                                         Refer to IETF RFC 4122 for details on generation of a UUID. */
}USB_MsModelIdDescType;

/** @brief Microsoft OS 2.0 CCGP device descriptor structure
 * @details The Microsoft OS 2.0 CCGP device descriptor is used to indicate
 * that the device should be treated as a composite device by Windows
 * regardless of the number of interfaces, configuration, or class, subclass, and protocol codes, the device reports.
 * @note The CCGP device descriptor must be applied to the entire device. */
typedef PACKED(struct)
{
    uint16_t wLength;               /*!< Size of Header in Bytes */
    uint16_t wDescriptorType;       /*!< CCGP device (0x07) */
}USB_MsCcgpDescType;

/** @brief Microsoft OS 2.0 vendor revision descriptor structure
 * @details The Microsoft OS 2.0 vendor revision descriptor is used to indicate
 * the revision of registry property and other MSOS descriptors.
 * If this value changes between enumerations the registry property descriptors will be updated in registry during that enumeration.
 * You must always change this value if you are adding/modifying any registry property or other MSOS descriptors.
 * @note The vendor revision descriptor must be applied at the device scope for a non-composite device
 * or for MSOS descriptors that apply to the device scope of a composite device.
 * Additionally, for a composite device, the vendor revision descriptor must be provided in every function subset
 * and may be updated independently per-function. */
typedef PACKED(struct)
{
    uint16_t wLength;               /*!< Size of Header in Bytes */
    uint16_t wDescriptorType;       /*!< Vendor Revision (0x08) */
    uint16_t wVendorRevision;       /*!< Revision number associated with the descriptor set.
                                         Modify it every time you add/modify a registry property
                                         or other MSOS descriptor. Set to greater than or equal to 1. */
}USB_MsVendorRevDescType;

/** @} */

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* __USB_MICROSOFT_OS_H_ */
