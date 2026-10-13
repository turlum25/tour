OUTPUT_FORMAT("binary")
ENTRY(_start)

SECTIONS
{
    . = 0x8000;

    /* Define the absolute kernel start boundary symbol for main.c */
    __kernel_start = .;

    .text : { 
        *(.text._start)
        *(.text*) 
    }
    
    .data : { 
        *(.data*) 
        *(.rodata*)
    }
    
    .bss  : {
        __bss_start = .;
        *(.bss*)
        *(COMMON)
        __bss_end = .;
    }

    /* Define the absolute kernel end boundary symbol for main.c */
    __kernel_end = .;
}
