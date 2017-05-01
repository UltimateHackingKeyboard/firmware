#include "bus_pal_hardware.h"
#include "usb_descriptor.h"
#include "usb_device_config.h"
#include "composite.h"
#include "bootloader_config.h"
#include "microseconds/microseconds.h"
#include "i2c.h"
#include "peripherals/test_led.h"
#include "usb_composite_device.h"

bool is_usb_active();
static status_t usb_device_full_init(const peripheral_descriptor_t *self, serial_byte_receive_func_t function);
static void usb_device_full_shutdown(const peripheral_descriptor_t *self);

status_t usb_hid_packet_init(const peripheral_descriptor_t *self);
static void usb_hid_packet_abort_data_phase(const peripheral_descriptor_t *self);
static status_t usb_hid_packet_finalize(const peripheral_descriptor_t *self);
static uint32_t usb_hid_packet_get_max_packet_size(const peripheral_descriptor_t *self);
static void init_i2c(uint32_t instance);

const peripheral_control_interface_t g_usbHidControlInterface = {.init = usb_device_full_init,
                                                                 .shutdown = usb_device_full_shutdown};

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
     .controlInterface = &g_usbHidControlInterface,
     .byteInterface = NULL,
     .packetInterface = &g_usbHidPacketInterface },
    { 0 } // Terminator
};

usb_device_composite_struct_t BuspalCompositeUsbDevice;
usb_status_t usb_device_callback(usb_device_handle handle, uint32_t event, void *param);

static i2c_user_config_t s_i2cUserConfig = {.slaveAddress = 0x10, //!< The slave's 7-bit address
                                            .baudRate_kbps = 100 };

static i2c_master_handle_t s_i2cHandle;

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

bool is_usb_active()
{
    return BuspalCompositeUsbDevice.attach && BuspalCompositeUsbDevice.hid_generic.hid_packet.didReceiveFirstReport;
}

usb_status_t usb_device_callback(usb_device_handle handle, uint32_t event, void *param)
{
    usb_status_t error = kStatus_USB_Success;
    uint16_t *temp16 = (uint16_t *)param;
    uint8_t *temp8 = (uint8_t *)param;

    switch (event)
    {
        case kUSB_DeviceEventBusReset:
        {
            BuspalCompositeUsbDevice.attach = 0;
        }
        break;
        case kUSB_DeviceEventSetConfiguration:
            if (param)
            {
                BuspalCompositeUsbDevice.attach = 1;
                BuspalCompositeUsbDevice.current_configuration = *temp8;
                error = usb_device_hid_generic_set_configure(BuspalCompositeUsbDevice.hid_generic.hid_handle, *temp8);
                error = kStatus_USB_Success;
            }
            break;
        case kUSB_DeviceEventSetInterface:
            if (BuspalCompositeUsbDevice.attach)
            {
                uint8_t interface = (uint8_t)((*temp16 & 0xFF00U) >> 0x08U);
                uint8_t alternate_setting = (uint8_t)(*temp16 & 0x00FFU);
                if (interface < USB_COMPOSITE_INTERFACE_COUNT)
                {
                    BuspalCompositeUsbDevice.current_interface_alternate_setting[interface] = alternate_setting;
                    usb_device_hid_generic_set_interface(BuspalCompositeUsbDevice.hid_generic.hid_handle, interface,
                                                         alternate_setting);
                    error = kStatus_USB_Success;
                }
            }
            break;
        case kUSB_DeviceEventGetConfiguration:
            if (param)
            {
                *temp8 = BuspalCompositeUsbDevice.current_configuration;
                error = kStatus_USB_Success;
            }
            break;
        case kUSB_DeviceEventGetInterface:
            if (param)
            {
                uint8_t interface = (uint8_t)((*temp16 & 0xFF00) >> 0x08);
                if (interface < USB_COMPOSITE_INTERFACE_COUNT)
                {
                    *temp16 = (*temp16 & 0xFF00) | BuspalCompositeUsbDevice.current_interface_alternate_setting[interface];
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
                error = usb_device_get_hid_report_descriptor(handle, (usb_device_get_hid_report_descriptor_struct_t *)param);
            }
            break;
        case kUSB_DeviceEventGetHidPhysicalDescriptor:
            if (param)
            {
                error = usb_device_get_hid_physical_descriptor( handle, (usb_device_get_hid_physical_descriptor_struct_t *)param);
            }
            break;
    }

    return error;
}

status_t usb_device_full_init(const peripheral_descriptor_t *self, serial_byte_receive_func_t function)
{
    // Not used for USB
    (void)function;

    uint8_t irqNumber;
    uint8_t usbDeviceKhciIrq[] = USB_IRQS;
    irqNumber = usbDeviceKhciIrq[CONTROLLER_ID - kUSB_ControllerKhci0];

    // Init the state info.
    memset(&BuspalCompositeUsbDevice, 0, sizeof(BuspalCompositeUsbDevice));

    usb_clock_init();

    g_language_ptr = &g_language_list;

    BuspalCompositeUsbDevice.speed = USB_SPEED_FULL;
    BuspalCompositeUsbDevice.attach = 0;
    BuspalCompositeUsbDevice.hid_generic.hid_handle = (class_handle_t)NULL;
    BuspalCompositeUsbDevice.device_handle = NULL;

    if (kStatus_USB_Success != USB_DeviceClassInit(CONTROLLER_ID, &g_composite_device_config_list, &BuspalCompositeUsbDevice.device_handle)) {
        return kStatus_Fail;
    } else {
        BuspalCompositeUsbDevice.hid_generic.hid_handle = g_composite_device_config_list.config[0].classHandle;
        usb_device_hid_generic_init(&BuspalCompositeUsbDevice);
    }

    /* Install isr, set priority, and enable IRQ. */
    NVIC_SetPriority((IRQn_Type)irqNumber, USB_DEVICE_INTERRUPT_PRIORITY);
    NVIC_EnableIRQ((IRQn_Type)irqNumber);

    USB_DeviceRun(BuspalCompositeUsbDevice.device_handle);

    return kStatus_Success;
}

void usb_device_full_shutdown(const peripheral_descriptor_t *self)
{
    if (kStatus_USB_Success != USB_DeviceClassDeinit(CONTROLLER_ID)) {
        return;
    }

    usb_device_hid_generic_deinit(&BuspalCompositeUsbDevice); // Shutdown class driver

    // Make sure we are clocking to the peripheral to ensure there are no bus errors
    if (SIM->SCGC4 & SIM_SCGC4_USBOTG_MASK) {
        NVIC_DisableIRQ(USB0_IRQn);           // Disable the USB interrupt
        NVIC_ClearPendingIRQ(USB0_IRQn);      // Clear any pending interrupts on USB
        SIM->SCGC4 &= ~SIM_SCGC4_USBOTG_MASK; // Turn off clocking to USB
    }
}

status_t usb_hid_packet_init(const peripheral_descriptor_t *self)
{
    sync_init(&BuspalCompositeUsbDevice.hid_generic.hid_packet.receiveSync, false);
    sync_init(&BuspalCompositeUsbDevice.hid_generic.hid_packet.sendSync, false);

    // Check for any received data that may be pending
    sync_signal(&BuspalCompositeUsbDevice.hid_generic.hid_packet.receiveSync);
    return kStatus_Success;
}

status_t usb_hid_packet_read(const peripheral_descriptor_t *self,
                             uint8_t **packet,
                             uint32_t *packetLength,
                             packet_type_t packetType)
{
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
    if (is_usb_active())
    {
        // The first receive data request was initiated after enumeration.
        // After that we wait until we are ready to read data before
        // we request more. This mechanism prevents data loss
        // by allowing the USB controller to hold off the host with NAKs
        // on the interrupt out pipe until we are ready.
        if (BuspalCompositeUsbDevice.hid_generic.hid_packet.isReceiveDataRequestRequired)
        {
            // Initiate receive on interrupt out pipe.
            USB_DeviceHidRecv(BuspalCompositeUsbDevice.hid_generic.hid_handle, USB_HID_GENERIC_ENDPOINT_OUT,
                              (uint8_t *)&BuspalCompositeUsbDevice.hid_generic.hid_packet.report.header,
                              sizeof(BuspalCompositeUsbDevice.hid_generic.hid_packet.report));
        }

        BuspalCompositeUsbDevice.hid_generic.hid_packet.isReceiveDataRequestRequired = true;

        // Wait until we have received a report.

        sync_wait(&BuspalCompositeUsbDevice.hid_generic.hid_packet.receiveSync, kSyncWaitForever);

        // Check the report ID, the first byte of the report buffer.
        if (BuspalCompositeUsbDevice.hid_generic.hid_packet.report.header.reportID != reportID)
        {
            // If waiting for a command but get data, this is a flush after a data abort.
            if ((reportID == kBootloaderReportID_CommandOut) &&
                (BuspalCompositeUsbDevice.hid_generic.hid_packet.report.header.reportID == kBootloaderReportID_DataOut))
            {
                return -1; // kStatus_AbortDataPhase;
            }
            //        debug_printf("usbhid: received unexpected report=%x\r\n",
            //        g_device_composite.hid_generic.hid_packet.report.header.reportID);
            return kStatus_Fail;
        }

        // Extract the packet length encoded as bytes 1 and 2 of the report. The packet length
        // is transferred in little endian byte order.
        uint16_t lengthOfPacket = BuspalCompositeUsbDevice.hid_generic.hid_packet.report.header.packetLengthLsb |
                                  (BuspalCompositeUsbDevice.hid_generic.hid_packet.report.header.packetLengthMsb << 8);

        // Make sure we got all of the packet. Some hosts (Windows) may send up to the maximum
        // report size, so there may be extra trailing bytes.
        if ((BuspalCompositeUsbDevice.hid_generic.hid_packet.reportSize -
             sizeof(BuspalCompositeUsbDevice.hid_generic.hid_packet.report.header)) < lengthOfPacket)
        {
            //        debug_printf("usbhid: received only %d bytes of packet with length %d\r\n",
            //        s_hidInfo[hidInfoIndex].reportSize - 3, lengthOfPacket);
            return kStatus_Fail;
        }

        // Return packet to caller.
        *packet = BuspalCompositeUsbDevice.hid_generic.hid_packet.report.packet;
        *packetLength = lengthOfPacket;
    }
    return kStatus_Success;
}

status_t usb_hid_packet_write(const peripheral_descriptor_t *self,
                              const uint8_t *packet,
                              uint32_t byteCount,
                              packet_type_t packetType)
{
    if (is_usb_active())
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
        if (BuspalCompositeUsbDevice.hid_generic.hid_packet.didReceiveDataPhaseAbort)
        {
            BuspalCompositeUsbDevice.hid_generic.hid_packet.didReceiveDataPhaseAbort = false;
            lock_release();
            return -1; // kStatus_AbortDataPhase;
        }
        lock_release();

        // Construct report contents.
        BuspalCompositeUsbDevice.hid_generic.hid_packet.report.header.reportID = reportID;
        BuspalCompositeUsbDevice.hid_generic.hid_packet.report.header._padding = 0;
        BuspalCompositeUsbDevice.hid_generic.hid_packet.report.header.packetLengthLsb = byteCount & 0xff;
        BuspalCompositeUsbDevice.hid_generic.hid_packet.report.header.packetLengthMsb = (byteCount >> 8) & 0xff;
        if (packet && byteCount > 0)
        {
            memcpy(&BuspalCompositeUsbDevice.hid_generic.hid_packet.report.packet, packet, byteCount);
        }
        if (BuspalCompositeUsbDevice.hid_generic.attach == 1)
        {
            // Send the maximum report size since that's what the host expects.
            // There may be extra trailing bytes.
            USB_DeviceHidSend(BuspalCompositeUsbDevice.hid_generic.hid_handle, USB_HID_GENERIC_ENDPOINT_IN,
                              (uint8_t *)&BuspalCompositeUsbDevice.hid_generic.hid_packet.report.header,
                              sizeof(BuspalCompositeUsbDevice.hid_generic.hid_packet.report));
            sync_wait(&BuspalCompositeUsbDevice.hid_generic.hid_packet.sendSync, kSyncWaitForever);
        }
    }
    return kStatus_Success;
}

static void usb_hid_packet_abort_data_phase(const peripheral_descriptor_t *self)
{
    status_t status = self->packetInterface->writePacket(self, NULL, 0, kPacketType_Command);
    if (status != kStatus_Success)
    {
        debug_printf("Error: usb_hid_packet_abort write packet returned status 0x%x\r\n", status);
        return;
    }
}

static status_t usb_hid_packet_finalize(const peripheral_descriptor_t *self)
{
    return kStatus_Success;
}

static uint32_t usb_hid_packet_get_max_packet_size(const peripheral_descriptor_t *self)
{
    return kMinPacketBufferSize;
}

uint32_t get_bus_clock(void)
{
    uint32_t busClockDivider = ((SIM->CLKDIV1 & SIM_CLKDIV1_OUTDIV2_MASK) >> SIM_CLKDIV1_OUTDIV2_SHIFT) + 1;
    return (SystemCoreClock / busClockDivider);
}

void init_hardware(void)
{
    microseconds_init();
    init_i2c(0);
    usb_device_full_init(&g_peripherals[0], 0);
}

void init_i2c(uint32_t instance)
{
    I2C_MasterTransferCreateHandle(I2C_MAIN_BUS_BASEADDR, &s_i2cHandle, NULL, NULL);
}

void configure_i2c_address(uint8_t address)
{
    s_i2cUserConfig.slaveAddress = address;
}

void configure_i2c_speed(uint32_t speedkhz)
{
    s_i2cUserConfig.baudRate_kbps = speedkhz;
}

status_t send_i2c_data(uint8_t *src, uint32_t writeLength)
{
    i2c_master_transfer_t send_data;
    send_data.slaveAddress = s_i2cUserConfig.slaveAddress;
    send_data.direction = kI2C_Write;
    send_data.data = src;
    send_data.dataSize = writeLength;
    send_data.subaddress = 0;
    send_data.subaddressSize = 0;
    send_data.flags = kI2C_TransferDefaultFlag;

    I2C_MasterTransferBlocking(I2C_MAIN_BUS_BASEADDR, &send_data);

    return kStatus_Success;
}

status_t receive_i2c_data(uint8_t *dest, uint32_t readLength)
{
    i2c_master_transfer_t receive_data;
    receive_data.slaveAddress = s_i2cUserConfig.slaveAddress;
    receive_data.direction = kI2C_Read;
    receive_data.data = dest;
    receive_data.dataSize = readLength;
    receive_data.subaddress = 0;
    receive_data.subaddressSize = 0;
    receive_data.flags = kI2C_TransferDefaultFlag;

    I2C_MasterTransferBlocking(I2C_MAIN_BUS_BASEADDR, &receive_data);

    return kStatus_Success;
}
