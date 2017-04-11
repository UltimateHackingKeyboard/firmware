#if !defined(__BOOTLOADER_H__)
#define __BOOTLOADER_H__

#include "bootloader_common.h"
#include "bootloader/bl_peripheral.h"
//#include "bootloader/bl_command.h"
#include "bootloader/bl_context.h"
//#include "bootloader/bl_version.h"
//#include "bootloader/bl_user_entry.h"
//#include "bootloader/bl_peripheral_interface.h"
//#include "bootloader/bl_shutdown_cleanup.h"

////////////////////////////////////////////////////////////////////////////////
// Definitions
////////////////////////////////////////////////////////////////////////////////

//! @brief Bootloader status codes.
//! @ingroup bl_core
enum _bootloader_status
{
    kStatus_UnknownCommand = MAKE_STATUS(kStatusGroup_Bootloader, 0),
    kStatus_SecurityViolation = MAKE_STATUS(kStatusGroup_Bootloader, 1),
    kStatus_AbortDataPhase = MAKE_STATUS(kStatusGroup_Bootloader, 2),
    kStatus_Ping = MAKE_STATUS(kStatusGroup_Bootloader, 3),
    kStatus_NoResponse = MAKE_STATUS(kStatusGroup_Bootloader, 4),
    kStatus_NoResponseExpected = MAKE_STATUS(kStatusGroup_Bootloader, 5)
};

//! @brief Root of the bootloader API tree.
//!
//! An instance of this struct resides in read-only memory in the bootloader. It
//! provides a user application access to APIs exported by the bootloader.
//!
//! @note The order of existing fields must not be changed.
//!
//! @ingroup context
#if 1 // Moved into each SOC based header file in future !!!!!!!!!!!!!
typedef struct BootloaderTree
{
    void (*runBootloader)(void *arg);            //!< Function to start the bootloader executing.
    standard_version_t version;                  //!< Bootloader version number.
    const char *copyright;                       //!< Copyright string.
    const bootloader_context_t *runtimeContext;  //!< Pointer to the bootloader's runtime context.
    const flash_driver_interface_t *flashDriver; //!< Flash driver API.
    const aes_driver_interface_t *aesDriver;     //!< AES driver API.
} bootloader_tree_t;
#endif
////////////////////////////////////////////////////////////////////////////////
// Prototypes
////////////////////////////////////////////////////////////////////////////////

#if defined(__cplusplus)
extern "C" {
#endif

/*!
 * @brief Verify that a given address is ok to jump to.
 *
 * @param applicationAddress The entry point address to validate.
 * @return Boolean indicating whether the address is valid.
 *
 * @ingroup bl_core
 */
bool is_valid_application_location(uint32_t applicationAddress);

#if defined(__cplusplus)
}
#endif

#endif // __BOOTLOADER_H__
