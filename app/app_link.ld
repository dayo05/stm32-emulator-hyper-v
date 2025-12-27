/* app_link.ld */
ENTRY(start_app)

SECTIONS
{
    /* 1. Start at 0x20000 (128KB) */
    . = 0x20000;

    .text : {
        /* 2. Put the entry section FIRST */
        *(.text.entry)
        /* 3. Put all other code (.text) AFTER it */
        *(.text)
    }

    .data : { *(.data) }
    .rdata : { *(.rdata) }
    .bss : { *(.bss) }
}