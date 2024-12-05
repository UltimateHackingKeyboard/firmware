#include "storage.h"

volatile bool IsStorageBusy;

bool IsStorageOperationValid(storage_operation_t operation)
{
    return operation == StorageOperation_Read || operation == StorageOperation_Write;
}
