ENTRY(_start)
SECTIONS {
    . = ALIGN(4096);
    . = 0x10000;
    __kernel_start = .;

    .text : {
        . = ALIGN(16);
        KEEP(*(.text._start))
        KEEP(*(.multiboot))
        *(.text .text.*)
    }

    .rodata : {
        . = ALIGN(16);
        *(.rodata .rodata.*)
    }

    .data : {
        . = ALIGN(16);
        *(.data .data.*)
    }

    .bss : {
        . = ALIGN(16);
        __bss_start = .;
        *(.bss .bss.*)
        *(COMMON)
        __bss_end = .;
    }

    . = ALIGN(16);
    __kernel_end = .;
}
