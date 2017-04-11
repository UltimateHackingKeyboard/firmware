#ifndef __MEMORY_H__
#define __MEMORY_H__

typedef struct _memory_interface {
    status_t (*init)(void);
    status_t (*read)(uint32_t address, uint32_t length, uint8_t *buffer);
    status_t (*write)(uint32_t address, uint32_t length, const uint8_t *buffer);
    status_t (*fill)(uint32_t address, uint32_t length, uint32_t pattern);
    status_t (*flush)(void);
    status_t (*erase)(uint32_t address, uint32_t length);
} memory_interface_t;

typedef struct _memory_region_interface {
    status_t (*init)(void);
    status_t (*read)(uint32_t address, uint32_t length, uint8_t *buffer);
    status_t (*write)(uint32_t address, uint32_t length, const uint8_t *buffer);
    status_t (*fill)(uint32_t address, uint32_t length, uint32_t pattern);
    status_t (*flush)(void);
    status_t (*erase)(uint32_t address, uint32_t length);
} memory_region_interface_t;

typedef struct _memory_map_entry {
    uint32_t startAddress;
    uint32_t endAddress;
    bool isExecutable;
    const memory_region_interface_t *memoryInterface;
} memory_map_entry_t;

#endif
