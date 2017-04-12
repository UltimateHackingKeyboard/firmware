#ifndef __BOOTLOADER_HID_REPORT_IDS_H__
#define __BOOTLOADER_HID_REPORT_IDS_H__

#include "packet/command_packet.h"

enum _hid_report_ids {
    kBootloaderReportID_CommandOut = 1,
    kBootloaderReportID_DataOut = 2,
    kBootloaderReportID_CommandIn = 3,
    kBootloaderReportID_DataIn = 4
};

typedef struct _bl_hid_header {
    uint8_t reportID;        // The report ID.
    uint8_t _padding;        // Pad byte necessary so that the length is 2-byte aligned and the data is 4-byte aligned. Set to zero.
    uint8_t packetLengthLsb; // Low byte of the packet length in bytes.
    uint8_t packetLengthMsb; // High byte of the packet length in bytes.
} bl_hid_header_t;

typedef struct _bl_hid_report {
    bl_hid_header_t header;               // Header of the report.
    uint8_t packet[kMinPacketBufferSize]; // The packet data that is transferred in the report.
} bl_hid_report_t;

#endif
