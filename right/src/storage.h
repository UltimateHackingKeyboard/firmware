#ifndef __STORAGE_H__
#define __STORAGE_H__

// Includes:

    #include <stdbool.h>
    // #include "config_parser/config_globals.h"

// Typedefs:

    typedef enum {
        StorageOperation_Read,
        StorageOperation_Write,
    } storage_operation_t;

// Variables:

    extern volatile bool IsStorageBusy;

// Functions:

    bool IsStorageOperationValid(storage_operation_t operation);

#endif
