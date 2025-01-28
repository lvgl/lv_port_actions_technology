ENTRY(pattern_main)

#include "att_linker_addr.h"

MEMORY
{
	pattern_ibank : ORIGIN = ATT_PATTERN_ADDR, LENGTH = ATT_PATTERN_SIZE
	flash	      : ORIGIN = 0, LENGTH = ATT_PATTERN_SIZE
}

SECTIONS
{
	/DISCARD/ :
	{
	   *(.comment)
	   *(.eh_frame)
	   *(.eh_frame_hdr)
	   *(.eh_frame_entry)
	   *(.MIPS.abiflags)
	   *(.MIPS.options)
	   *(.options)
	   *(.pdr)
	   *(.reginfo)
	}

	.text :
	{
		__core_c_code_start = .;
		KEEP(*(.entry))
		*(.text*)
		*(.rodata*)
		. = ALIGN(8);
		__core_c_code_end = .;

		__data_ram_start = .;
		*(.data*)
		. = ALIGN(8);
		__data_ram_end = .;
	} >pattern_ibank AT>flash

	.bss :
	{
		__bss_start = .;
		*(.bss*)
		*(COMMON*)
		*(.sbss)
		*(.common)
		*(common)
		. = ALIGN(4);
		__bss_end = .;
	} >pattern_ibank
}

