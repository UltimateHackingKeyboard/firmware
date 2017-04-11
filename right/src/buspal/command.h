#ifndef _command_h
#define _command_h

#include "packet/serial_packet.h"
#include "packet/command_packet.h"

//! @addtogroup command
//! @{

////////////////////////////////////////////////////////////////////////////////
// Definitions
////////////////////////////////////////////////////////////////////////////////

//! @brief Command state machine states.
enum _command_state
{
    kCommandState_CommandPhase,
    kCommandState_DataPhase,
    kCommandState_DataPhaseRead,
    kCommandState_DataPhaseWrite
};

typedef enum _buspal_state
{
    kBuspal_Idle,
    kBuspal_I2c,
} buspal_state_t;

typedef struct ConfigI2cCmdPacket
{
    command_packet_t commandPacket; //!< header
    uint32_t address;               //!< Parameter 0: i2c slave address.
    uint32_t speed;                 //!< Parameter 1: i2c speed in kHz
} config_i2c_cmd_packet_t;

//! @brief Format of command handler entry.
typedef struct CommandHandlerEntry
{
    void (*handleCommand)(uint8_t *packet, uint32_t packetLength);
    status_t (*handleData)(bool *hasMoreData);
} command_handler_entry_t;

//! @brief Command processor data format.
typedef struct CommandProcessorData
{
    int32_t state;         //!< Current state machine state
    uint8_t *packet;       //!< Pointer to packet in process
    uint32_t packetLength; //!< Length of packet in process
    struct DataPhase
    {
        uint8_t *data;               //!< Data for data phase
        uint32_t count;              //!< Remaining count to produce/consume
        uint32_t address;            //!< Address for data phase
        uint32_t dataBytesAvailable; //!< Number of bytes available at data pointer
        uint8_t commandTag;          //!< Tag of command running data phase
        uint8_t option;              //!< option for special command
    } dataPhase;
    const command_handler_entry_t *handlerEntry; //! Pointer to handler table entry for packet in process
} command_processor_data_t;

//! @brief Interface to command processor operations.
typedef struct CommandInterface
{
    status_t (*init)(void);
    status_t (*pump)(void);
    const command_handler_entry_t *handlerTable;
    command_processor_data_t *stateData;
} command_interface_t;

////////////////////////////////////////////////////////////////////////////////
// Externals
////////////////////////////////////////////////////////////////////////////////

//! @brief Default command interface.
extern command_interface_t g_commandInterface;

////////////////////////////////////////////////////////////////////////////////
// Prototypes
////////////////////////////////////////////////////////////////////////////////

#if defined(__cplusplus)
extern "C" {
#endif // __cplusplus

//! @brief Initialize the command processor component.
status_t bootloader_command_init(void);

//! @brief Pump the command state machine.
//!
//! Executes one command or data phase transaction.
status_t bootloader_command_pump(void);

#if defined(__cplusplus)
}
#endif // __cplusplus

//! @}

#endif // _command.h_
