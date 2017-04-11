#if !defined(__CONTEXT_H__)
#define __CONTEXT_H__

#include "bootloader_common.h"
#include "bootloader/bl_peripheral.h"
#include "memory.h"
#include "packet/command_packet.h"
#include "property.h"
#include "command.h"

#if !defined(BOOTLOADER_HOST)
#include "flash/fsl_flash.h"
#if BL_FEATURE_ENCRYPTION
#include "security/aes_security.h"
#endif // #if BL_FEATURE_ENCRYPTION
#endif // #if !defined(BOOTLOADER_HOST)

//! @addtogroup context
//! @{

////////////////////////////////////////////////////////////////////////////////
// Definitions
////////////////////////////////////////////////////////////////////////////////

#if !defined(BOOTLOADER_HOST)

//! @brief Interface for the flash driver.
typedef struct FlashDriverInterface
{
    standard_version_t version; //!< flash driver API version number.
    status_t (*flash_init)(flash_config_t *config);
    status_t (*flash_erase_all)(flash_config_t *config, uint32_t key);
    status_t (*flash_erase_all_unsecure)(flash_config_t *config, uint32_t key);
    status_t (*flash_erase)(flash_config_t *config, uint32_t start, uint32_t lengthInBytes, uint32_t key);
    status_t (*flash_program)(flash_config_t *config, uint32_t start, uint32_t *src, uint32_t lengthInBytes);
    status_t (*flash_get_security_state)(flash_config_t *config, flash_security_state_t *state);
    status_t (*flash_security_bypass)(flash_config_t *config, const uint8_t *backdoorKey);
    status_t (*flash_verify_erase_all)(flash_config_t *config, flash_margin_value_t margin);
    status_t (*flash_verify_erase)(flash_config_t *config,
                                   uint32_t start,
                                   uint32_t lengthInBytes,
                                   flash_margin_value_t margin);
    status_t (*flash_verify_program)(flash_config_t *config,
                                     uint32_t start,
                                     uint32_t lengthInBytes,
                                     const uint32_t *expectedData,
                                     flash_margin_value_t margin,
                                     uint32_t *failedAddress,
                                     uint32_t *failedData);
    status_t (*flash_get_property)(flash_config_t *config, flash_property_tag_t whichProperty, uint32_t *value);
    status_t (*flash_register_callback)(flash_config_t *config, flash_callback_t callback);
    status_t (*flash_program_once)(flash_config_t *config, uint32_t index, uint32_t *src, uint32_t lengthInBytes);
    status_t (*flash_read_once)(flash_config_t *config, uint32_t index, uint32_t *dst, uint32_t lengthInBytes);
    status_t (*flash_read_resource)(flash_config_t *config,
                                    uint32_t start,
                                    uint32_t *dst,
                                    uint32_t lengthInBytes,
                                    flash_read_resource_option_t option);
    status_t (*flash_prepare_execute_in_ram_functions)(flash_config_t *config);
    status_t (*flash_is_execute_only)(flash_config_t *config,
                                      uint32_t start,
                                      uint32_t lengthInBytes,
                                      flash_execute_only_access_state_t *access_state);
    status_t (*flash_erase_all_execute_only_segments)(flash_config_t *config, uint32_t key);
    status_t (*flash_verify_erase_all_execute_only_segments)(flash_config_t *config, flash_margin_value_t margin);
    status_t (*flash_set_flexram_function)(flash_config_t *config, flash_flexram_function_option_t option);
    status_t (*flash_program_section)(flash_config_t *config, uint32_t start, uint32_t *src, uint32_t lengthInBytes);
} flash_driver_interface_t;

//! @brief Interface for AES 128 functions
typedef struct AesDriverInterface
{
    void (*aes_init)(uint32_t *key);
    void (*aes_encrypt)(uint32_t *in, uint32_t *key, uint32_t *out);
    void (*aes_decrypt)(uint32_t *in, uint32_t *key, uint32_t *out);
} aes_driver_interface_t;

#else // #if !defined(BOOTLOADER_HOST)

// Provide stub definitions for flash driver types for the host.
typedef uint32_t flash_driver_interface_t;
typedef uint32_t flash_config_t;
typedef uint32_t aes_driver_interface_t;

#endif // #if !defined(BOOTLOADER_HOST)

//! @brief Structure of bootloader global context.
typedef struct _bootloaderContext
{
    //! @name API tree
    //@{
    const memory_interface_t *memoryInterface;            //!< Abstract interface to memory operations.
    const memory_map_entry_t *memoryMap;                  //!< Memory map used by abstract memory interface.
    const property_interface_t *propertyInterface;        //!< Interface to property store.
    const command_interface_t *commandInterface;          //!< Interface to command processor operations.
    const flash_driver_interface_t *flashDriverInterface; //!< Flash driver interface.
    const peripheral_descriptor_t *allPeripherals;        //!< Array of all peripherals.
    const aes_driver_interface_t *aesInterface;           //!< Interface to the AES driver
    //@}

    //! @name Runtime state
    //@{
    const peripheral_descriptor_t *activePeripheral; //!< The currently active peripheral.
    flash_config_t flashState;                       //!< Flash driver instance.
    //@}
} bootloader_context_t;

////////////////////////////////////////////////////////////////////////////////
// Externs
////////////////////////////////////////////////////////////////////////////////

extern bootloader_context_t g_bootloaderContext;
extern const flash_driver_interface_t g_flashDriverInterface;
extern const aes_driver_interface_t g_aesInterface;

//! @}

#endif // __CONTEXT_H__
