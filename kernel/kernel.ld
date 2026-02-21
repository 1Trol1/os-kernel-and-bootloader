ENTRY(_start)

SECTIONS
{
    /* Kernel będzie ładowany przez EFI gdziekolwiek,
       ale my ustawiamy adres linkowania na 1 MB */
    . = 0x100000;

    .text : {
        *(.text*)
    }

    .rodata : {
        *(.rodata*)
    }

    .data : {
        *(.data*)
    }

    .bss : {
        *(.bss*)
        *(COMMON)
    }
}

