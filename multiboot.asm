[bits 32]

section .text._start
global _start
extern kernel_main
extern multiboot_capture_ram

_start:
    cli
    cld
    mov esp, 0x90000
    and esp, 0xFFFFFFF0

    push ebx
    push eax
    call multiboot_capture_ram
    add esp, 8

    call kernel_main

.halt:
    cli
    hlt
    jmp .halt

section .multiboot
align 4
    dd 0x1BADB002
    dd 0x00000003
    dd -(0x1BADB002 + 0x00000003)
