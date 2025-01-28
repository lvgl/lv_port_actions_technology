/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file ram_dump memory config
 */

#include <string.h>
#include <zephyr/types.h>
#include <linker/linker-defs.h>
#include "ramdump_core.h"

const ramd_addr_t ramd_mem_regions[] = {
	//sram region
	{(uintptr_t)0x2ff18000, (uintptr_t)0x2ff20000, 0},  // ShareRAM (32K)
	{(uintptr_t)0x2ff20000, (uintptr_t)0x2ff30000, TYPE_BTCPU_DBG},  // BTRAM (64K)
	//{(uintptr_t)0x2ff30000, (uintptr_t)0x2ff48000, 0},  // DSP_DTCM_RAM (96K)
	//{(uintptr_t)0x2ff50000, (uintptr_t)0x2ff58000, 0},  // JPEG_OUTRAM (32K)
	{(uintptr_t)0x31000000, (uintptr_t)0x31000400, 0},  // TraceRAM (1K)
	{(uintptr_t)&__ramdump_sram_start, (uintptr_t)&__ramdump_sram_end, 0},  // RAM0 ~ RAM16 (992K)

#ifndef CONFIG_SOC_NO_PSRAM
	//psram region
	{(uintptr_t)&__ramdump_psram_start, (uintptr_t)&__ramdump_psram_end, 0},
#endif

#ifdef CONFIG_DEBUG_TRACEDUMP
	{(uintptr_t)&__tracedump_ram_start, (uintptr_t)&__tracedump_ram_end, TYPE_TRACEDUMP_DBG},
#endif

	//soc peripheral register
	{(uintptr_t)0x40000000, (uintptr_t)0x40050000, 0}, // ignore USB(0x40050000)
	{(uintptr_t)0x40054000, (uintptr_t)0x4009C000, 0},

	//cortex-m4 peripheral register
	{(uintptr_t)0xe000e000, (uintptr_t)0xe000f000, 0},

	{0, 0} /* End of list */
};

