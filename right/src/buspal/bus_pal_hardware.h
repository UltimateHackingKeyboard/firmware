#if !defined(__BUS_PAL_HARDWARE_H__)
#define __BUS_PAL_HARDWARE_H__

#include "fsl_dspi.h"
#include "fsl_i2c.h"
#include "bl_peripheral.h"

uint32_t get_bus_clock(void);

/*!
 * @brief user config from host for i2c
 */
typedef struct _i2c_user_config
{
    uint8_t slaveAddress;
    uint16_t baudRate_kbps;
} i2c_user_config_t;

/*!
 * @brief user config from host for spi
 */
typedef struct _dspi_user_config
{
    dspi_clock_polarity_t polarity;   /*!< Clock polarity */
    dspi_clock_phase_t phase;         /*!< Clock phase */
    dspi_shift_direction_t direction; /*!< MSB or LSB */
    uint32_t baudRate_Bps;            /*!< Baud Rate for SPI in Hz */
    uint32_t clock_Hz;
} dspi_user_config_t;

/*!
 * @brief hardware initialization
 */
void init_hardware(void);

//! @brief sending host bytes command process
void write_bytes_to_host(uint8_t *src, uint32_t length);

//! @brief receiving host start command process
void host_start_command_rx(uint8_t *dest, uint32_t length);

//! @brief receiving host stop command process
void host_stop_command_rx(void);

//! @brief receiving host get bytes command process
uint32_t get_bytes_received_from_host(void);

//! @brief i2c config address process
void configure_i2c_address(uint8_t address);

//! @brief i2c config speed process
void configure_i2c_speed(uint32_t speedkhz);

//! @brief i2c sending data process
status_t send_i2c_data(uint8_t *src, uint32_t writeLength);

//! @brief i2c receiving data process
status_t receive_i2c_data(uint8_t *dest, uint32_t readLength);

//! @brief GPIO config processing
void configure_gpio(uint8_t port, uint8_t pinNum, uint8_t muxVal);

//! @brief GPIO set up function
void set_gpio(uint8_t port, uint8_t pinNum, uint8_t level);

//! @brief fpga clock set function
void set_fpga_clock(uint32_t clock);

bool usb_hid_poll_for_activity(const peripheral_descriptor_t *self);
status_t usb_hid_packet_init(const peripheral_descriptor_t *self);
status_t usb_hid_packet_read(const peripheral_descriptor_t *self,
                             uint8_t **packet,
                             uint32_t *packetLength,
                             packet_type_t packetType);
status_t usb_hid_packet_write(const peripheral_descriptor_t *self,
                              const uint8_t *packet,
                              uint32_t byteCount,
                              packet_type_t packetType);
#endif // __BUS_PAL_HARDWARE_H__
