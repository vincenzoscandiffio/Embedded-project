ENTRY(_start)

MEMORY
{
    /* Programs are linked at address 0 and must fit within the 3072 bytes reserved for code, data, and BSS */
    PROG_RAM (rwx) : ORIGIN = 0x00000000, LENGTH = 3072
}

SECTIONS
{
    .text :
    {
        *(.text*)
        *(.rodata*)
    } > PROG_RAM

    .data :
    {
        *(.data*)
    } > PROG_RAM

    .bss :
    {
        *(.bss*)
        *(COMMON)
    } > PROG_RAM

    /DISCARD/ :
    {
        *(.ARM.exidx*)
        *(.ARM.extab*)
        *(.eh_frame*)
        *(.comment)
        *(.note*)
    }
}
