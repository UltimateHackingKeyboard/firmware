#ifndef __ATTRIBUTES_H__
#define __ATTRIBUTES_H__

// Macros:

#define ATTR_WEAK __attribute__((weak))
#define ATTR_UNUSED __attribute__((unused))
#define ATTR_PACKED __attribute__((packed))
#define ATTR_ALIGNED __attribute__((aligned))
#define ATTR_NO_INIT __attribute__((section(".noinit")))
#define ATTR_BOOTLOADER_CONFIG __attribute__((used, section(".BootloaderConfig")))

#ifdef __ZEPHYR__
    #define ATTR_DATA2
#else
    #define ATTR_DATA2 __attribute__((section(".m_data_2")))
    #define ATTR_DATA_NO_INIT __attribute__((section(".m_data_noinit")))
#endif

#endif
