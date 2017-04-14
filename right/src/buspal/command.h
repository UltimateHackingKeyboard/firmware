#ifndef __COMMAND_H__
#define __COMMAND_H__

#include "packet/serial_packet.h"

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

typedef struct CommandHandlerEntry
{
    void (*handleCommand)(uint8_t *packet, uint32_t packetLength);
    status_t (*handleData)(bool *hasMoreData);
} command_handler_entry_t;

typedef struct CommandProcessorData {
    int32_t state;                   // Current state machine state
    uint8_t *packet;                 // Pointer to packet in process
    uint32_t packetLength;           // Length of packet in process
    struct DataPhase {
        uint8_t *data;               // Data for data phase
        uint32_t count;              // Remaining count to produce/consume
        uint32_t address;            // Address for data phase
        uint32_t dataBytesAvailable; // Number of bytes available at data pointer
        uint8_t commandTag;          // Tag of command running data phase
        uint8_t option;              // Option for special command
    } dataPhase;
    const command_handler_entry_t *handlerEntry; // Pointer to handler table entry for packet in process
} command_processor_data_t;

// Interface to command processor operations.
typedef struct CommandInterface {
    command_processor_data_t *stateData;
} command_interface_t;

// Default command interface.
extern command_interface_t g_commandInterface;

void handleUsbBusPalCommand();

// Initialize the command processor component.
status_t bootloader_command_init(void);

// Pump the command state machine. Executes one command or data phase transaction.
status_t bootloader_command_pump(void);

#endif
