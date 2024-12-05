#ifndef __LEGACY_PORTS_H__
#define __LEGACY_PORTS_H__

// Typedefs:

#ifdef __ZEPHYR__
    typedef enum _usb_status
    {
        kStatus_USB_Success = 0x00U, /*!< Success */
        kStatus_USB_Error,           /*!< Failed */

        kStatus_USB_Busy,                       /*!< Busy */
        kStatus_USB_InvalidHandle,              /*!< Invalid handle */
        kStatus_USB_InvalidParameter,           /*!< Invalid parameter */
        kStatus_USB_InvalidRequest,             /*!< Invalid request */
        kStatus_USB_ControllerNotFound,         /*!< Controller cannot be found */
        kStatus_USB_InvalidControllerInterface, /*!< Invalid controller interface */

        kStatus_USB_NotSupported,   /*!< Configuration is not supported */
        kStatus_USB_Retry,          /*!< Enumeration get configuration retry */
        kStatus_USB_TransferStall,  /*!< Transfer stalled */
        kStatus_USB_TransferFailed, /*!< Transfer failed */
        kStatus_USB_AllocFail,      /*!< Allocation failed */
        kStatus_USB_LackSwapBuffer, /*!< Insufficient swap buffer for KHCI */
        kStatus_USB_TransferCancel, /*!< The transfer cancelled */
        kStatus_USB_BandwidthFail,  /*!< Allocate bandwidth failed */
        kStatus_USB_MSDStatusFail,  /*!< For MSD, the CSW status means fail */
    } usb_status_t;

    typedef uint8_t usb_hid_protocol_t;
#endif

#endif
