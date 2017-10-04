#ifndef __ATTRIBUTES_H__
#define __ATTRIBUTES_H__

// Macros:

    #define ATTR_PACKED __attribute__ ((packed))
    #define ATTR_NO_INIT __attribute__ ((section (".noinit")))
    #define ATTR_DATA2 __attribute__((section (".m_data_2")))
    #define ATTR_BOOTLOADER_CONFIG __attribute__((used, section(".BootloaderConfig")))

#endif
