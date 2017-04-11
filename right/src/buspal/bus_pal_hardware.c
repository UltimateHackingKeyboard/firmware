#include "bus_pal_hardware.h"
#include "fsl_device_registers.h"
#include "fsl_i2c.h"
#include "fsl_uart.h"
#include "microseconds/microseconds.h"
#include "fsl_rtos_abstraction.h"

#include "usb_descriptor.h"

#include "usb_device_config.h"
#include "usb.h"
#include "usb_device.h"

#include "usb_device_class.h"
#include "usb_device_hid.h"
#include "usb_device_ch9.h"
#include "usb_descriptor.h"

#include "composite.h"

////////////////////////////////////////////////////////////////////////////////
// Definitions
////////////////////////////////////////////////////////////////////////////////

#define REQ_DATA_SIZE (1)

#define USB_HID_INDEX (0)
#define USB_MSC_INDEX (1)

////////////////////////////////////////////////////////////////////////////////
// Prototypes
////////////////////////////////////////////////////////////////////////////////

// bool usb_hid_poll_for_activity(const peripheral_descriptor_t *self);
static status_t usb_device_full_init(const peripheral_descriptor_t *self, serial_byte_receive_func_t function);
static void usb_device_full_shutdown(const peripheral_descriptor_t *self);
static void usb_msc_pump(const peripheral_descriptor_t *self);

// status_t usb_hid_packet_init(const peripheral_descriptor_t *self);

static void usb_hid_packet_abort_data_phase(const peripheral_descriptor_t *self);
static status_t usb_hid_packet_finalize(const peripheral_descriptor_t *self);
static uint32_t usb_hid_packet_get_max_packet_size(const peripheral_descriptor_t *self);

// static bool s_dHidMscActivity[USB_COMPOSITE_INTERFACE_COUNT] = {false};
static bool s_dHidMscActivity[2] = { false };

/*!
 * @brief i2c initialization.
 */
static void init_i2c(uint32_t instance);

/*!
 * @brief get PORT base address function.
 */
static PORT_Type *getPortBaseAddrFromAscii(uint8_t port);

/*!
 * @brief get GPIO base address function.
 */
static GPIO_Type *getGpioBaseAddrFromAscii(uint8_t port);

////////////////////////////////////////////////////////////////////////////////
// Variables
////////////////////////////////////////////////////////////////////////////////

const peripheral_control_interface_t g_usbHidControlInterface = {.pollForActivity = usb_hid_poll_for_activity,
                                                                 .init = usb_device_full_init,
                                                                 .shutdown = usb_device_full_shutdown,
                                                                 .pump = usb_msc_pump };

const peripheral_packet_interface_t g_usbHidPacketInterface = {.init = usb_hid_packet_init,
                                                               .readPacket = usb_hid_packet_read,
                                                               .writePacket = usb_hid_packet_write,
                                                               .abortDataPhase = usb_hid_packet_abort_data_phase,
                                                               .finalize = usb_hid_packet_finalize,
                                                               .getMaxPacketSize = usb_hid_packet_get_max_packet_size,
                                                               .byteReceivedCallback = 0 };

const peripheral_descriptor_t g_peripherals[] = {
    // USB HID - Full speed
    {.typeMask = kPeripheralType_USB_HID,
     .instance = 0,
     .pinmuxConfig = NULL,
     .controlInterface = &g_usbHidControlInterface,
     .byteInterface = NULL,
     .packetInterface = &g_usbHidPacketInterface },
    { 0 } // Terminator
};

static usb_device_composite_struct_t g_device_composite;
usb_status_t usb_device_callback(usb_device_handle handle, uint32_t event, void *param);

extern usb_status_t usb_device_msc_callback(class_handle_t handle, uint32_t event, void *param);

//! @brief Variable for i2c host configuration information
static i2c_user_config_t s_i2cUserConfig = {.slaveAddress = 0x10, //!< The slave's 7-bit address
                                            .baudRate_kbps = 100 };

static i2c_master_handle_t s_i2cHandle;

//! @brief Variable for host data receiving
static uint8_t *s_rxData;
static uint32_t s_bytesRx;

const static uint32_t g_i2cBaseAddr[] = I2C_BASE_ADDRS;
const static uint32_t g_uartBaseAddr[] = UART_BASE_ADDRS;

uint32_t g_calculatedBaudRate;

////////////////////////////////////////////////////////////////////////////////
// Code
////////////////////////////////////////////////////////////////////////////////

/*FUNCTION**********************************************************************
 *
 * Function Name : usb_clock_init
 * Description   : usb clock init
 *
 *END**************************************************************************/
bool usb_clock_init(void)
{
    SIM->CLKDIV2 = (uint32_t)0x0UL; /* Update USB clock prescalers */
                                    // Select IRC48M clock
    SIM->SOPT2 |= (SIM_SOPT2_USBSRC_MASK | SIM_SOPT2_PLLFLLSEL_MASK);

    // Enable USB-OTG IP clocking
    SIM->SCGC4 |= (SIM_SCGC4_USBOTG_MASK);

    // Configure enable USB regulator for device
    SIM->SOPT1 |= SIM_SOPT1_USBREGEN_MASK;
    /* SIM_SOPT1: OSC32KSEL=0 */
    SIM->SOPT1 &=
        (uint32_t)~SIM_SOPT1_OSC32KSEL_MASK; /* System oscillator drives 32 kHz clock for various peripherals */

    USB0->CLK_RECOVER_IRC_EN = 0x03;
    USB0->CLK_RECOVER_CTRL |= USB_CLK_RECOVER_CTRL_CLOCK_RECOVER_EN_MASK;

    USB0->CLK_RECOVER_CTRL |= 0x20;
    return true;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : usb_device_callback
 * Description   : usb hid poll for activity
 *
 *END**************************************************************************/
bool usb_hid_poll_for_activity(const peripheral_descriptor_t *self)
{
    //    uint32_t hidInfoIndex = self->instance / 2;
    bool hid_active = false;
    bool msc_active = false;
    hid_active = g_device_composite.hid_generic.hid_packet.didReceiveFirstReport;

    s_dHidMscActivity[0] = hid_active;
    s_dHidMscActivity[1] = msc_active;
    return (g_device_composite.attach && (hid_active || msc_active));
}

/*FUNCTION**********************************************************************
 *
 * Function Name : usb_device_callback
 * Description   : device callback processing
 *
 *END**************************************************************************/
usb_status_t usb_device_callback(usb_device_handle handle, uint32_t event, void *param)
{
    usb_status_t error = kStatus_USB_Success;
    uint16_t *temp16 = (uint16_t *)param;
    uint8_t *temp8 = (uint8_t *)param;

    switch (event)
    {
        case kUSB_DeviceEventBusReset:
        {
            g_device_composite.attach = 0;
#if defined(USB_DEVICE_CONFIG_EHCI) && (USB_DEVICE_CONFIG_EHCI > 0)
            if (kStatus_USB_Success == USB_DeviceClassGetSpeed(CONTROLLER_ID, &g_device_composite.speed))
            {
                usb_device_set_speed(handle, g_device_composite.speed);
            }
#endif
        }
        break;
        case kUSB_DeviceEventSetConfiguration:
            if (param)
            {
                g_device_composite.attach = 1;
                g_device_composite.current_configuration = *temp8;
                error = usb_device_hid_generic_set_configure(g_device_composite.hid_generic.hid_handle, *temp8);
                error = kStatus_USB_Success;
            }
            break;
        case kUSB_DeviceEventSetInterface:
            if (g_device_composite.attach)
            {
                uint8_t interface = (uint8_t)((*temp16 & 0xFF00U) >> 0x08U);
                uint8_t alternate_setting = (uint8_t)(*temp16 & 0x00FFU);
                if (interface < USB_COMPOSITE_INTERFACE_COUNT)
                {
                    g_device_composite.current_interface_alternate_setting[interface] = alternate_setting;
                    usb_device_hid_generic_set_interface(g_device_composite.hid_generic.hid_handle, interface,
                                                         alternate_setting);
                    error = kStatus_USB_Success;
                }
            }
            break;
        case kUSB_DeviceEventGetConfiguration:
            if (param)
            {
                *temp8 = g_device_composite.current_configuration;
                error = kStatus_USB_Success;
            }
            break;
        case kUSB_DeviceEventGetInterface:
            if (param)
            {
                uint8_t interface = (uint8_t)((*temp16 & 0xFF00) >> 0x08);
                if (interface < USB_COMPOSITE_INTERFACE_COUNT)
                {
                    *temp16 = (*temp16 & 0xFF00) | g_device_composite.current_interface_alternate_setting[interface];
                    error = kStatus_USB_Success;
                }
                else
                {
                    error = kStatus_USB_InvalidRequest;
                }
            }
            break;
        case kUSB_DeviceEventGetDeviceDescriptor:
            if (param)
            {
                error = usb_device_get_device_descriptor(handle, (usb_device_get_device_descriptor_struct_t *)param);
            }
            break;
        case kUSB_DeviceEventGetConfigurationDescriptor:
            if (param)
            {
                error = usb_device_get_configuration_descriptor(
                    handle, (usb_device_get_configuration_descriptor_struct_t *)param);
            }
            break;
        case kUSB_DeviceEventGetStringDescriptor:
            if (param)
            {
                error = usb_device_get_string_descriptor(handle, (usb_device_get_string_descriptor_struct_t *)param);
            }
            break;
        case kUSB_DeviceEventGetHidDescriptor:
            if (param)
            {
                error = usb_device_get_hid_descriptor(handle, (usb_device_get_hid_descriptor_struct_t *)param);
            }
            break;
        case kUSB_DeviceEventGetHidReportDescriptor:
            if (param)
            {
                error = usb_device_get_hid_report_descriptor(handle,
                                                             (usb_device_get_hid_report_descriptor_struct_t *)param);
            }
            break;
        case kUSB_DeviceEventGetHidPhysicalDescriptor:
            if (param)
            {
                error = usb_device_get_hid_physical_descriptor(
                    handle, (usb_device_get_hid_physical_descriptor_struct_t *)param);
            }
            break;
    }

    return error;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : usb_device_full_init
 * Description   : device full init processing
 *
 *END**************************************************************************/
status_t usb_device_full_init(const peripheral_descriptor_t *self, serial_byte_receive_func_t function)
{
    // Not used for USB
    (void)function;

    //    uint8_t controllerId = kUSB_ControllerKhci0;
    uint8_t irqNumber;
#if defined(USB_DEVICE_CONFIG_EHCI) && (USB_DEVICE_CONFIG_EHCI > 0U)
    uint8_t usbDeviceEhciIrq[] = USBHS_IRQS;
    irqNumber = usbDeviceEhciIrq[CONTROLLER_ID - kUSB_ControllerEhci0];
#endif
#if defined(USB_DEVICE_CONFIG_KHCI) && (USB_DEVICE_CONFIG_KHCI > 0U)
    uint8_t usbDeviceKhciIrq[] = USB_IRQS;
    irqNumber = usbDeviceKhciIrq[CONTROLLER_ID - kUSB_ControllerKhci0];
#endif

    // Init the state info.
    memset(&g_device_composite, 0, sizeof(g_device_composite));

    usb_clock_init();

    g_language_ptr = &g_language_list;

    g_device_composite.speed = USB_SPEED_FULL;
    g_device_composite.attach = 0;
    g_device_composite.hid_generic.hid_handle = (class_handle_t)NULL;
    g_device_composite.device_handle = NULL;

    if (kStatus_USB_Success !=
        USB_DeviceClassInit(CONTROLLER_ID, &g_composite_device_config_list, &g_device_composite.device_handle))
    {
        return kStatus_Fail;
    }
    else
    {
        g_device_composite.hid_generic.hid_handle = g_composite_device_config_list.config[0].classHandle;
        usb_device_hid_generic_init(&g_device_composite);
    }

    /* Install isr, set priority, and enable IRQ. */
    NVIC_SetPriority((IRQn_Type)irqNumber, USB_DEVICE_INTERRUPT_PRIORITY);
    NVIC_EnableIRQ((IRQn_Type)irqNumber);

    USB_DeviceRun(g_device_composite.device_handle);

    return kStatus_Success;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : usb_device_full_shutdown
 * Description   : device shutdown processing
 *
 *END**************************************************************************/
void usb_device_full_shutdown(const peripheral_descriptor_t *self)
{
    //    uint32_t hidInfoIndex = self->instance / 2;

    if (kStatus_USB_Success != USB_DeviceClassDeinit(CONTROLLER_ID))
    {
        return;
    }
    else
    {
        // Shutdown class driver
        usb_device_hid_generic_deinit(&g_device_composite);
    }

// Make sure we are clocking to the peripheral to ensure there
// are no bus errors
#if defined(PCC_BASE_ADDRS)
    if ((self->instance == kUSB_ControllerEhci0) && ((*(volatile uint32_t *)kCLOCK_Usbfs0) & PCC_CLKCFG_CGC_MASK))
    {
        // Disable the USB interrupt
        NVIC_DisableIRQ(USB0_IRQn);

        // Clear any pending interrupts on USB
        NVIC_ClearPendingIRQ(USB0_IRQn);

        // Turn off clocking to USB
        CLOCK_DisableClock(kCLOCK_Usbfs0);
    }
#else
    if ((CONTROLLER_ID == kUSB_ControllerKhci0) && (SIM->SCGC4 & SIM_SCGC4_USBOTG_MASK))
    {
        // Disable the USB interrupt
        NVIC_DisableIRQ(USB0_IRQn);

        // Clear any pending interrupts on USB
        NVIC_ClearPendingIRQ(USB0_IRQn);

        // Turn off clocking to USB
        SIM->SCGC4 &= ~SIM_SCGC4_USBOTG_MASK;
    }
#endif // defined(PCC_BASE_ADDRS)
#if USB_DEVICE_CONFIG_EHCI
    else if ((CONTROLLER_ID == kUSB_ControllerEhci0) && (SIM->SCGC3 & SIM_SCGC3_USBHS_MASK))
    {
        // Disable the USB interrupt
        NVIC_DisableIRQ(USBHS_IRQn);

        // Clear any pending interrupts on USB
        NVIC_ClearPendingIRQ(USBHS_IRQn);

        // Turn off HS USB PHY clock gate
        SIM->SCGC3 &= ~(SIM_SCGC3_USBHS_MASK | SIM_SCGC3_USBHSPHY_MASK);
    }
#endif
}

/*FUNCTION**********************************************************************
 *
 * Function Name : usb_msc_pump
 * Description   : This function is called repeatedly by the main application
 * loop. We use it to run the state machine from non-interrupt context
 *
 *END**************************************************************************/
void usb_msc_pump(const peripheral_descriptor_t *self)
{
    s_dHidMscActivity[USB_HID_INDEX] = true;
    s_dHidMscActivity[USB_MSC_INDEX] = false;
}

status_t usb_hid_packet_init(const peripheral_descriptor_t *self)
{
//    uint32_t hidInfoIndex = self->instance / 2;
#if USB_DEVICE_CONFIG_HID
    sync_init(&g_device_composite.hid_generic.hid_packet.receiveSync, false);
    sync_init(&g_device_composite.hid_generic.hid_packet.sendSync, false);

    // Check for any received data that may be pending
    sync_signal(&g_device_composite.hid_generic.hid_packet.receiveSync);
#endif
    return kStatus_Success;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : usb_hid_packet_read
 * Description   : packet read
 *
 *END**************************************************************************/
status_t usb_hid_packet_read(const peripheral_descriptor_t *self,
                             uint8_t **packet,
                             uint32_t *packetLength,
                             packet_type_t packetType)
{
    //    uint32_t hidInfoIndex = self->instance / 2;

    if (!packet || !packetLength)
    {
        //        debug_printf("Error: invalid packet\r\n");
        return kStatus_InvalidArgument;
    }
    *packetLength = 0;

    // Determine report ID based on packet type.
    uint8_t reportID;
    switch (packetType)
    {
        case kPacketType_Command:
            reportID = kBootloaderReportID_CommandOut;
            break;
        case kPacketType_Data:
            reportID = kBootloaderReportID_DataOut;
            break;
        default:
            //            debug_printf("usbhid: unsupported packet type %d\r\n", (int)packetType);
            return kStatus_Fail;
    };
#if USB_DEVICE_CONFIG_HID
    if (s_dHidMscActivity[USB_HID_INDEX])
    {
        // The first receive data request was initiated after enumeration.
        // After that we wait until we are ready to read data before
        // we request more. This mechanism prevents data loss
        // by allowing the USB controller to hold off the host with NAKs
        // on the interrupt out pipe until we are ready.
        if (g_device_composite.hid_generic.hid_packet.isReceiveDataRequestRequired)
        {
            // Initiate receive on interrupt out pipe.
            USB_DeviceHidRecv(g_device_composite.hid_generic.hid_handle, USB_HID_GENERIC_ENDPOINT_OUT,
                              (uint8_t *)&g_device_composite.hid_generic.hid_packet.report.header,
                              sizeof(g_device_composite.hid_generic.hid_packet.report));
        }

        g_device_composite.hid_generic.hid_packet.isReceiveDataRequestRequired = true;

        // Wait until we have received a report.

        sync_wait(&g_device_composite.hid_generic.hid_packet.receiveSync, kSyncWaitForever);

        // Check the report ID, the first byte of the report buffer.
        if (g_device_composite.hid_generic.hid_packet.report.header.reportID != reportID)
        {
            // If waiting for a command but get data, this is a flush after a data abort.
            if ((reportID == kBootloaderReportID_CommandOut) &&
                (g_device_composite.hid_generic.hid_packet.report.header.reportID == kBootloaderReportID_DataOut))
            {
                return -1; // kStatus_AbortDataPhase;
            }
            //        debug_printf("usbhid: received unexpected report=%x\r\n",
            //        g_device_composite.hid_generic.hid_packet.report.header.reportID);
            return kStatus_Fail;
        }

        // Extract the packet length encoded as bytes 1 and 2 of the report. The packet length
        // is transferred in little endian byte order.
        uint16_t lengthOfPacket = g_device_composite.hid_generic.hid_packet.report.header.packetLengthLsb |
                                  (g_device_composite.hid_generic.hid_packet.report.header.packetLengthMsb << 8);

        // Make sure we got all of the packet. Some hosts (Windows) may send up to the maximum
        // report size, so there may be extra trailing bytes.
        if ((g_device_composite.hid_generic.hid_packet.reportSize -
             sizeof(g_device_composite.hid_generic.hid_packet.report.header)) < lengthOfPacket)
        {
            //        debug_printf("usbhid: received only %d bytes of packet with length %d\r\n",
            //        s_hidInfo[hidInfoIndex].reportSize - 3, lengthOfPacket);
            return kStatus_Fail;
        }

        // Return packet to caller.
        *packet = g_device_composite.hid_generic.hid_packet.report.packet;
        *packetLength = lengthOfPacket;
    }
    return kStatus_Success;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : usb_hid_packet_write
 * Description   : packet write
 *
 *END**************************************************************************/
status_t usb_hid_packet_write(const peripheral_descriptor_t *self,
                              const uint8_t *packet,
                              uint32_t byteCount,
                              packet_type_t packetType)
{
#if USB_DEVICE_CONFIG_HID
    //    uint32_t hidInfoIndex = self->instance / 2;
    if (s_dHidMscActivity[USB_HID_INDEX])
    {
        if (byteCount > kMinPacketBufferSize)
        {
            debug_printf("Error: invalid packet size %d\r\n", byteCount);
            return kStatus_InvalidArgument;
        }

        // Determine report ID based on packet type.
        uint8_t reportID;
        switch (packetType)
        {
            case kPacketType_Command:
                reportID = kBootloaderReportID_CommandIn;
                break;
            case kPacketType_Data:
                reportID = kBootloaderReportID_DataIn;
                break;
            default:
                debug_printf("usbhid: unsupported packet type %d\r\n", (int)packetType);
                return kStatus_Fail;
        };

        // Check for data phase aborted by receiver.
        lock_acquire();
        if (g_device_composite.hid_generic.hid_packet.didReceiveDataPhaseAbort)
        {
            g_device_composite.hid_generic.hid_packet.didReceiveDataPhaseAbort = false;
            lock_release();
            return -1; // kStatus_AbortDataPhase;
        }
        lock_release();

        // Construct report contents.
        g_device_composite.hid_generic.hid_packet.report.header.reportID = reportID;
        g_device_composite.hid_generic.hid_packet.report.header._padding = 0;
        g_device_composite.hid_generic.hid_packet.report.header.packetLengthLsb = byteCount & 0xff;
        g_device_composite.hid_generic.hid_packet.report.header.packetLengthMsb = (byteCount >> 8) & 0xff;
        if (packet && byteCount > 0)
        {
            memcpy(&g_device_composite.hid_generic.hid_packet.report.packet, packet, byteCount);
        }
        if (g_device_composite.hid_generic.attach == 1)
        {
            // Send the maximum report size since that's what the host expects.
            // There may be extra trailing bytes.
            USB_DeviceHidSend(g_device_composite.hid_generic.hid_handle, USB_HID_GENERIC_ENDPOINT_IN,
                              (uint8_t *)&g_device_composite.hid_generic.hid_packet.report.header,
                              sizeof(g_device_composite.hid_generic.hid_packet.report));
            sync_wait(&g_device_composite.hid_generic.hid_packet.sendSync, kSyncWaitForever);
        }
    }
#endif // USB_DEVICE_CONFIG_HID
    return kStatus_Success;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : usb_hid_packet_abort_data_phase
 * Description   : packet abort data phase processing
 *
 *END**************************************************************************/
static void usb_hid_packet_abort_data_phase(const peripheral_descriptor_t *self)
{
    status_t status = self->packetInterface->writePacket(self, NULL, 0, kPacketType_Command);
    if (status != kStatus_Success)
    {
        debug_printf("Error: usb_hid_packet_abort write packet returned status 0x%x\r\n", status);
        return;
    }
}

/*FUNCTION**********************************************************************
 *
 * Function Name : usb_hid_packet_finalize
 * Description   : place holder
 *
 *END**************************************************************************/
static status_t usb_hid_packet_finalize(const peripheral_descriptor_t *self)
{
    return kStatus_Success;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : usb_hid_packet_get_max_packet_size
 * Description   : get buffer size
 *
 *END**************************************************************************/
static uint32_t usb_hid_packet_get_max_packet_size(const peripheral_descriptor_t *self)
{
    return kMinPacketBufferSize;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : USBHS_IRQHandler
 * Description   : USB EHCI interrup hander
 *
 *END**************************************************************************/
#if defined(USB_DEVICE_CONFIG_EHCI) && (USB_DEVICE_CONFIG_EHCI > 0)
void USBHS_IRQHandler(void)
{
    USB_DeviceEhciIsrFunction(g_device_composite.device_handle);
}
#endif

/*FUNCTION**********************************************************************
 *
 * Function Name : USB0_IRQHandler
 * Description   : USB KHCI interrup hander
 *
 *END**************************************************************************/
/*

#if defined(USB_DEVICE_CONFIG_KHCI) && (USB_DEVICE_CONFIG_KHCI > 0)
void USB0_IRQHandler(void)
{
    USB_DeviceKhciIsrFunction(g_device_composite.device_handle);
}
#endif
*/

#endif

/*FUNCTION**********************************************************************
 *
 * Function Name : get_bus_clock
 * Description   : Gets bus clock
 *
 *END**************************************************************************/
uint32_t get_bus_clock(void)
{
    uint32_t busClockDivider = ((SIM->CLKDIV1 & SIM_CLKDIV1_OUTDIV2_MASK) >> SIM_CLKDIV1_OUTDIV2_SHIFT) + 1;
    return (SystemCoreClock / busClockDivider);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : get_fast_peripheral_clock
 * Description   : fast peripheral clock
 *
 *END**************************************************************************/
uint32_t get_fast_peripheral_clock(void)
{
    uint32_t busClockDivider = ((SIM->CLKDIV1 & SIM_CLKDIV1_OUTDIV2_MASK) >> SIM_CLKDIV1_OUTDIV2_SHIFT) + 1;
    return (SystemCoreClock / busClockDivider);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : init_hardware
 * Description   : hardware initialization
 *
 *END**************************************************************************/
void init_hardware(void)
{
    // Disable the MPU otherwise USB cannot access the bus
//    MPU->CESR = 0;

    SIM->SCGC5 |= (SIM_SCGC5_PORTA_MASK | SIM_SCGC5_PORTB_MASK | SIM_SCGC5_PORTC_MASK | SIM_SCGC5_PORTD_MASK |
                   SIM_SCGC5_PORTE_MASK);

    // Enable pins for I2C0 on PTD8 - PTD9.
    PORTE->PCR[18] = PORT_PCR_MUX(4) | PORT_PCR_ODE_MASK; // I2C0_SCL is ALT2 for pin PTD9, I2C0_SCL set for open drain
    PORTE->PCR[19] = PORT_PCR_MUX(4) | PORT_PCR_ODE_MASK; // I2C0_SDA is ALT2 for pin PTD9, I2C0_SDA set for open drain

    microseconds_init();

    init_i2c(0);
    usb_device_full_init(&g_peripherals[0], 0);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : init_i2c
 * Description   : I2C init function
 *
 *END**************************************************************************/
void init_i2c(uint32_t instance)
{
    uint32_t baseAddr = g_i2cBaseAddr[instance];
    i2c_master_config_t config;

    I2C_MasterGetDefaultConfig(&config);

    I2C_MasterInit((I2C_Type *)baseAddr, &config, get_bus_clock());
    I2C_MasterTransferCreateHandle((I2C_Type *)baseAddr, &s_i2cHandle, NULL, NULL);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : host_start_command_rx
 * Description   : receiving host start command process
 *
 *END**************************************************************************/
void host_start_command_rx(uint8_t *dest, uint32_t length)
{
    s_rxData = dest;
    s_bytesRx = 0;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : host_stop_command_rx
 * Description   : receiving host stop command process
 *
 *END**************************************************************************/
void host_stop_command_rx(void)
{
    s_rxData = 0;
    s_bytesRx = 0;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : get_bytes_received_from_host
 * Description   : receiving host get bytes command process
 *
 *END**************************************************************************/
uint32_t get_bytes_received_from_host(void)
{
    return s_bytesRx;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : write_bytes_to_host
 * Description   : sending host bytes command process
 *
 *END**************************************************************************/
void write_bytes_to_host(uint8_t *src, uint32_t length)
{
    uint32_t baseAddr = g_uartBaseAddr[4];

    UART_WriteBlocking((UART_Type *)baseAddr, src, length);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : configure_i2c_address
 * Description   : i2c config address process
 *
 *END**************************************************************************/
void configure_i2c_address(uint8_t address)
{
    s_i2cUserConfig.slaveAddress = address;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : configure_i2c_speed
 * Description   : i2c config speed process
 *
 *END**************************************************************************/
void configure_i2c_speed(uint32_t speedkhz)
{
    s_i2cUserConfig.baudRate_kbps = speedkhz;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : send_i2c_data
 * Description   : i2c sending data process
 *
 *END**************************************************************************/
status_t send_i2c_data(uint8_t *src, uint32_t writeLength)
{
    i2c_master_transfer_t send_data;
    uint32_t baseAddr = g_i2cBaseAddr[0];
    send_data.slaveAddress = s_i2cUserConfig.slaveAddress;
    send_data.direction = kI2C_Write;
    send_data.data = src;
    send_data.dataSize = writeLength;
    send_data.subaddress = 0;
    send_data.subaddressSize = 0;
    send_data.flags = kI2C_TransferDefaultFlag;

    I2C_MasterTransferBlocking((I2C_Type *)baseAddr, &send_data);

    return kStatus_Success;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : receive_i2c_data
 * Description   : i2c receiving data process
 *
 *END**************************************************************************/
status_t receive_i2c_data(uint8_t *dest, uint32_t readLength)
{
    i2c_master_transfer_t receive_data;
    uint32_t baseAddr = g_i2cBaseAddr[0];
    receive_data.slaveAddress = s_i2cUserConfig.slaveAddress;
    receive_data.direction = kI2C_Read;
    receive_data.data = dest;
    receive_data.dataSize = readLength;
    receive_data.subaddress = 0;
    receive_data.subaddressSize = 0;
    receive_data.flags = kI2C_TransferDefaultFlag;

    I2C_MasterTransferBlocking((I2C_Type *)baseAddr, &receive_data);

    return kStatus_Success;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : getPortBaseAddrFromAscii
 * Description   : PORT get base address function
 *
 *END**************************************************************************/
PORT_Type *getPortBaseAddrFromAscii(uint8_t port)
{
    if ((port >= 'a') && (port <= 'e'))
    {
        port = port - 'a';
    }
    else if ((port >= 'A') && (port <= 'E'))
    {
        port = port - 'A';
    }

    switch (port)
    {
        default:
        case 0:
            return PORTA;
        case 1:
            return PORTB;
        case 2:
            return PORTC;
        case 3:
            return PORTD;
        case 4:
            return PORTE;
    }
}

/*FUNCTION**********************************************************************
 *
 * Function Name : getGpioBaseAddrFromAscii
 * Description   : GPIO get base address function
 *
 *END**************************************************************************/
GPIO_Type *getGpioBaseAddrFromAscii(uint8_t port)
{
    if ((port >= 'a') && (port <= 'e'))
    {
        port = port - 'a';
    }
    else if ((port >= 'A') && (port <= 'E'))
    {
        port = port - 'A';
    }

    switch (port)
    {
        default:
        case 0:
            return GPIOA;
        case 1:
            return GPIOB;
        case 2:
            return GPIOC;
        case 3:
            return GPIOD;
        case 4:
            return GPIOE;
    }
}

/*FUNCTION**********************************************************************
 *
 * Function Name : configure_gpio
 * Description   : GPIO config processing
 *
 *END**************************************************************************/
void configure_gpio(uint8_t port, uint8_t pinNum, uint8_t muxVal)
{
    PORT_Type *realPort = getPortBaseAddrFromAscii(port);
    realPort->PCR[pinNum] = ((~PORT_PCR_MUX_MASK) & realPort->PCR[pinNum]) | PORT_PCR_MUX(muxVal);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : set_gpio
 * Description   : GPIO set up function
 *
 *END**************************************************************************/
void set_gpio(uint8_t port, uint8_t pinNum, uint8_t level)
{
    GPIO_Type *realPort = getGpioBaseAddrFromAscii(port);

    realPort->PDDR |= 1 << pinNum;

    if (level)
    {
        realPort->PSOR |= 1 << pinNum;
    }
    else
    {
        realPort->PCOR |= 1 << pinNum;
    }
}
