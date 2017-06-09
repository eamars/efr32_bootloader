/* Linker script for Silicon Labs EFR32MG12P devices                      */
/*                                                                     */
/* This file is subject to the license terms as defined in ARM's       */
/* CMSIS END USER LICENSE AGREEMENT.pdf, governing the use of          */
/* Example Code.                                                       */
/*                                                                     */
/* Copyright 2017 Silicon Laboratories, Inc. http://www.silabs.com     */
/*                                                                     */
/* Version 5.1.3 */
/*                                                                     */


MEMORY
{
	FLASH (rx) : ORIGIN = 0x00000000, LENGTH = 1M
	RAM (rwx)  : ORIGIN = 0x20000000, LENGTH = 256K
}

/* Linker script to place sections and symbol values. Should be used together
 * with other linker script that defines memory regions FLASH and RAM.
 * It references following symbols, which must be defined in code:
 *   Reset_Handler : Entry of reset handler
 *
 * It defines following symbols, which code can use without definition:
 *   __exidx_start
 *   __exidx_end
 *   __copy_table_start__
 *   __copy_table_end__
 *   __zero_table_start__
 *   __zero_table_end__
 *   __etext
 *   __data_start__
 *   __preinit_array_start
 *   __preinit_array_end
 *   __init_array_start
 *   __init_array_end
 *   __fini_array_start
 *   __fini_array_end
 *   __data_end__
 *   __bss_start__
 *   __bss_end__
 *   __end__
 *   end
 *   __HeapLimit
 *   __StackLimit
 *   __StackTop
 *   __stack
 *   __Vectors_End
 *   __Vectors_Size
 */
ENTRY(Reset_Handler)

SECTIONS
{
	/* FLASH segment */
	. = ORIGIN(FLASH);

	.LOADER 0x0 :
	{
		KEEP(*(.tiny_loader*));

		KEEP(*(.loader*));
	} > FLASH

	.VECTORS 0x100 :
	{
		KEEP(*(.vectors));
		KEEP(*(.AAT));
	} > FLASH

	.text :
	{
		__TEXT__begin = .;
		__end__ = .;

		. = ALIGN(4);
		*(.textrw_init .textrw_init*)

		. = ALIGN(4);
		*(.text*)

		. = ALIGN(4);
		KEEP(*(.init))

		. = ALIGN(4);
		KEEP(*(.fini))

		/* .ctors */
		. = ALIGN(4);
		*crtbegin.o(.ctors)
		*crtbegin?.o(.ctors)
		*(EXCLUDE_FILE(*crtend?.o *crtend.o) .ctors)
		*(SORT(.ctors.*))
		*(.ctors)

		/* .dtors */
		. = ALIGN(4);
		*crtbegin.o(.dtors)
		*crtbegin?.o(.dtors)
		*(EXCLUDE_FILE(*crtend?.o *crtend.o) .dtors)
		*(SORT(.dtors.*))
		*(.dtors)

		/* global constants */
		. = ALIGN(4);
		*(.rodata*)

		. = ALIGN(4);
		*(.data_init*)

		. = ALIGN(4);
		KEEP(*(.eh_frame*))
		__TEXT__end = .;
	} > FLASH

	.ARM.extab :
	{
		*(.ARM.extab* .gnu.linkonce.armextab.*)
	} > FLASH

	__exidx_start = .;
	.ARM.exidx :
	{
		*(.ARM.exidx* .gnu.linkonce.armexidx.*)
	} > FLASH
	__exidx_end = .;

	/* End of text */
	__etext = .;

	/* Simulated ROM space */
	.NVM_SIMEE 0xfe000 (NOLOAD) :
	{
	    __SIMEE$$Base = . ;
		__SIMEE__begin = . ;
		KEEP(*(SIMEE .simee));
		__SIMEE__end = . ;
		__SIMEE$$Limit = . ;
	} > FLASH

	/* RAM segment */
	. = ORIGIN(RAM);

	OVERLAY 0x20000000 :
	{
		.EMHEAP
		{
		    EMHEAP$$Base = . ;
			__EMHEAP__begin = . ;
			KEEP(*(EMHEAP_overlay));
			KEEP(*(.emheap*));
			__EMHEAP__end = . ;
			EMHEAP_overlay$$Limit = . ;
		}

		.RESETINFO
		{
		    RESETINFO$$Base = . ;
			__RESETINFO__begin = . ;
			KEEP(*(RESETINFO));
			KEEP(*(.resetinfo*));
			__RESETINFO__end = . ;
			RESETINFO$$Limit = . ;
		}
	} > RAM

	.GUARD_REGION 0x20000f20 (NOLOAD) :
	{
		__GUARD_REGION__begin = . ;
		KEEP(*(GUARD_REGION));
		KEEP(*(.guard_region));
		. += 0x20;
		__GUARD_REGION__end = . ;
	} > RAM

	.CSTACK :
	{
		. = ALIGN(8);
		CSTACK$$Base = . ;
		__CSTACK__begin = . ;
		KEEP(*(CSTACK));
		KEEP(*(.cstack .cstack.*));
		__CSTACK__end = . ;
		CSTACK$$Limit = . ;
	} > RAM

	.data ALIGN(256) : AT (__etext)
	{
		__data_start__ = .;
		KEEP(*(.dynamic_vectors))

		. = ALIGN(4);
		*(.textrw .textrw*);

		. = ALIGN (4);
		*(.data*)

		. = ALIGN (4);
		*(.ram)

		. = ALIGN(4);
		/* preinit data */
		PROVIDE_HIDDEN (__preinit_array_start = .);
		KEEP(*(.preinit_array))
		PROVIDE_HIDDEN (__preinit_array_end = .);

		. = ALIGN(4);
		/* init data */
		PROVIDE_HIDDEN (__init_array_start = .);
		KEEP(*(SORT(.init_array.*)))
		KEEP(*(.init_array))
		PROVIDE_HIDDEN (__init_array_end = .);

		. = ALIGN(4);
		/* finit data */
		PROVIDE_HIDDEN (__fini_array_start = .);
		KEEP(*(SORT(.fini_array.*)))
		KEEP(*(.fini_array))
		PROVIDE_HIDDEN (__fini_array_end = .);

		. = ALIGN(4);
		KEEP(*(.jcr*))

		. = ALIGN(4);
		*(DLIB_PERTHREAD_INITIALIZED_DATA);
		*(DLIB_PERTHREAD_ZERO_DATA);

		/* All data end */
		__data_end__ = .;

	} > RAM

	__rom_end__ = __etext + SIZEOF(.data);

	.bss :
	{
		. = ALIGN(4);
		__bss_start__ = .;
		*(.bss*)
		*(COMMON)
		. = ALIGN(4);
		__bss_end__ = .;

		__end__ = .;
		end = __end__;
		_end = __end__;
	} > RAM

	/* Set stack top to end of RAM, and stack limit move down by
   	* size of stack_dummy section */
	__StackTop = ORIGIN(RAM) + LENGTH(RAM);

	/* Check if FLASH usage exceeds FLASH size */
	ASSERT( (LENGTH(FLASH) - 0x4) > (__etext + SIZEOF(.data) - ORIGIN(FLASH)), "FLASH memory overflowed !")

    /* Stabs debugging sections.  */
    .stab          0 : { *(.stab) }
    .stabstr       0 : { *(.stabstr) }
    .stab.excl     0 : { *(.stab.excl) }
    .stab.exclstr  0 : { *(.stab.exclstr) }
    .stab.index    0 : { *(.stab.index) }
    .stab.indexstr 0 : { *(.stab.indexstr) }
    .comment       0 : { *(.comment) }

    /* DWARF debug sections.
     Symbols in the DWARF debugging sections are relative to the beginning
     of the section so we begin them at 0.  */
    /* DWARF 1 */
    .debug          0 : { *(.debug) }
    .line           0 : { *(.line) }
    /* GNU DWARF 1 extensions */
    .debug_srcinfo  0 : { *(.debug_srcinfo) }
    .debug_sfnames  0 : { *(.debug_sfnames) }
    /* DWARF 1.1 and DWARF 2 */
    .debug_aranges  0 : { *(.debug_aranges) }
    .debug_pubnames 0 : { *(.debug_pubnames) }
    /* DWARF 2 */
    .debug_info     0 : { *(.debug_info .gnu.linkonce.wi.*) }
    .debug_abbrev   0 : { *(.debug_abbrev) }
    .debug_line     0 : { *(.debug_line) }
    .debug_frame    0 : { *(.debug_frame) }
    .debug_str      0 : { *(.debug_str) }
    .debug_loc      0 : { *(.debug_loc) }
    .debug_macinfo  0 : { *(.debug_macinfo) }
    /* SGI/MIPS DWARF 2 extensions */
    .debug_weaknames 0 : { *(.debug_weaknames) }
    .debug_funcnames 0 : { *(.debug_funcnames) }
    .debug_typenames 0 : { *(.debug_typenames) }
    .debug_varnames  0 : { *(.debug_varnames) }
}
