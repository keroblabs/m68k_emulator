/*
 * The following values should NOT be modified. PC relative applications
 * have no fixed base address - they may be loaded and executed anywhere in
 * memory. For simplicity sake we consider it to start from address 0.
 *
 * Relative operations are limited to a range of +/- 32K on the 68000 CPU
 * due to offsets being specified using a signed 16 bit value. This places
 * an effective size limitation of 32K on the total application size for all
 * code and data (a reference at the start of the binary can only look 32K
 * forwards, and a reference at the end of the binary can only look 32K
 * backwards).
 */
_bin_base = 0;
_bin_sz = 256k;

_mmu_base = (_bin_base + _bin_sz);
_mmu_sz   = 192k;

_io_base  = (_mmu_base + _mmu_sz);
_io_sz    = 64k;

_bss_base = (_io_base + _io_sz);
_bss_sz   = 512k;

/*STARTUP(../boot/irq_boot.o)*/
OUTPUT_ARCH(m68k)

__code_start = _bin_base;
__code_sz = _bin_sz;

__mmu_start = _mmu_base;
__mmu_sz    = _mmu_sz;

__io_start  = _io_base;
__io_sz     = _io_sz;

__sram_start = _bss_base;
__sram_sz    = _bss_sz;

_Min_Heap_Size = 8192; /* 2 KB */
_Min_Stack_Size = 8192; /* 4 KB */

MEMORY {
    code (rx)  : ORIGIN = __code_start, LENGTH = __code_sz
    mmu  (rwx) : ORIGIN = __mmu_start,  LENGTH = __mmu_sz
    io   (rwx) : ORIGIN = __io_start,   LENGTH = __io_sz
    sram (rwx) : ORIGIN = __sram_start, LENGTH = __sram_sz
}

SECTIONS {
	.vectors :
	{
		. = ALIGN(4);
		KEEP(*(.vectors)) /* Startup code */
		. = ALIGN(4);
	} > code

	.handlers_asm :
	{
		. = ALIGN(4);
		KEEP(*(.handlers_asm)) /* Startup code */
		. = ALIGN(4);
	} > code

	.handlers_c :
	{
		. = ALIGN(4);
		KEEP(*(.handlers_c)) /* Startup code */
		. = ALIGN(4);
	} > code

    .text : {
        _text_start = .;
        KEEP(*(.text .text.* .fini .init .eh_frame))
        . = ALIGN(4);
        _text_end = .;
    } > code

    .data : AT(_text_end) {
        _data_start = .;
        *(.rodata .rodata.*)
        *(.data .data.*)
        . = ALIGN(4);
        _data_end = .;
    } > code

    .mmu : {
		. = ALIGN(4);
        _mmu_start = .;
		KEEP(*(.mmu)) /* app code */
		. = ALIGN(4);
        _mmu_end = .;
    } > mmu

    .io : {
		. = ALIGN(4);
        _io_start = .;
		KEEP(*(.io)) /* app code */
		. = ALIGN(4);
        _io_end = .;
    } > io

    .bss : {
        _bss_start = .;
        *(.bss .bss.*)
        *(COMMON)
        . = ALIGN(4);
        _bss_end = .;
    } > sram

    /* Heap region, starts after the BSS section */
    . = ALIGN(4);
    _heap_start = .;

    .heap (NOLOAD):
    {
        . = . + _Min_Heap_Size;
    } > sram

    . = ALIGN(4);
    _heap_end = .;

    /* Define the heap size as a symbol */
    _heap_size = _heap_end - _heap_start;
    
    /* Stack region, starts at the end of RAM and grows downwards */
    _estack = ORIGIN(sram) + LENGTH(sram);

    .stack (NOLOAD):
    {
        /* . = . - _Min_Stack_Size; */
        _stack_start = .;
    } > sram

    /* Ensure stack and heap don't collide. The linker will issue an error if they overlap. */
    /* ASSERT( _heap_end <= _stack_start, "Stack and heap overlap!" ); */
}