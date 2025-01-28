/*
 * Copyright (c) 2017 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief SPICACHE profile interface for Actions SoC
 */

#include <errno.h>
#include <kernel.h>
#include "soc.h"
#include "spicache.h"
#include "board_cfg.h"

#define SPI1_CACHE_SIZE (1024*32)


#ifdef CONFIG_SOC_SPICACHE_PROFILE
static void spicache_update_profile_data(struct spicache_profile *profile)
{
	if (!profile->spi_id) {
		profile->hit_cnt = sys_read32(SPICACHE_RANGE_ADDR_HIT_COUNT);
		profile->miss_cnt = sys_read32(SPICACHE_RANGE_ADDR_MISS_COUNT);
		profile->total_hit_cnt = sys_read32(SPICACHE_TOTAL_HIT_COUNT);
		profile->total_miss_cnt = sys_read32(SPICACHE_TOTAL_MISS_COUNT);
	} else {
		profile->hit_cnt = sys_read32(SPI1_CACHE_MCPU_HIT_COUNT);
		profile->miss_cnt = sys_read32(SPI1_CACHE_MCPU_MISS_COUNT);
		profile->dma_hit_cnt = sys_read32(SPI1_CACHE_DMA_HIT_COUNT);
		profile->dma_miss_cnt = sys_read32(SPI1_CACHE_DMA_MISS_COUNT);
	}
}

int spicache_profile_get_data(struct spicache_profile *profile)
{
	if (!profile)
		return -EINVAL;

	spicache_update_profile_data(profile);

	return 0;
}

int spicache_profile_stop(struct spicache_profile *profile)
{
	if (!profile)
		return -EINVAL;

	profile->end_time = k_cycle_get_32();
	spicache_update_profile_data(profile);

	if (!profile->spi_id)
		sys_write32(sys_read32(SPICACHE_CTL) & ~(1 << 4), SPICACHE_CTL);
	else
		sys_write32(sys_read32(SPI1_CACHE_CTL) & ~(1 << 4), SPI1_CACHE_CTL);

	return 0;
}

int spicache_profile_start(struct spicache_profile *profile)
{
	if (!profile)
		return -EINVAL;

	if (!profile->spi_id) {
		sys_write32(profile->start_addr, SPICACHE_PROFILE_ADDR_START);
		sys_write32(profile->end_addr, SPICACHE_PROFILE_ADDR_END);
	} else {
		sys_write32(profile->start_addr, SPI1_CACHE_PROFILE_ADDR_START);
		sys_write32(profile->end_addr, SPI1_CACHE_PROFILE_ADDR_END);
	}

	profile->start_time = k_cycle_get_32();

	if (!profile->spi_id)
		sys_write32(sys_read32(SPICACHE_CTL) | (1 << 4), SPICACHE_CTL);
	else
		sys_write32(sys_read32(SPI1_CACHE_CTL) | (1 << 4), SPI1_CACHE_CTL);

	return 0;
}
#endif

int spicache_master_enable(int spi_id, SPI_CACHE_MASTER master_id)
{
	if (!spi_id) {
		if ((master_id >= SPI0_CACHE_MASTER_DMA) && (master_id <= SPI0_CACHE_MASTER_DE))
			sys_write32(sys_read32(SPICACHE_CTL) | (1<<((master_id - SPI0_CACHE_MASTER_DMA) + 9)), SPICACHE_CTL);
	} else {
		if ((master_id >= SPI1_CACHE_MASTER_GPU) && (master_id <= SPI1_CACHE_MASTER_DE))
			sys_write32(sys_read32(SPI1_CACHE_CTL) | (1<<((master_id - SPI1_CACHE_MASTER_GPU) + 8)), SPI1_CACHE_CTL);
	}

	return 0;
}

int spicache_master_disable(int spi_id, SPI_CACHE_MASTER master_id)
{
	if (!spi_id) {
		if ((master_id >= SPI0_CACHE_MASTER_DMA) && (master_id<=SPI0_CACHE_MASTER_DE))
			sys_write32(sys_read32(SPICACHE_CTL) & ~(1<<((master_id - SPI0_CACHE_MASTER_DMA) + 9)), SPICACHE_CTL);
	} else {
		if ((master_id >= SPI1_CACHE_MASTER_GPU) && (master_id <= SPI1_CACHE_MASTER_DE))
			sys_write32(sys_read32(SPI1_CACHE_CTL) & ~(1<<((master_id - SPI1_CACHE_MASTER_GPU) + 8)), SPI1_CACHE_CTL);
	}

	return 0;
}

int spicache_set_priority(int spi_id, SPI_CACHE_PRIORITY priority)
{
	if (!spi_id) {
		sys_write32((sys_read32(SPICACHE_CTL) & ~(0xf<<20)) | ((priority - SPI0_CACHE_PRIORITY_POLL)<<20), SPICACHE_CTL);
	} else {
		sys_write32((sys_read32(SPI1_CACHE_CTL) & ~(0xf<<20)) | ((priority - SPI1_CACHE_PRIORITY_POLL)<<20), SPI1_CACHE_CTL);
	}

	return 0;
}

void spi1_cache_ops_wait_finshed(void)
{
	while(sys_test_bit(SPI1_CACHE_OPERATE, 0));

}


void spi1_cache_ops(SPI_CACHE_OPS ops, void* addr, int size)
{
	// uint32_t end;
	uint32_t off = (uint32_t)addr;
	uint32_t op;

	if((off < SPI1_CACHE_WT_WNA_ADDR) || (off > SPI1_UNCACHE_END_ADDR))
		return;

	spi1_cache_ops_wait_finshed();

	switch(ops){
		case SPI_CACHE_FLUSH:
			sys_write32((uint32_t)addr, CACHE_OPERATE_ADDR_START);
			sys_write32((uint32_t)addr + size - 1, CACHE_OPERATE_ADDR_END);
			op = 0x13; // write back // configurable address space mode
			sys_write32(op, SPI1_CACHE_OPERATE);
			spi1_cache_ops_wait_finshed();
			return;
		case SPI_CACHE_INVALIDATE:
			sys_write32((uint32_t)addr, CACHE_OPERATE_ADDR_START);
			sys_write32((uint32_t)addr + size - 1, CACHE_OPERATE_ADDR_END);
			op = 0x0b; // invalid // configurable address space mode
			sys_write32(op, SPI1_CACHE_OPERATE);
			spi1_cache_ops_wait_finshed();
			return;
		case SPI_WRITEBUF_FLUSH:
			sys_write32(0x1f, SPI1_CACHE_OPERATE);
			spi1_cache_ops_wait_finshed();
			return;
		case SPI_CACHE_FLUSH_ALL://flush all
			sys_write32(0x11, SPI1_CACHE_OPERATE);
			return ;
		case SPI_CACHE_INVALID_ALL: //invalid all
			sys_write32(0x9, SPI1_CACHE_OPERATE);
			spi1_cache_ops_wait_finshed();
			return ;
		case SPI_CACHE_FLUSH_INVALID:
			sys_write32((uint32_t)addr, CACHE_OPERATE_ADDR_START);
			sys_write32((uint32_t)addr + size - 1, CACHE_OPERATE_ADDR_END);
			op = 0x1b; // writeback & invalid // configurable address space mode
			sys_write32(op, SPI1_CACHE_OPERATE);
			spi1_cache_ops_wait_finshed();
			return;
		case SPI_CACHE_FLUSH_INVALID_ALL:
			sys_write32(0x19, SPI1_CACHE_OPERATE); //flush and invalid all
			return ;
		default:
			return;
	}

	// /*address mode operate*/
	// off = (off - SPI1_BASE_ADDR);
	// end = off + size;
	// off &= (~0x1f);

	// /*not flush all*/
	// while(off < end) {
	// 	sys_write32(op|off, SPI1_CACHE_OPERATE);
	// 	off+= 0x20;
	// 	//while(sys_test_bit(SPI1_CACHE_OPERATE, 0));
	// 	spi1_cache_ops_wait_finshed();
	// }

}

static uint32_t system_phy_addr, sdfs_phy_addr;
static uint32_t get_system_phy_addr(void)
{
	if (system_phy_addr == 0) {
		system_phy_addr = soc_boot_get_info()->system_phy_addr;
	}
	return system_phy_addr;
}

#include <partition/partition.h>
static uint32_t get_sdfs_phy_addr(void)
{
	const struct partition_entry *part;

	if (sdfs_phy_addr == 0) {
		part = partition_get_part(PARTITION_FILE_ID_SDFS);
		if (part) {
			sdfs_phy_addr = part->file_offset;
		}
	}

	return sdfs_phy_addr;
}

void * cache_to_uncache(void *vaddr)
{
	void *pvadr = vaddr;

	if (buf_is_nor(vaddr)) {
		pvadr = (void *) (SPI0_UNCACHE_ADDR + (((uint32_t)vaddr) - SPI0_BASE_ADDR));
	} else if (buf_is_psram_cache(vaddr)) {
		pvadr = (void *) (SPI1_UNCACHE_ADDR + (((uint32_t)vaddr) - SPI1_BASE_ADDR));
	} else if (buf_is_psram_wt_wna(vaddr)) {
		pvadr = (void *) (SPI1_UNCACHE_ADDR + (((uint32_t)vaddr) - SPI1_CACHE_WT_WNA_ADDR));
	}

	return pvadr;
}

void * cache_to_unmap_uncache(void *vaddr)
{
	void *pvadr = vaddr;

	if (buf_is_nor(vaddr)) {
		uint32_t vaddr_val = (uint32_t)vaddr;
		if ((vaddr_val & 0xff000000) == CONFIG_FLASH_BASE_ADDRESS) {
			pvadr = (void *) (SPI0_UNCACHE_ADDR + (vaddr_val - CONFIG_FLASH_BASE_ADDRESS + get_system_phy_addr()));
		} else if ((vaddr_val & 0xff000000) == CONFIG_SPI_XIP_VADDR) {
			pvadr = (void *) ((vaddr_val - CONFIG_SPI_XIP_VADDR + SPI0_UNCACHE_ADDR));
		}
	}
	return pvadr;
}

void * cache_to_uncache_by_master(void *vaddr, SPI_CACHE_MASTER master_id)
{
	if ((master_id >= SPI0_CACHE_MASTER_DMA) && (master_id<=SPI0_CACHE_MASTER_DE))
		return cache_to_unmap_uncache(vaddr);

	return cache_to_uncache(vaddr);
}

void * uncache_to_cache(void *paddr)
{
	void *vaddr = paddr;

	if (buf_is_nor_un(paddr)) {
		vaddr = (void *) (SPI0_BASE_ADDR + (((uint32_t)paddr) - SPI0_UNCACHE_ADDR));
	} else if (buf_is_psram_un(paddr)) {
		vaddr = (void *) (SPI1_BASE_ADDR + (((uint32_t)paddr) - SPI1_UNCACHE_ADDR));
	}

	return vaddr;
}

void * unmap_uncache_to_cache(void *paddr)
{
	void *vaddr = paddr;

	if (buf_is_nor_un(paddr)) {
		uint32_t offs = (uint32_t)paddr - SPI0_UNCACHE_ADDR;
		if ((offs >= get_system_phy_addr()) && (offs < get_sdfs_phy_addr())) {
			vaddr = (void *) (SPI0_BASE_ADDR + (offs- get_system_phy_addr()));
		} else if (offs >= get_sdfs_phy_addr()) {
			vaddr = (void *) (CONFIG_SD_FS_VADDR_START + (offs - get_sdfs_phy_addr()));
		}
	}
	return vaddr;
}

void * uncache_to_cache_by_master(void *paddr, SPI_CACHE_MASTER master_id)
{
	if ((master_id >= SPI0_CACHE_MASTER_DMA) && (master_id<=SPI0_CACHE_MASTER_DE))
		return unmap_uncache_to_cache(paddr);

	return uncache_to_cache(paddr);
}


void * uncache_to_wt_wna_cache(void *paddr)
{
	void *vaddr = paddr;

	if (buf_is_psram_un(paddr)) {
		vaddr = (void *) (SPI1_CACHE_WT_WNA_ADDR + (((uint32_t)paddr) - SPI1_UNCACHE_ADDR));
	}

	return vaddr;
}

void * cache_to_wt_wna_cache(void *vaddr)
{
	void *pvadr = vaddr;

	if (buf_is_psram_cache(vaddr)) {
		pvadr = (void *) (SPI1_CACHE_WT_WNA_ADDR + (((uint32_t)vaddr) - SPI1_BASE_ADDR));
	}

	return pvadr;
}

void * wt_wna_cache_to_cache(void *vaddr)
{
	void *pvadr = vaddr;

	if (buf_is_psram_wt_wna(vaddr)) {
		pvadr = (void *) (SPI1_BASE_ADDR + (((uint32_t)vaddr) - SPI1_CACHE_WT_WNA_ADDR));
	}

	return pvadr;
}
