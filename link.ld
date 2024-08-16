OUTPUT_FORMAT(elf64-x86-64)
ENTRY(_start)
PHDRS
{
    exec PT_LOAD FLAGS(5);
    ro PT_LOAD FLAGS(4);
    rw PT_LOAD FLAGS(6);
}
SECTIONS
{
    . = 0xFFFFFFFF80000000;

    .text ALIGN(0x1000): { *(.text*) } :exec
    .rodata ALIGN(0x1000): { *(.rodata*) } :ro
    .data ALIGN(0x1000): { *(.data) } :rw
    .bss : { *(COMMON) *(.bss) }

    /DISCARD/ :
    {
        *(.comment)
        *(.eh_frame)
        *(.note*)
    }
}
