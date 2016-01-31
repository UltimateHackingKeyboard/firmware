#ifndef _MOUSE_H
#define _MOUSE_H

#include "usb_descriptor.h"

/* Macros: */
    #define  MOUSE_BUFF_SIZE   (4) /* report buffer size */
    #define  REQ_DATA_SIZE     (1)
    #define COMPLIANCE_TESTING (0) /* 1:TRUE, 0:FALSE */

/* Type defines: */
    typedef struct hid_mouse_struct {
        hid_handle_t app_handle;
        bool mouse_init; /* flag to check lower layer status*/
        uint8_t rpt_buf[MOUSE_BUFF_SIZE]; /* report/data buff for mouse application */
        uint8_t app_request_params[2]; /* for get/set idle and protocol requests */
    } hid_mouse_struct_t;

/* Function prototypes: */
    void hid_mouse_init(void* param);

#endif
