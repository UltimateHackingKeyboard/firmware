#include "wormhole.h"

wormhole_t *Wormhole NO_INIT_GCC;

void* getSP(void)
{
    void *sp;
    __asm__ __volatile__ ("mov %0, sp" : "=r"(sp));
//    __asm__ __volatile__ ("mrs %0, msp" : "=r"(sp));
    return sp;
}

void __attribute__ ((__section__ (".init3"), __naked__)) move_sp(void) {
    void* SP = getSP();
    SP -= sizeof(wormhole_t);
    Wormhole = (wormhole_t*)(SP + 1);
}
