#ifndef __BUS_PAL_HARDWARE_H__
#define __BUS_PAL_HARDWARE_H__

#include "fsl_i2c.h"
#include "bl_peripheral.h"

typedef struct _i2c_user_config {
    uint8_t slaveAddress;
    uint16_t baudRate_kbps;
} i2c_user_config_t;

uint32_t get_bus_clock(void);
void init_hardware(void);
void write_bytes_to_host(uint8_t *src, uint32_t length);
void host_start_command_rx(uint8_t *dest, uint32_t length);
void host_stop_command_rx(void);
uint32_t get_bytes_received_from_host(void);
void configure_i2c_address(uint8_t address);
void configure_i2c_speed(uint32_t speedkhz);
status_t send_i2c_data(uint8_t *src, uint32_t writeLength);
status_t receive_i2c_data(uint8_t *dest, uint32_t readLength);
bool usb_hid_poll_for_activity(const peripheral_descriptor_t *self);
status_t usb_hid_packet_init(const peripheral_descriptor_t *self);
status_t usb_hid_packet_read(const peripheral_descriptor_t *self, uint8_t **packet, uint32_t *packetLength, packet_type_t packetType);
status_t usb_hid_packet_write(const peripheral_descriptor_t *self, const uint8_t *packet, uint32_t byteCount,packet_type_t packetType);
extern void usb_msc_pump(const peripheral_descriptor_t *self);

#endif
