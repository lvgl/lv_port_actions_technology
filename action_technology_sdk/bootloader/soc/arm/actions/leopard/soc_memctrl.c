/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file memory controller interface
 */

#include <kernel.h>
#include <soc.h>
#include "soc_memctrl.h"

#define SOC_MEMCTRL_TEMP_MAP_CPU_ADDR 0x13800000

void soc_memctrl_set_mapping(int idx, u32_t map_addr, u32_t nor_bus_addr)
{
	u32_t bus_addr;

	if (idx >= CACHE_MAPPING_ITEM_NUM)
		return;

	bus_addr = soc_memctrl_cpu_addr_to_bus_addr(map_addr);
	/* default: mapping size: 2MB */
	sys_write32((bus_addr & ~0x3f) | 0x1f, SPI_CACHE_MAPPING_ADDR0 + idx * 8);
	/*set entry must afer mapping set*/
	sys_write32(nor_bus_addr, SPI_CACHE_ADDR0_ENTRY + idx * 8);
}

int soc_memctrl_item_is_free(int idx)
{
	u32_t map_reg_val;

	map_reg_val = sys_read32(SPI_CACHE_MAPPING_ADDR0 + idx * 8);

	return (map_reg_val ? 0 : 1);
}

void soc_memctrl_cache_invalid(void)
{
	sys_write32(1, SPICACHE_INVALIDATE);
	while ((sys_read32(SPICACHE_INVALIDATE) & 0x1) == 1)
		;
}

int soc_memctrl_mapping(u32_t map_addr, u32_t nor_phy_addr, int enable_crc)
{
	u32_t nor_bus_addr;
	int i;

	if (nor_phy_addr % 0x1000)	{
		printk("invalid nor phy addr 0x%x\n", nor_phy_addr);
		return -EINVAL;
	}

	if (map_addr & (0x1ffff0))	{
		printk("invalid cpu addr 0x%x for mapping to 2MB, bit[4~20] must be 0\n", map_addr);
		return -EINVAL;
	}

	nor_bus_addr = nor_phy_addr;
	if (enable_crc) {
		nor_bus_addr |= (1 << 31);
	}

	for (i = CACHE_MAPPING_ITEM_NUM - 1; i >= 0; i--) {
		if (soc_memctrl_item_is_free(i)) {
			printk("slot=%d  to map nor phy addr 0x%x, map=0x%x\n", i, nor_phy_addr, map_addr);
			soc_memctrl_set_mapping(i, map_addr, nor_bus_addr);
			soc_memctrl_cache_invalid();
			return 0;
		}
	}

	printk("cannot found slot to map nor phy addr 0x%x\n", nor_phy_addr);
	return -ENOENT;
}

int soc_memctrl_unmapping(u32_t map_addr)
{
	u32_t bus_addr, map_reg_val;
	int i;

	bus_addr = soc_memctrl_cpu_addr_to_bus_addr(map_addr);

	for (i = CACHE_MAPPING_ITEM_NUM - 1; i >= 0; i--) {
		map_reg_val = sys_read32(SPI_CACHE_MAPPING_ADDR0 + i * 8);
		if (map_reg_val == 0)
			continue;

		if ((map_reg_val & ~0x3F) == bus_addr) {
			sys_write32(0x0, SPI_CACHE_ADDR0_ENTRY + i * 8);
			sys_write32(0x0, SPI_CACHE_MAPPING_ADDR0 + i * 8);
			soc_memctrl_cache_invalid();
			return 0;
		}

	}

	printk("cannot found slot to map cpu addr 0x%x\n", map_addr);
	return -ENOENT;
}

void *soc_memctrl_create_temp_mapping(u32_t nor_phy_addr, int enable_crc)
{
	u32_t tmp_cpu_addr = SOC_MEMCTRL_TEMP_MAP_CPU_ADDR;
	int err;

	printk("%s: create temp mapping 0x%x, nor_phy_addr 0x%x, enable_crc %d\n", __func__,
		tmp_cpu_addr, nor_phy_addr, enable_crc);

	err = soc_memctrl_mapping(tmp_cpu_addr, nor_phy_addr, enable_crc);
	if (err < 0)
		return NULL;

	return (void *)tmp_cpu_addr;
}

void soc_memctrl_clear_temp_mapping(void *cpu_addr)
{
	soc_memctrl_unmapping((u32_t)cpu_addr);
}

#if 0

u32_t soc_memctrl_cpu_to_nor_phy_addr(u32_t cpu_addr)
{
	u32_t map_reg_val, map_start_addr, map_end_addr;
	u32_t nor_reg_val, nor_addr, bus_addr, bus_offset;
	int i;

	bus_addr = soc_memctrl_cpu_addr_to_bus_addr(cpu_addr);

	for (i = 0; i < CACHE_MAPPING_ITEM_NUM; i++) {
		map_reg_val = sys_read32(MAPPING_ADDR0 + i * 8);
		if (map_reg_val == 0)
			continue;

		map_start_addr = map_reg_val & ~0xfffff;
		map_end_addr = map_start_addr + (((map_reg_val & 0xf) + 1) << 20);

		if (bus_addr >= map_start_addr && bus_addr < map_end_addr) {
			nor_reg_val = sys_read32(ADDR0_ENTRY + i * 8);

			bus_offset = bus_addr - map_start_addr;
			nor_addr = (nor_reg_val & 0xffffff) + bus_offset;
			if (nor_reg_val >> 31) {
				/* crc is enabled for this mapping */
				nor_addr = (nor_reg_val & 0xffffff) + (bus_offset / 32 * 34) + (bus_offset % 32);
			}

			return nor_addr;
		}
	}

	return -1;
}


__ramfunc void soc_memctrl_config_cache_size(int cache_size_mode)
{
    if (cache_size_mode == CACHE_SIZE_16KB) {
        sys_write32(sys_read32(SPICACHE_CTL) | (1 << SPICACHE_CTL_CACHE_SIZE_MODE), SPICACHE_CTL);
        k_busy_wait(10);
        while (1) {
            if ((sys_read32(SPICACHE_CTL) & (1 << SPICACHE_CTL_CACHE_AUTO_INVALID_FLAG)) == 0)
                break;
        }
        sys_write32(sys_read32(CMU_MEMCLKEN) | (1 << 8), CMU_MEMCLKEN);
        k_busy_wait(10);
    }
}
#endif
