/* link.ld */
OUTPUT_FORMAT("pei-i386")

SECTIONS
{
    /* Set load address to 0x10000 */
    . = 0x10000;

    .text : {
        /* 1. The Entry Point must be first */
        build/kernel_entry.o(.text)

        /* 2. Interrupt wrappers */
        build/interrupts.o(.text)

        /* 3. All other code */
        *(.text)

        /* 4. CONSTANTS & STRINGS (Crucial!) */
        *(.rdata)
        *(.rdata*)
        *(.rodata)
    }

    .data : {
        *(.data)
    }

    .bss : {
        *(.bss)
    }
}