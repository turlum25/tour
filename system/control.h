#ifndef CONTROL_H

#define CONTROL_H

// RE: 

// I purposefully put one line gaps because it looks like a blob on VS Code.

// Thank you

// system/control.h

// 10/26/2026

// This code is part of TOUR.

// You can distribute it, do whatever you want.

// I don't give two damns if you hardbrick your computer with it.

// Use QEMU for testing stuff, then comes computer

// You can use this by itself. BUT copy kernel.h , OR

// Make the contents of /vga.c into a header.

#define BINEXEC_ADDR 0x100000

#define BINEXEC_MAX_SIZE 0x10000

#include <stdint.h>

#include <stddef.h>

#include "../kernel.h"

#include "time.h"

#include <string.h>

int reboot(void) {

       __asm__ volatile ("cli");

       struct __attribute__((packed)) {

           uint16_t limit;

           uint32_t base;

       } zero_idt = {0, 0};

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

static void control_set_idt_gate(

       uint8_t vector,

       void (*handler)(void),

       uint16_t selector

) {

       uintptr_t address = (uintptr_t)handler;

       control_idt[vector].offset_low = (uint16_t)address;

       control_idt[vector].selector = selector;

       control_idt[vector].reserved = 0;

       control_idt[vector].attributes = 0x8E;

       control_idt[vector].offset_high = (uint16_t)(address >> 16);

}

#define CONTROL_ISR_NO_ERROR(vector) \
       static __attribute__((naked, used)) \
       void control_isr_##vector(void) \
       { \
           __asm__ volatile ( \
               "pushl $0\n\t" \
               "pushl $" #vector "\n\t" \
               "jmp control_isr_common\n\t" \
           ); \
       }

#define CONTROL_ISR_ERROR(vector) \
       static __attribute__((naked, used)) \
       void control_isr_##vector(void) \
       { \
           __asm__ volatile ( \
               "pushl $" #vector "\n\t" \
               "jmp control_isr_common\n\t" \
           ); \
       }

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

#undef CONTROL_ISR_ERROR

#undef CONTROL_ISR_NO_ERROR

static __attribute__((naked, used))

void control_isr_default(void)

{

       __asm__ volatile (

           "pushl $0\n\t"

           "pushl $255\n\t"

           "jmp control_isr_common\n\t"

       );

}

static void (*const control_exception_isrs[32])(void) = {

       control_isr_0,  control_isr_1,

       control_isr_2,  control_isr_3,

       control_isr_4,  control_isr_5,

       control_isr_6,  control_isr_7,

       control_isr_8,  control_isr_9,

       control_isr_10, control_isr_11,

       control_isr_12, control_isr_13,

       control_isr_14, control_isr_15,

       control_isr_16, control_isr_17,

       control_isr_18, control_isr_19,

       control_isr_20, control_isr_21,

       control_isr_22, control_isr_23,

       control_isr_24, control_isr_25,

       control_isr_26, control_isr_27,

       control_isr_28, control_isr_29,

       control_isr_30, control_isr_31

};

static inline void autopanic(void)

{

       uint16_t code_segment;

       __asm__ volatile (

           "mov %%cs, %0"

           : "=r"(code_segment)

       );

       for (unsigned vector = 0; vector < 256; ++vector) {

control_set_idt_gate(

               (uint8_t)vector,

               control_isr_default,

               code_segment

           );

       }

       for (unsigned vector = 0; vector < 32; ++vector) {

control_set_idt_gate(

               (uint8_t)vector,

               control_exception_isrs[vector],

               code_segment

           );

       }

control_idtr_t idtr = {

           .limit = (uint16_t)(sizeof(control_idt) - 1),

           .base = (uint32_t)(uintptr_t)control_idt

       };

       __asm__ volatile (

           "lidt %0"

           :

           : "m"(idtr)

           : "memory"

       );

}

typedef struct __attribute__((packed)) {

       uint32_t edi, esi, ebp, esp;

       uint32_t ebx, edx, ecx, eax;

       uint32_t vector;

       uint32_t error_code;

       uint32_t eip;

       uint32_t cs;

       uint32_t eflags;

} interrupt_frame_t;

typedef interrupt_frame_t register_stack_t;

static interrupt_frame_t *fault_frame = NULL;

static void print_register_hex(uint32_t value)

{

       static const char digits[] = "0123456789ABCDEF";

       char text[9];

       for (unsigned i = 0; i < 8; ++i) {

           unsigned shift = 28 - (i * 4);

           text[i] = digits[(value >> shift) & 0xF];

       }

       text[8] = '\0';

printcol(text, VGA_COLOR_WHITE);

}

static void DumpRegisters(void)

{

       uint32_t eax, ebx, ecx, edx, esp, ebp, esi, edi;

       uint32_t eip = 0, eflags = 0;

       bool has_fault = (fault_frame != NULL);

       if (has_fault) {

           eax = fault_frame->eax;

           ebx = fault_frame->ebx;

           ecx = fault_frame->ecx;

           edx = fault_frame->edx;

           esp = fault_frame->esp;

           ebp = fault_frame->ebp;

           esi = fault_frame->esi;

           edi = fault_frame->edi;

           eip = fault_frame->eip;

           eflags = fault_frame->eflags;

       } else {

           __asm__ volatile (

               "movl %%edi, %0\n\t"

               "movl %%esi, %1\n\t"

               "movl %%ebp, %2\n\t"

               "movl %%esp, %3\n\t"

               "movl %%ebx, %4\n\t"

               "movl %%edx, %5\n\t"

               "movl %%ecx, %6\n\t"

               "movl %%eax, %7\n\t"

               : "=m"(edi), "=m"(esi), "=m"(ebp), "=m"(esp),

                 "=m"(ebx), "=m"(edx), "=m"(ecx), "=m"(eax)

           );

       }

printcol("\nEAX: ", VGA_COLOR_WHITE);

print_register_hex(eax);

printcol("  EBX: ", VGA_COLOR_WHITE);

print_register_hex(ebx);

printcol("  ECX: ", VGA_COLOR_WHITE);

print_register_hex(ecx);

printcol("  EDX: ", VGA_COLOR_WHITE);

print_register_hex(edx);

printcol("\nESP: ", VGA_COLOR_WHITE);

print_register_hex(esp);

printcol("  EBP: ", VGA_COLOR_WHITE);

print_register_hex(ebp);

printcol("  ESI: ", VGA_COLOR_WHITE);

print_register_hex(esi);

printcol("  EDI: ", VGA_COLOR_WHITE);

print_register_hex(edi);

       if (has_fault) {

printcol("\nEIP: ", VGA_COLOR_YELLOW);

print_register_hex(eip);

printcol("  EFLAGS: ", VGA_COLOR_WHITE);

print_register_hex(eflags);

       }

printcol("\n", VGA_COLOR_WHITE);

}

static void control_exception_panic(interrupt_frame_t *frame)

{

       fault_frame = frame;

printcol("[fault] CPU exception ", VGA_COLOR_RED);

printcolvar(frame->vector, 1, VGA_COLOR_RED);

printcol(" error code ", VGA_COLOR_RED);

printcolvar(frame->error_code, 1, VGA_COLOR_RED);

printcol("\n", VGA_COLOR_RED);

panic("Unhandled CPU exception", (int)frame->vector);

}

static __attribute__((naked, used))

void control_isr_common(void)

{

       __asm__ volatile (

           "cld\n\t"

           "pushal\n\t"

           "pushl %esp\n\t"

           "call control_exception_panic\n\t"

           "1: cli\n\t"

           "hlt\n\t"

           "jmp 1b\n\t"

       );

}

// End of vibecoded garbage

void panic(const char *reason, int retnum)

{

       uint32_t code = (uint32_t)retnum;

printcol("[panic] ", VGA_COLOR_RED);

printcol(reason, VGA_COLOR_RED);

printcol(" , return code ", VGA_COLOR_RED);

printcolvar(code, 1, VGA_COLOR_RED);

sleep(6);

printcol("\n[panic] [log] Dumping registers\n", VGA_COLOR_RED);

DumpRegisters();

printcol("\n[panic] Halting computer.\n", VGA_COLOR_RED);

       __asm__ volatile ("cli");

       while (1) {

           __asm__ volatile ("hlt");

       }

}

int tour_handle_int81(register_stack_t *regs)
{
    uint8_t syscall = (regs->eax >> 8) & 0xFF;

    switch (syscall) {
        case 0x01:
            printcol((const char *)regs->edx, VGA_COLOR_WHITE);
            break;

        default:
            return -1;
    }

    return 0;
}

int handle_tour_int80(void)

{

       // TODO: TOUR int 0x80 handler.

       return 0;

}

void *binexec(uint8_t *binary, uint32_t size)

{

       if (binary == NULL || size == 0)

           return NULL;

       if (size > BINEXEC_MAX_SIZE)

           return NULL;

       uint8_t *dest = (uint8_t *)BINEXEC_ADDR;

       // Don't know what this means but OK, reddit*

memcpy(dest, binary, size);

       void (*entry)(void) = (void (*)(void))dest;

entry();

       return dest;

}

#endif /* CONTROL_H */