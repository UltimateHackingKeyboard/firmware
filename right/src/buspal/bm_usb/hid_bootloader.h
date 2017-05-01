#ifndef __USB_HID_BOOTLOADER_H__
#define __USB_HID_BOOTLOADER_H__

#include "fsl_rtos_abstraction.h"
#include "bootloader_hid_report_ids.h"
#include "bm_usb/usb_descriptor.h"

enum { // Request parameters
    kAppRequestParam_IdleRate = 0,
    kAppRequestParam_Protocol,
    kAppRequestParamCount
};

typedef struct _usb_hid_packetizer_info {
    bool didReceiveFirstReport;        // Whether the first report has been received.
    bool didReceiveDataPhaseAbort;     // Whether we received a data phase abort request.
    bool isReceiveDataRequestRequired; // Whether an interrupt out pipe receive data request is required.
    uint8_t appRequestParams[kAppRequestParamCount]; // Storage for request parameter values.
    sync_object_t receiveSync;                       // Sync object used for reading packets.
    sync_object_t sendSync;                          // Sync object used for sending packets.
    uint32_t reportSize; // The size in bytes of a received report. May be greater than the packet contained
                         // within the report plus the header, as the host can send up to the max report size bytes.
    bl_hid_report_t report; //!< Buffer used to hold HID reports for sending and receiving.
} usb_hid_packetizer_info_t;

typedef struct _usb_hid_generic_struct {
    usb_device_handle device_handle;
    class_handle_t hid_handle;
    usb_hid_packetizer_info_t hid_packet;
    uint8_t idle_rate;
    uint8_t speed;
    uint8_t attach;
    uint8_t current_configuration;
    uint8_t current_interface_alternate_setting[USB_HID_GENERIC_INTERFACE_COUNT];
} usb_hid_generic_struct_t;

#endif
