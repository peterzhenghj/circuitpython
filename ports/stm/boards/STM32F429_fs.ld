/*
    GNU linker script for STM32F429 with filesystem
*/

/* Specify the memory areas */
MEMORY
{
    FLASH (rx)          : ORIGIN = 0x08000000, LENGTH = 1024K /* entire flash */
    FLASH_ISR (rx)      : ORIGIN = 0x08000000, LENGTH = 16K   /* sector 0 */
    FLASH_FS (rx)       : ORIGIN = 0x08004000, LENGTH = 48K   /* sectors 1–3: 3×16K */
    FLASH_FIRMWARE (rx) : ORIGIN = 0x08010000, LENGTH = 960K  /* sector 4: 64K + sectors 5–11: 7×128K */
    RAM (xrw)           : ORIGIN = 0x20000000, LENGTH = 256K  /* SRAM1 + SRAM2 */
}

/* produce a link error if there is not this amount of RAM for these sections */
_minimum_stack_size = 24K;
_minimum_heap_size = 16K;

/* Define the top end of the stack. The stack is full descending so begins just
   above last byte of RAM. Note that EABI requires the stack to be 8-byte aligned. */
_estack = ORIGIN(RAM) + LENGTH(RAM);

/* RAM extents for the garbage collector */
_ram_start = ORIGIN(RAM);
_ram_end = ORIGIN(RAM) + LENGTH(RAM);
