#ifndef __USB_REPORT_SENDER_H__
#define __USB_REPORT_SENDER_H__

// Includes:

    #include <stdint.h>
    #include <stdbool.h>
    #include "hid/transport.h" // errno_t
    #include "usb_semaphore.h" // report_send_state_t

// Macros:

    // Backoff sizing. MAX_RETRIES is sized so that the cumulative wait from
    // UsbReportSender_ComputeResendDelay() is ~255 ms (1+2+4+8+16+32 + 6*32 = 255).
    #define THROTTLE_MAX_RETRIES 12

// Variables:

    // Test only: once, the test sink confirms its held report inside the next UsbReportSender_Fail
    // of a keyboard report.
    extern bool UsbReportSender_TestConfirmsInFail;

// Functions:

    void UsbReportSender_UpdateAndSendUsbReports(void);
    bool UsbReportSender_ShouldGiveUp(int err, uint8_t* counter);
    bool UsbReportSender_Fail(report_send_state_t* st, errno_t result, bool withDelay);
    uint16_t UsbReportSender_ComputeResendDelay(uint8_t counter);
    void UsbReportSender_TearDownStalledLink(report_send_state_t* st);
    void UsbReportSender_RequestStateResend(void);

#endif
