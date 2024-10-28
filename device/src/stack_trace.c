#include "stack_trace.h"
#include "shared/attributes.h"
#include "shared/atomicity.h"
#include <zephyr/kernel.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/logging/log.h>
#include <zephyr/linker/linker-defs.h>

// With this config, zephyr defines its own handler, leading to multiple definition error
#ifndef CONFIG_RESET_ON_FATAL_ERROR

#define MAX_STACK_DEPTH 32
#define RAW_STACK_DUMP_SIZE 64

static bool is_valid_code_address(uint32_t addr) {
    return (addr >= (uint32_t)__rom_region_start && addr < (uint32_t)__rom_region_end);
}

static bool is_valid_ram_address(uint32_t addr) {
    extern char _image_ram_start[];
    extern char _image_ram_end[];
    return (addr >= (uint32_t)_image_ram_start && addr < (uint32_t)_image_ram_end);
}

void print_stack_trace(const z_arch_esf_t *esf) {
    uint32_t *sp = (uint32_t *)esf;
    uint32_t lr = esf->basic.lr;
    uint32_t pc = esf->basic.pc;

    printk("Stack trace:\n");
    printk("  0x%08x (PC)\n", pc);

    if (is_valid_code_address(lr & ~1)) {
        printk("  0x%08x (LR)\n", lr & ~1);
    }

    // Walk up the stack
    for (int i = 0; i < MAX_STACK_DEPTH && is_valid_ram_address((uint32_t)sp); i++) {
        uint32_t value = *sp;
        if (is_valid_code_address(value & ~1)) {
            printk("  0x%08x\n", value & ~1);
        }
        sp++;
    }
    printk("To translate to code, you can use:\n    ./build.sh right addrline 0x1234\n   arm-none-eabi-addr2line -e device/build/$device/zephyr/zephyr.elf $ADDR\n");
}

void k_sys_fatal_error_handler(unsigned int reason, const z_arch_esf_t *esf)
{
    // Disable interrupts to prevent re-entrance
    ATTR_UNUSED DISABLE_IRQ();

    printk("*** Kernel Fatal Error %d ***\n", reason);
    printk("Current thread: %p\n", k_current_get());

    print_stack_trace(esf);

    // Halt the system
    printk("System halted\n");
    while (1) {
        k_cpu_idle();
    }
}

#endif
