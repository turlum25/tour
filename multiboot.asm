[bits 32]

section .text._start
global _start
extern kernel_main
extern multiboot_capture_ram

global tInt81_stub
extern tour_handle_int81

extern __bss_start
extern __bss_end

_start:
    cli
    cld
    mov esp, 0x90000
    and esp, 0xFFFFFFF0

    push ebx
    push eax

    mov edi, __bss_start
    mov ecx, __bss_end
    sub ecx, edi
    xor eax, eax
    rep stosb

    call multiboot_capture_ram
    add esp, 8

    call kernel_main


.halt:
    cli
    hlt
    jmp .halt

tInt81_stub: cld
	pushad
	push esp
	call tour_handle_int81
	add esp, 4
	popad
	iretd

section .multiboot
align 4
    MULTIBOOT_MAGIC    equ 0x1BADB002
    MULTIBOOT_FLAGS    equ 0x00000007
    MULTIBOOT_CHECKSUM equ -(MULTIBOOT_MAGIC + MULTIBOOT_FLAGS)

    dd MULTIBOOT_MAGIC
    dd MULTIBOOT_FLAGS
    dd MULTIBOOT_CHECKSUM
    dd 0, 0, 0, 0, 0
    dd 0
    dd 1280
    dd 720
    dd 32
