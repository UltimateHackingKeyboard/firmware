#include "usb_device_config.h"
#include "usb.h"
#include "usb_device_stack_interface.h"
#include "usb_class_hid.h"
#include "keyboard.h"
#include "usb_descriptor.h"
#include "fsl_device_registers.h"
#include "fsl_clock_manager.h"
#include "board.h"
#include "fsl_debug_console.h"
#include "fsl_port_hal.h"
#include <stdio.h>
#include <stdlib.h>

extern void Main_Task(uint32_t param);
#define MAIN_TASK       10

keyboard_global_variable_struct_t* g_keyboard;
uint32_t g_process_times = 1;
uint8_t g_key_index = 2;
uint8_t g_new_key_pressed = 0;

void USB_Keyboard_App_Device_Callback(uint8_t event_type, void* val, void* arg);
uint8_t USB_Keyboard_App_Class_Callback(uint8_t request, uint16_t value, uint8_t ** data,
    uint32_t* size, void* arg);

/*****************************************************************************
 * Local Variables
 *****************************************************************************/

/*****************************************************************************
 * Local Functions
 *****************************************************************************/
/******************************************************************************
 * @name:         KeyBoard_Events_Process
 *
 * @brief:        This function gets the input from keyboard, the keyboard
 *                does not include the code to filter the glitch on keys since
 *                it is just for demo
 *
 * @param:        None
 *
 * @return:       None
 *
 *****************************************************************************
 * This function sends the keyboard data depending on which key is pressed on
 * the board
 *****************************************************************************/
void KeyBoard_Events_Process(void)
{
    static int32_t x = 0;
    static enum { UP, DOWN} dir = UP;

    switch(dir)
    {
    case UP:
        g_keyboard->rpt_buf[g_key_index] = KEY_PAGEUP;
        x++;
        if (x > 100)
        {
            dir = DOWN;
        }
        break;
    case DOWN:
        g_keyboard->rpt_buf[g_key_index] = KEY_PAGEDOWN;
        x--;
        if (x < 0)
        {
            dir = UP;
        }
        break;
    }
    (void) USB_Class_HID_Send_Data(g_keyboard->app_handle, HID_ENDPOINT, g_keyboard->rpt_buf, KEYBOARD_BUFF_SIZE);

    return;
}

/******************************************************************************
 *
 *    @name        USB_App_Device_Callback
 *
 *    @brief       This function handles the callback
 *
 *    @param       handle : handle to Identify the controller
 *    @param       event_type       : value of the event
 *    @param       val              : gives the configuration value
 *
 *    @return      None
 *
 *****************************************************************************/
void USB_Keyboard_App_Device_Callback(uint8_t event_type, void* val, void* arg)
{
    UNUSED_ARGUMENT (arg)
    UNUSED_ARGUMENT (val)

    switch(event_type)
    {
    case USB_DEV_EVENT_BUS_RESET:
        g_keyboard->keyboard_init = FALSE;
        if (USB_OK == USB_Class_HID_Get_Speed(g_keyboard->app_handle, &g_keyboard->app_speed)) {
            USB_Desc_Set_Speed(g_keyboard->app_handle, g_keyboard->app_speed);
        }
        break;
    case USB_DEV_EVENT_ENUM_COMPLETE:
        g_keyboard->keyboard_init = TRUE;
        g_process_times = 1;
        KeyBoard_Events_Process();/* run the cursor movement code */
        break;
    case USB_DEV_EVENT_ERROR:
        // user may add code here for error handling
        // NOTE : val has the value of error from h/w
        break;
    default:
        break;
    }
    return;
}

/******************************************************************************
 *
 *    @name        USB_App_Class_Callback
 *
 *    @brief       This function handles USB-HID Class callback
 *
 *    @param       request  :  request type
 *    @param       value    :  give report type and id
 *    @param       data     :  pointer to the data
 *    @param       size     :  size of the transfer
 *
 *    @return      status
 *                  USB_OK  :  if successful
 *                  else return error
 *
 *****************************************************************************
 * This function is called whenever a HID class request is received. This
 * function handles these class requests.
 *****************************************************************************/
uint8_t USB_Keyboard_App_Class_Callback
(
    uint8_t request,
    uint16_t value,
    uint8_t ** data,
    uint32_t* size,
    void* arg
    )
{
    uint8_t error = USB_OK;

    uint8_t index = (uint8_t)((request - 2) & USB_HID_REQUEST_TYPE_MASK);
    if ((request == USB_DEV_EVENT_SEND_COMPLETE) && (value == USB_REQ_VAL_INVALID) && (*size != 0xFFFFFFFF)) {
        if ((g_keyboard->keyboard_init) && (arg != NULL)) {
#if COMPLIANCE_TESTING
            uint32_t g_compliance_delay = 0x009FFFFF;
            while(g_compliance_delay--);
#endif

            KeyBoard_Events_Process();/* run the cursor movement code */
        }
        return error;
    }

    /* index == 0 for get/set idle, index == 1 for get/set protocol */
    /* handle the class request */
    switch(request)
    {
    case USB_HID_GET_REPORT_REQUEST:
        *data = &g_keyboard->rpt_buf[0]; /* point to the report to send */
        *size = KEYBOARD_BUFF_SIZE; /* report size */
        break;

    case USB_HID_SET_REPORT_REQUEST:
        for (index = 0; index < (*size); index++)
        { /* copy the report sent by the host */
            //g_keyboard->rpt_buf[index] = *(*data + index);
        }
        *size = 0;
        break;

    case USB_HID_GET_IDLE_REQUEST:
        /* point to the current idle rate */
        *data = &g_keyboard->app_request_params[index];
        *size = REQ_DATA_SIZE;
        break;

    case USB_HID_SET_IDLE_REQUEST:
        /* set the idle rate sent by the host */
        g_keyboard->app_request_params[index] = (uint8_t)((value & MSB_MASK) >>
        HIGH_BYTE_SHIFT);
        *size = 0;
        break;

    case USB_HID_GET_PROTOCOL_REQUEST:
        /* point to the current protocol code
         0 = Boot Protocol
         1 = Report Protocol*/
        *data = &g_keyboard->app_request_params[index];
        *size = REQ_DATA_SIZE;
        break;

    case USB_HID_SET_PROTOCOL_REQUEST:
        /* set the protocol sent by the host
         0 = Boot Protocol
         1 = Report Protocol*/
        g_keyboard->app_request_params[index] = (uint8_t)(value);
        *size = 0;
        break;
    }
    return error;
}

/******************************************************************************
 *
 *   @name        APP_init
 *
 *   @brief       This function is the entry for Keyboard Application
 *
 *   @param       None
 *
 *   @return      None
 *
 *****************************************************************************
 * This function starts the keyboard application
 *****************************************************************************/

void hid_keyboard_init(void* param) {
    g_keyboard = (keyboard_global_variable_struct_t*)param;
}
