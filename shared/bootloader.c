#include "bootloader.h"

void JumpToBootloader(void) {
    uint32_t runBootloaderAddress;
    void (*runBootloader)(void *arg);

    // Read the function address from the ROM API tree.
    runBootloaderAddress = **(uint32_t **)(0x1c00001c);
    runBootloader = (void (*)(void * arg))runBootloaderAddress;

    runBootloader(NULL);
}
