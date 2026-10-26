// system/control.h
// 10/26/2026
// This code is part of TOUR.
// You can distribute it, do whatever you want.
// I don't give two damns if you hardbrick your computer with it.
// Use QEMU for testing stuff, then comes computer

// You can use this by itself. BUT copy kernel.h , OR
// Make the contents of /vga.c into a header.
#ifndef CONTROL_H
#define CONTROL_H

#include <stdint.h>
#include <stddef.h> // Just in case, added these.
#include "../kernel.h"
#include "time.h"

int reboot() { 
    volatile uint16_t zero_idt[3] = {0, 0, 0};
    __asm__ volatile ("lidt %0" : : "m"(zero_idt));
    __asm__ volatile ("int $3");
    while (1) {
        __asm__ volatile ("hlt");
    }
}


// This, that, and DumpRegister
// vibecoded. utter garbage
// But meh rest coded myself at least

typedef struct {
    uint32_t edi, esi, ebp, esp;
    uint32_t ebx, edx, ecx, eax;
} register_stack_t;

static void print_register_hex(uint32_t value) {
    static const char digits[] = "0123456789ABCDEF";
    char text[9];

    for (unsigned i = 0; i < 8; ++i) {
        unsigned shift = 28 - (i * 4);
        text[i] = digits[(value >> shift) & 0xF];
    }
    text[8] = '\0';

    printcol(text, VGA_COLOR_WHITE);
}

static void DumpRegisters(void) {
    register_stack_t regs;

    __asm__ volatile (
        "movl %%edi, %0\n\t"
        "movl %%esi, %1\n\t"
        "movl %%ebp, %2\n\t"
        "movl %%esp, %3\n\t"
        "movl %%ebx, %4\n\t"
        "movl %%edx, %5\n\t"
        "movl %%ecx, %6\n\t"
        "movl %%eax, %7\n\t"
        : "=m"(regs.edi), "=m"(regs.esi),
          "=m"(regs.ebp), "=m"(regs.esp),
          "=m"(regs.ebx), "=m"(regs.edx),
          "=m"(regs.ecx), "=m"(regs.eax)
    );
    printcol("\n", VGA_COLOR_WHITE);
    printcol("EAX: ", VGA_COLOR_WHITE); print_register_hex(regs.eax);
    printcol("  EBX: ", VGA_COLOR_WHITE); print_register_hex(regs.ebx);
    printcol("  ECX: ", VGA_COLOR_WHITE); print_register_hex(regs.ecx);
    printcol("  EDX: ", VGA_COLOR_WHITE); print_register_hex(regs.edx);
    printcol("\nESP: ", VGA_COLOR_WHITE); print_register_hex(regs.esp);
    printcol("  EBP: ", VGA_COLOR_WHITE); print_register_hex(regs.ebp);
    printcol("  ESI: ", VGA_COLOR_WHITE); print_register_hex(regs.esi);
    printcol("  EDI: ", VGA_COLOR_WHITE); print_register_hex(regs.edi);
    printcol("\n", VGA_COLOR_WHITE);
}


// End of vibecoded garbage

void panic(const char *reason, int retnum) {
    printcol("[panic] ", VGA_COLOR_RED);
    printcol(reason, VGA_COLOR_RED);
    printcol(" , return code ", VGA_COLOR_RED);
    printcolvar((uint32_t)retnum, 1, VGA_COLOR_RED);

    sleep(6);
    printcol("\n[panic] [log] Dumping registers\n", VGA_COLOR_RED);
    DumpRegisters();
    printcol("\n[panic] Halting computer.\n", VGA_COLOR_RED);
    
    __asm__ volatile ("cli");
    for (;;) {
        __asm__ volatile ("hlt");
    }
}

#endif /*CONTROL_H*/