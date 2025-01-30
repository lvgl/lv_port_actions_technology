/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Linker command/script file
 *
 * Linker script for the Cortex-M platforms.
 */

#include <autoconf.h>
#include <linker/sections.h>
#include <devicetree.h>

#include <linker/linker-defs.h>
#include <linker/linker-tool.h>

#ifdef CONFIG_SECTION_OVERLAY
#include <section_overlay.h>
#endif

/* physical address of RAM */
#define ROMABLE_REGION FLASH
#define RAMABLE_REGION PSRAM

#define ROM_ADDR (CONFIG_FLASH_BASE_ADDRESS + CONFIG_FLASH_LOAD_OFFSET)
#define ROM_SIZE (CONFIG_FLASH_SIZE*1K - CONFIG_FLASH_LOAD_OFFSET)

#define RAM_SIZE (CONFIG_SRAM_SIZE * 1K)
#define RAM_ADDR CONFIG_SRAM_BASE_ADDRESS

/* Set alignment to CONFIG_ARM_MPU_REGION_MIN_ALIGN_AND_SIZE
 * to make linker section alignment comply with MPU granularity.
 */
#if defined(CONFIG_ARM_MPU_REGION_MIN_ALIGN_AND_SIZE)
_region_min_align = CONFIG_ARM_MPU_REGION_MIN_ALIGN_AND_SIZE;
#else
/* If building without MPU support, use default 4-byte alignment. */
_region_min_align = 4;
#endif

#define MPU_ALIGN(region_size) . = ALIGN(_region_min_align)

MEMORY
{
	FLASH                 (rx) : ORIGIN = ROM_ADDR,                                 LENGTH = ROM_SIZE
	SRAM                  (wx) : ORIGIN = CONFIG_SRAM_BASE_ADDRESS,                 LENGTH = RAM_SIZE
	PSRAM                 (wx) : ORIGIN = CONFIG_PSRAM_BASE_ADDRESS,                LENGTH = CONFIG_PSRAM_SIZE * 1K
	SHARE_RAM             (wx) : ORIGIN = 0x2FF1AE00,                               LENGTH = 0x51FF
	DSP_SRAM              (wx) : ORIGIN = 0x30044000,                               LENGTH = 0x14000

    /* Used by and documented in include/linker/intlist.ld */
	IDT_LIST  (wx)      : ORIGIN = (RAM_ADDR + RAM_SIZE), LENGTH = 2K
}

ENTRY(CONFIG_KERNEL_ENTRY)

SECTIONS
{

#include <linker/rel-sections.ld>

	/*
	* .plt and .iplt are here according to 'arm-zephyr-elf-ld --verbose',
	* before text section.
	*/
	/DISCARD/ :
	{
		*(.plt)
	}

	/DISCARD/ :
	{
		*(.iplt)
	}

	GROUP_START(ROMABLE_REGION)

	__rom_region_start = ROM_ADDR;

	SECTION_PROLOGUE(rom_start,,)
	{

/* Located in generated directory. This file is populated by calling
 * zephyr_linker_sources(ROM_START ...). This typically contains the vector
 * table and debug information.
 */
#include <snippets-rom-start.ld>

	} GROUP_LINK_IN(ROMABLE_REGION)

    SECTION_PROLOGUE(img_head,(ROM_ADDR+0x200),) AT (ROM_ADDR+0x200)
    {
         KEEP(*(.img_head*))
    } GROUP_LINK_IN(ROMABLE_REGION)

	SECTION_PROLOGUE(_ACTIONS_RODATA_SECTION_NAME,,)
	{
		. = ALIGN(4);
		__app_entry_table = .;
		KEEP(*(.app_entry))
		__app_entry_end = .;

		. = ALIGN(4);
		__service_entry_table = .;
		KEEP(*(.service_entry))
		__service_entry_end = .;

		. = ALIGN(4);
		__view_entry_table = .;
		KEEP(*(.view_entry))
		__view_entry_end = .;

	} GROUP_LINK_IN(ROMABLE_REGION)

#ifdef CONFIG_SENSOR_ALGO

#include <sensor_algo.ld>

#endif

	SECTION_PROLOGUE(_TEXT_SECTION_NAME,,)
	{
	__text_region_start = .;

#include <linker/kobject-text.ld>

		*(.text)
		*(".text.*")
		*(.gnu.linkonce.t.*)
		/*
		 * These are here according to 'arm-zephyr-elf-ld --verbose',
		 * after .gnu.linkonce.t.*
		 */
		*(.glue_7t) *(.glue_7) *(.vfp11_veneer) *(.v4_bx)

	} GROUP_LINK_IN(ROMABLE_REGION)

	__text_region_end = .;

/*#if defined (CONFIG_CPLUSPLUS)*/
	SECTION_PROLOGUE(.ARM.extab,,)
	{
		/*
		 * .ARM.extab section containing exception unwinding information.
		 */
		*(.ARM.extab* .gnu.linkonce.armextab.*)
	} GROUP_LINK_IN(ROMABLE_REGION)
/*#endif*/

	SECTION_PROLOGUE(.ARM.exidx,,)
	{
		/*
		 * This section, related to stack and exception unwinding, is placed
		 * explicitly to prevent it from being shared between multiple regions.
		 * It  must be defined for gcc to support 64-bit math and avoid
		 * section overlap.
		 */
		__start_unwind_idx = .;
		__exidx_start = .;
	#if defined (__GCC_LINKER_CMD__)
		*(.ARM.exidx* gnu.linkonce.armexidx.*)
	#endif
		__exidx_end = .;
		__stop_unwind_idx = .;
	} GROUP_LINK_IN(ROMABLE_REGION)

	__rodata_region_start = .;

#include <linker/common-rom.ld>
#include <linker/thread-local-storage.ld>

    SECTION_PROLOGUE(_RODATA_SECTION_NAME,,)
	{
		*(.rodata)
		*(".rodata.*")
		*(.gnu.linkonce.r.*)

/* Located in generated directory. This file is populated by the
 * zephyr_linker_sources() Cmake function.
 */
#include <snippets-rodata.ld>

#ifdef CONFIG_SECTION_OVERLAY
	    . = ALIGN(4);
		__overlay_table = .;
		LONG(OVERLAY_TABLE_MAGIC)
		/* overlay items count */
		LONG(7)

		/* for a1_wav_p.a */
		LONG(OVERLAY_ID_LIBAPWAV)
		LONG(0);
		LONG(0);
		LONG(0);
		LONG(ABSOLUTE(ADDR(.overlay.data.apwav)));
		LONG(SIZEOF(.overlay.data.apwav));
		LONG(LOADADDR(.overlay.data.apwav));
		LONG(ABSOLUTE(ADDR(.overlay.bss.apwav)));
		LONG(SIZEOF(.overlay.bss.apwav));

		/* for a1_mp3_p.a */
		LONG(OVERLAY_ID_LIBAPMP3)
		LONG(0);
		LONG(0);
		LONG(0);
		LONG(ABSOLUTE(ADDR(.overlay.data.apmp3)));
		LONG(SIZEOF(.overlay.data.apmp3));
		LONG(LOADADDR(.overlay.data.apmp3));
		LONG(ABSOLUTE(ADDR(.overlay.bss.apmp3)));
		LONG(SIZEOF(.overlay.bss.apmp3));

		/* for a1_ape_p.a */
		LONG(OVERLAY_ID_LIBAPAPE)
		LONG(0);
		LONG(0);
		LONG(0);
		LONG(ABSOLUTE(ADDR(.overlay.data.apape)));
		LONG(SIZEOF(.overlay.data.apape));
		LONG(LOADADDR(.overlay.data.apape));
		LONG(ABSOLUTE(ADDR(.overlay.bss.apape)));
		LONG(SIZEOF(.overlay.bss.apape));

		/* for a1_w13_p.a */
		LONG(OVERLAY_ID_LIBAPWMA)
		LONG(0);
		LONG(0);
		LONG(0);
		LONG(ABSOLUTE(ADDR(.overlay.data.apwma)));
		LONG(SIZEOF(.overlay.data.apwma));
		LONG(LOADADDR(.overlay.data.apwma));
		LONG(ABSOLUTE(ADDR(.overlay.bss.apwma)));
		LONG(SIZEOF(.overlay.bss.apwma));

		/* for a1_fla_p.a */
		LONG(OVERLAY_ID_LIBADFLA)
		LONG(0);
		LONG(0);
		LONG(0);
		LONG(ABSOLUTE(ADDR(.overlay.data.apfla)));
		LONG(SIZEOF(.overlay.data.apfla));
		LONG(LOADADDR(.overlay.data.apfla));
		LONG(ABSOLUTE(ADDR(.overlay.bss.apfla)));
		LONG(SIZEOF(.overlay.bss.apfla));

		/* for a1_a23_p.a */
		LONG(OVERLAY_ID_LIBAPAAC)
		LONG(0);
		LONG(0);
		LONG(0);
		LONG(ABSOLUTE(ADDR(.overlay.data.apaac)));
		LONG(SIZEOF(.overlay.data.apaac));
		LONG(LOADADDR(.overlay.data.apaac));
		LONG(ABSOLUTE(ADDR(.overlay.bss.apaac)));
		LONG(SIZEOF(.overlay.bss.apaac));

		. = ALIGN(4);
#endif

#include <linker/kobject-rom.ld>

		/*
		 * For XIP images, in order to avoid the situation when __data_rom_start
		 * is 32-bit aligned, but the actual data is placed right after rodata
		 * section, which may not end exactly at 32-bit border, pad rodata
		 * section, so __data_rom_start points at data and it is 32-bit aligned.
		 *
		 * On non-XIP images this may enlarge image size up to 3 bytes. This
		 * generally is not an issue, since modern ROM and FLASH memory is
		 * usually 4k aligned.
		 */
		. = ALIGN(4);
	} GROUP_LINK_IN(ROMABLE_REGION)

#include <linker/cplusplus-rom.ld>

	__rodata_region_end = .;
	MPU_ALIGN(__rodata_region_end -__rom_region_start);
	__rom_region_end = .;
	__rom_region_size = __rom_region_end - __rom_region_start;

    GROUP_END(ROMABLE_REGION)


	/*
	 * These are here according to 'arm-zephyr-elf-ld --verbose',
	 * before data section.
	 */
	/DISCARD/ : {
		*(.got.plt)
		*(.igot.plt)
		*(.got)
		*(.igot)
	}

	. = RAM_ADDR;
	__ramdump_sram_start = .;

	/* Align the start of image SRAM with the
	 * minimum granularity required by MPU.
	 */
	. = ALIGN(_region_min_align);
	_image_ram_start = .;

	/* ================================= sram data =================================*/
	SECTION_DATA_PROLOGUE(_SRAM_CODE_SECTION_NAME,,)
	{
		. = ALIGN(4);
		_sram_data_start = .;
		_sram_func_start = .;
		 *(.sleepfunc)
		 *(".sleepfunc.*")
		 *(.defunc)
		 *(.lvglfunc)
		 *(.vglite.func)
		_sram_func_end = .;
	}GROUP_DATA_LINK_IN(SRAM, ROMABLE_REGION)
	_sram_func_ram_size = _sram_func_end - _sram_func_start;
	_sram_func_rom_start = LOADADDR(_SRAM_CODE_SECTION_NAME);

	SECTION_DATA_PROLOGUE(_SRAM_DATA_SECTION_NAME,,)
	{
		. = ALIGN(4);
		 *(.sleep.data*)
	}GROUP_DATA_LINK_IN(SRAM, ROMABLE_REGION)
	_sram_data_end = .;
	_sram_data_ram_size = _sram_data_end - _sram_data_start;
	_sram_data_rom_start = LOADADDR(_SRAM_CODE_SECTION_NAME);

	/* ================================= psram data =================================*/
	OVERLAY : NOCROSSREFS
	{
		.overlay.data.apfla {
			*a1_fla_p.a:*(.data .data.*)
		}

		.overlay.data.apwma {
			*a1_w13_p.a:*(.data .data.*)
		}

		.overlay.data.apape {
			*a1_ape_p.a:*(.data .data.*)
		}

		.overlay.data.apmp3 {
			*a1_mp3_p.a:*(.data .data.*)
		}

		.overlay.data.apwav {
			*a1_wav_p.a:*(.data .data.*)
		}

		.overlay.data.apaac {
			*a1_a13_p.a:*(.data .data.*)
		}
	} GROUP_DATA_LINK_IN(PSRAM, ROMABLE_REGION)

	__ramdump_psram_start = ALIGN(4);

	/* Located in generated directory. This file is populated by the
	 * zephyr_linker_sources() Cmake function.
	 */
	#include <snippets-ram-sections.ld>
	SECTION_DATA_PROLOGUE(_DATA_SECTION_NAME,,)
	{
		__data_region_start = .;
		__data_start = .;
		*(.data)
		*(".data.*")
		*(".kernel.*")

		/* Located in generated directory. This file is populated by the
		 * zephyr_linker_sources() Cmake function.
		 */
		#include <snippets-rwdata.ld>
	__data_end = .;

	} GROUP_DATA_LINK_IN(PSRAM, ROMABLE_REGION)
    __data_size = __data_end - __data_start;
    __data_load_start = LOADADDR(_DATA_SECTION_NAME);

    __data_region_load_start = LOADADDR(_DATA_SECTION_NAME);

	#include <linker/common-ram.ld>
	#include <linker/kobject-data.ld>
	#include <linker/cplusplus-ram.ld>

    __data_region_end = .;

	/* ================================= sram bss =================================*/
	SECTION_PROLOGUE(_SRAM_BSS_SECTION_NAME,(NOLOAD),)
	{
		__sram_bss_start = .;
		*(.lvgl.bss*)
		*(.vglite.bss*)
		__sram_bss_end = .;
	} GROUP_LINK_IN(SRAM)

	/* ================================= psram bss ================================= */
	OVERLAY : NOCROSSREFS
	{
		.overlay.bss.apfla {
			*a1_fla_p.a:*(.bss .bss.* .scommon COMMON)
		}

		.overlay.bss.apwma {
			*a1_w13_p.a:*(.bss .bss.* .scommon COMMON)
		}

		.overlay.bss.apape {
			*a1_ape_p.a:*(.bss .bss.* .scommon COMMON)
		}

		.overlay.bss.apmp3 {
			*a1_mp3_p.a:*(.bss .bss.* .scommon COMMON)
		}

		.overlay.bss.apwav {
			*a1_wav_p.a:*(.bss .bss.* .scommon COMMON)
		}

		.overlay.bss.apaac {
			*a1_a13_p.a:*(.bss .bss.* .scommon COMMON)
		}
	} GROUP_LINK_IN(PSRAM)

	__psram_bss_start = .;
	__bss_start = .;
	SECTION_PROLOGUE(_BSS_SECTION_NAME,(NOLOAD),)
	{
		/*
		 * For performance, BSS section is assumed to be 4 byte aligned and
		 * a multiple of 4 bytes
		 */
		. = ALIGN(4);
		*(.bss)
		*(.bss.*)
		*(.scommon)
		*(COMMON)
		/*
		 * As memory is cleared in words only, it is simpler to ensure the BSS
		 * section ends on a 4 byte boundary. This wastes a maximum of 3 bytes.
		 */

	} GROUP_LINK_IN(PSRAM)

	SECTION_PROLOGUE(_PSRAM_BSS_SECTION_NAME,(NOLOAD),)
	{
		/*
		 * For performance, BSS section is assumed to be 4 byte aligned and
		 * a multiple of 4 bytes
		 */
		. = ALIGN(4);
		*(".kernel_bss.*")
		*(.bthost_bss*)
		*(.btsrv_bss*)
	} GROUP_LINK_IN(PSRAM)

	__bss_end = ALIGN(4);
	__psram_bss_end = ALIGN(4);

	/* ================================= sram noinit ================================= */
	SECTION_PROLOGUE(_SRAM_NOINIT_SECTION_NAME,(NOLOAD),)
	{
		. = ALIGN(512);
		__kernel_ram_start = .;
		*(.srm_irq_vector*)
		*(.interrupt.noinit.stack*)
		*(.main.noinit.stack*)
		*(.uisrv.noinit.stack*)
		*(.lvgl.noinit.gpu*)
		*(.spinand.bss.BLK_ARRAY*)
		*(.spinand.bss.PAGE_CACHE_BUF*)
		*(.system.bss.sdfs_cache*)
		*(.diskio.cache.pool*)
		*(.jpeg.bss.temp_buffer*)
		*(.ram.noinit*)
		*(.decompress.bss.cache*)
		*(.ram.noinit.stack*)
		*(.audio.bss.ouput_pcm*)
		*(.audio.bss.input_pcm*)
		*(.act_s2_not_save_mem*)

		__ramdump_sram_end = ALIGN(4);

		*(.lvgl.noinit.vdb*)
		__kernel_ram_save_end = .;

	}GROUP_LINK_IN(SRAM)

	SECTION_PROLOGUE(_SRAM_SLEEP_SHUTDOWN_SECTION_NAME,(NOLOAD),)
	{
		. = ALIGN(512);
		_sleep_shutdown_ram_start = .;
		*(.sram.noinit.sufacebuffer*)
		_sleep_shutdown_ram_end = .;
	}GROUP_LINK_IN(SRAM)

#ifdef CONFIG_SIM_FLASH_ACTS
	SECTION_PROLOGUE(SIM_ACTLOG,(NOLOAD),)
	{
		. = ALIGN(4);
		__sim_flash_ram_start = .;
		*(.sram.noinit.actlog*)
		__sim_flash_ram_end = .;
	}GROUP_LINK_IN(SRAM)
#endif

	/* ================================= psram noinit ================================= */
	SECTION_PROLOGUE(_NOINIT_SECTION_NAME,(NOLOAD),)
	{
		/*
		* This section is used for non-initialized objects that
		* will not be cleared during the boot process.
		*/
		*(.noinit)
		*(".noinit.*")
		*(".kernel_noinit.*")

		/* Located in generated directory. This file is populated by the
		 * zephyr_linker_sources() Cmake function.
		 */
#include <snippets-noinit.ld>
#ifdef CONFIG_SOC_NOINIT_LD
#include <soc-noinit.ld>
#endif

	} GROUP_LINK_IN(PSRAM)

	SECTION_PROLOGUE(_PRAM_NOINIT_SECTION_NAME, (NOLOAD),SUBALIGN(64))
	{
		*(.lvgl.noinit.malloc*)
		*(.vglite.noinit.mem_pool*)
		*(.vglite.noinit.malloc*)
#ifdef CONFIG_VIDEO_PLAYER
		*(.VIDEO_PSRAM_REGION*)
#endif
		__ramdump_psram_end = ALIGN(4);

		*(.UI_PSRAM_REGION*)
		*(.RES_PSRAM_REGION*)
		*(.BMPFONT_PSRAM_REGION*)
		*(.font.bss.cache*)
		*(.tile.bss.cache*)
	} GROUP_LINK_IN(PSRAM)

#ifdef CONFIG_DEBUG_TRACEDUMP
	SECTION_PROLOGUE(_TRACEDUMP_SECTION_NAME,(NOLOAD),)
	{
		__tracedump_ram_start = .;
		. = . + 0x10000;
		__tracedump_ram_end = .;
	} GROUP_LINK_IN(PSRAM)
#endif

	/* Define linker symbols */
	_image_ram_end = .;
	__kernel_ram_end = RAM_ADDR + RAM_SIZE;
	__kernel_ram_size = __kernel_ram_end - __kernel_ram_start;

	SECTION_PROLOGUE(_ATT_RUNTIME_DATA_SECTION_NAME,(NOLOAD),)
	{
        /*
         * For att runtime data
         */
        . = ALIGN(4);
        KEEP(*(.attruntimedata*))
	}GROUP_LINK_IN(DSP_SRAM)

	/* ================================= share ram noinit ================================= */
	SECTION_DATA_PROLOGUE(_SHARE_RAM_BSS_SECTION_NAME, (NOLOAD),SUBALIGN(4))
	{
		__share_ram_start = .;
		*(.DSP_SHARE_RAM*)
	} GROUP_LINK_IN(SHARE_RAM)
	__share_ram_end = .;

	_end = .; /* end of image */

	/* Located in generated directory. This file is populated by the
	 * zephyr_linker_sources() Cmake function.
	 */
#include <snippets-sections.ld>

#include <linker/debug-sections.ld>

    /DISCARD/ : { *(.note.GNU-stack) }

    SECTION_PROLOGUE(.ARM.attributes, 0,)
	{
		KEEP(*(.ARM.attributes))
		KEEP(*(.gnu.attributes))
	}

	/* Must be last in romable region */
	SECTION_PROLOGUE(.last_section,(NOLOAD),)
	{

	} GROUP_LINK_IN(ROMABLE_REGION)

	/* To provide the image size as a const expression,
	 * calculate this value here. */
	_flash_used = LOADADDR(.last_section) - __rom_region_start;

}
