OUTPUT_ARCH(riscv)
ENTRY(_start)

SECTIONS
{
    . = 0x1000000;
    .text : {
        *(.text*)
    }
    . = ALIGN(4);
    .data : {
        *(.data*)
    }
    . = ALIGN(4);
    .bss : {
        *(.bss*)
    }
}
