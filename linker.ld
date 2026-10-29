OUTPUT_FORMAT(binary)
ENTRY(_start)
SECTIONS
{
    . = 0x1000;
    .text   : { *(.text*) }
    .rodata : { *(.rodata*) }
    .data   : { *(.data*) }
    .init_array : {
        __init_array_start = .;
        *(.init_array*) *(.ctors*)
        __init_array_end = .;
    }
    .bss    : {
        __bss_start = .;
        *(.bss*) *(COMMON)
        __bss_end = .;
    }
    /DISCARD/ : { *(.note*) *(.comment) *(.eh_frame*) }
}
