#ifndef MEM_H
#define MEM_H

#include "kernel.h"

extern uint64_t TotalRAM;
extern uint64_t TakenRAM; // We're not gonna use you because yes
                          // Re: Nevermind.

static inline void checkprintram(void) {

    // Bootloader kept RAM amount, taken amount riiight here.
    // Let's take it! Colonise like England. Hehe! But we're just reading it apparently
    // OKay i admit it its vibecoded i give up
    volatile uint16_t *const boot_mem_ptr = (volatile uint16_t *)0x7E00;
    TotalRAM = (uint64_t)(*boot_mem_ptr);

    extern unsigned char __kernel_start[];
    extern unsigned char __kernel_end[];
    
    uintptr_t kernel_start = (uintptr_t)__kernel_start;
    uintptr_t kernel_end   = (uintptr_t)__kernel_end;

    if (kernel_end >= kernel_start) {
        uint64_t kernel_bytes = (uint64_t)(kernel_end - kernel_start);
        TakenRAM = (kernel_bytes + 1023) / 1024;
    } else {
        TakenRAM = 1; 
    }
}

#endif /* MEM_H */
