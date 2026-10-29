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
typedef struct __attribute__((packed)) {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t reserved;
    uint8_t attributes;
    uint16_t offset_high;
} control_idt_gate_t;

typedef struct __attribute__((packed)) {
    uint16_t limit;
    uint32_t base;
} control_idtr_t;

static control_idt_gate_t control_idt[256];
void panic(const char *reason, int retnum);

static void control_set_idt_gate(uint8_t vector, void (*handler)(void), uint16_t selector) {
    uintptr_t address = (uintptr_t)handler;
    control_idt[vector].offset_low = (uint16_t)address;
    control_idt[vector].selector = selector;
    control_idt[vector].reserved = 0;
    control_idt[vector].attributes = 0x8E;
    control_idt[vector].offset_high = (uint16_t)(address >> 16);
}

static void control_exception_panic(uint32_t vector, uint32_t error_code) {
    printcol("[fault] CPU exception ", VGA_COLOR_RED);
    printcolvar(vector, 1, VGA_COLOR_RED);
    printcol(" error code ", VGA_COLOR_RED);
    printcolvar(error_code, 1, VGA_COLOR_RED);
    printcol("\n", VGA_COLOR_RED);
    panic("Unhandled CPU exception", (int)vector);
}

__asm__(
    ".text\n"
    ".global control_isr_common\n"
    "control_isr_common:\n"
    "cld\n"
    "pushal\n"
    "movl 32(%esp), %eax\n"
    "movl 36(%esp), %edx\n"
    "pushl %edx\n"
    "pushl %eax\n"
    "call control_exception_panic\n"
    "1: cli\n"
    "hlt\n"
    "jmp 1b\n"
);

#define CONTROL_ISR_NO_ERROR(vector) \
    __asm__(".global control_isr_" #vector "\n" \
            "control_isr_" #vector ":\n" \
            "pushl $0\n" \
            "pushl $" #vector "\n" \
            "jmp control_isr_common\n")

#define CONTROL_ISR_ERROR(vector) \
    __asm__(".global control_isr_" #vector "\n" \
            "control_isr_" #vector ":\n" \
            "pushl $" #vector "\n" \
            "jmp control_isr_common\n")

CONTROL_ISR_NO_ERROR(0);
CONTROL_ISR_NO_ERROR(1);
CONTROL_ISR_NO_ERROR(2);
CONTROL_ISR_NO_ERROR(3);
CONTROL_ISR_NO_ERROR(4);
CONTROL_ISR_NO_ERROR(5);
CONTROL_ISR_NO_ERROR(6);
CONTROL_ISR_NO_ERROR(7);
CONTROL_ISR_ERROR(8);
CONTROL_ISR_NO_ERROR(9);
CONTROL_ISR_ERROR(10);
CONTROL_ISR_ERROR(11);
CONTROL_ISR_ERROR(12);
CONTROL_ISR_ERROR(13);
CONTROL_ISR_ERROR(14);
CONTROL_ISR_NO_ERROR(15);
CONTROL_ISR_NO_ERROR(16);
CONTROL_ISR_ERROR(17);
CONTROL_ISR_NO_ERROR(18);
CONTROL_ISR_NO_ERROR(19);
CONTROL_ISR_NO_ERROR(20);
CONTROL_ISR_ERROR(21);
CONTROL_ISR_NO_ERROR(22);
CONTROL_ISR_NO_ERROR(23);
CONTROL_ISR_NO_ERROR(24);
CONTROL_ISR_NO_ERROR(25);
CONTROL_ISR_NO_ERROR(26);
CONTROL_ISR_NO_ERROR(27);
CONTROL_ISR_NO_ERROR(28);
CONTROL_ISR_ERROR(29);
CONTROL_ISR_ERROR(30);
CONTROL_ISR_NO_ERROR(31);

__asm__(
    ".global control_isr_default\n"
    "control_isr_default:\n"
    "pushl $0\n"
    "pushl $255\n"
    "jmp control_isr_common\n"
);

#define CONTROL_DECLARE_ISR(vector) extern void control_isr_##vector(void)
CONTROL_DECLARE_ISR(0);
CONTROL_DECLARE_ISR(1);
CONTROL_DECLARE_ISR(2);
CONTROL_DECLARE_ISR(3);
CONTROL_DECLARE_ISR(4);
CONTROL_DECLARE_ISR(5);
CONTROL_DECLARE_ISR(6);
CONTROL_DECLARE_ISR(7);
CONTROL_DECLARE_ISR(8);
CONTROL_DECLARE_ISR(9);
CONTROL_DECLARE_ISR(10);
CONTROL_DECLARE_ISR(11);
CONTROL_DECLARE_ISR(12);
CONTROL_DECLARE_ISR(13);
CONTROL_DECLARE_ISR(14);
CONTROL_DECLARE_ISR(15);
CONTROL_DECLARE_ISR(16);
CONTROL_DECLARE_ISR(17);
CONTROL_DECLARE_ISR(18);
CONTROL_DECLARE_ISR(19);
CONTROL_DECLARE_ISR(20);
CONTROL_DECLARE_ISR(21);
CONTROL_DECLARE_ISR(22);
CONTROL_DECLARE_ISR(23);
CONTROL_DECLARE_ISR(24);
CONTROL_DECLARE_ISR(25);
CONTROL_DECLARE_ISR(26);
CONTROL_DECLARE_ISR(27);
CONTROL_DECLARE_ISR(28);
CONTROL_DECLARE_ISR(29);
CONTROL_DECLARE_ISR(30);
CONTROL_DECLARE_ISR(31);
extern void control_isr_default(void);

static void (*const control_exception_isrs[32])(void) = {
    control_isr_0, control_isr_1, control_isr_2, control_isr_3,
    control_isr_4, control_isr_5, control_isr_6, control_isr_7,
    control_isr_8, control_isr_9, control_isr_10, control_isr_11,
    control_isr_12, control_isr_13, control_isr_14, control_isr_15,
    control_isr_16, control_isr_17, control_isr_18, control_isr_19,
    control_isr_20, control_isr_21, control_isr_22, control_isr_23,
    control_isr_24, control_isr_25, control_isr_26, control_isr_27,
    control_isr_28, control_isr_29, control_isr_30, control_isr_31
};

static inline void autopanic(void) {
    uint16_t code_segment;
    __asm__ volatile ("mov %%cs, %0" : "=r"(code_segment));

    for (unsigned vector = 0; vector < 256; ++vector) {
        control_set_idt_gate((uint8_t)vector, control_isr_default, code_segment);
    }
    for (unsigned vector = 0; vector < 32; ++vector) {
        control_set_idt_gate((uint8_t)vector, control_exception_isrs[vector], code_segment);
    }

    control_idtr_t idtr = {
        .limit = (uint16_t)(sizeof(control_idt) - 1),
        .base = (uint32_t)(uintptr_t)control_idt
    };
    __asm__ volatile ("lidt %0" : : "m"(idtr));
}

#undef CONTROL_DECLARE_ISR
#undef CONTROL_ISR_ERROR
#undef CONTROL_ISR_NO_ERROR

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
    while (1) {
        __asm__ volatile ("hlt");
    }
}

#endif /*CONTROL_H*/
