/*
 * Copyright (c) 2017 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief NVRAM storage
 */

#ifndef PC
#include <errno.h>
#include <sys/__assert.h>
#include <stdbool.h>
#include <kernel.h>
#include <device.h>
#include <init.h>
#include <drivers/flash.h>
#include <string.h>
#include <board_cfg.h>
#include "nvram_storage.h"

#define LOG_LEVEL 2
#include <logging/log.h>
LOG_MODULE_REGISTER(nvram_stor);
#else
#include "nvram_test_simulate.h"
#endif

#ifdef CONFIG_NVRAM_STORAGE_RAMSIM
#define NVRAM_MEM_SIZE	(CONFIG_NVRAM_FACTORY_REGION_SIZE + \
			 CONFIG_NVRAM_FACTORY_RW_REGION_SIZE + CONFIG_NVRAM_USER_REGION_SIZE)
#define NVRAM_MEM_OFFS	CONFIG_NVRAM_FACTORY_REGION_BASE_ADDR

uint8_t nvram_mem_buf[NVRAM_MEM_SIZE];
#endif

#if  defined(CONFIG_BOARD_NANDBOOT) || defined(CONFIG_BOARD_EMMCBOOT)

#define NV_NAND_PAGE_SIZE 2048
#define NV_NAND_SECTOR_SIZE 512
#define NV_CACHE_SECT_NUM	(NV_NAND_PAGE_SIZE/NV_NAND_SECTOR_SIZE)	
#define NV_CACEH_NUM_ITEM	2
struct  nv_nand_cache_item {	
	u16_t cache_valid:1;
	u16_t write_valid:1;
	u16_t reserved:14;
	u16_t seq_index;

	u32_t cache_start_sector;
	u8_t  cache_data[NV_NAND_PAGE_SIZE];
};

static struct nv_nand_cache_item __act_s2_notsave nv_nand_cache[NV_CACEH_NUM_ITEM];
static u16_t g_seq_index;
static void _nv_nand_cache_init(void)
{
	int i;
	for(i = 0; i < NV_CACEH_NUM_ITEM; i++) {
		nv_nand_cache[i].cache_valid = 0;
		nv_nand_cache[i].write_valid = 0;
	}
}

static int  _nv_flush_cache(struct device *dev, struct nv_nand_cache_item *item)
{
	int err;
	if(item->write_valid && item->cache_valid){
		item->write_valid = 0;
		LOG_DBG("nand flush startsec 0x%x, buf %p",	item->cache_start_sector, item->cache_data);
		err = flash_write(dev, (item->cache_start_sector<<9), item->cache_data, NV_CACHE_SECT_NUM<<9);
		if (err < 0) {
			LOG_ERR("nand write error %d, offsec 0x%x, buf %p",
				err, item->cache_start_sector, item->cache_data);
			return -EIO;
		}
	}
	return 0;
}
static int  _nv_update_cache(struct device *dev, struct nv_nand_cache_item *item, u32_t sec_off)
{
	int err;
	u32_t page_off = sec_off/NV_CACHE_SECT_NUM;
	item->cache_start_sector = page_off * NV_CACHE_SECT_NUM;
	item->cache_valid = 1;
	item->write_valid = 0;
	item->seq_index = g_seq_index++;
	LOG_DBG("nand update start_sec 0x%x, buf %p, sec_off=0x%x,0x%x",	
		item->cache_start_sector, item->cache_data, sec_off, sec_off*NV_NAND_SECTOR_SIZE);

	err = flash_read(dev, (item->cache_start_sector)<<9, item->cache_data, NV_CACHE_SECT_NUM<<9);
	if (err < 0) {
		LOG_ERR("nand read error %d, offsec 0x%x, buf %p",
			err, item->cache_start_sector, item->cache_data);
		return -EIO;
	}	
	return 0;
}


static struct nv_nand_cache_item * _nv_find_cache_item(u32_t sec_off)
{
	for (int i = 0; i < NV_CACEH_NUM_ITEM; i++) {
		if (nv_nand_cache[i].cache_valid && (nv_nand_cache[i].cache_start_sector <= sec_off)
			&& (sec_off < nv_nand_cache[i].cache_start_sector+NV_CACHE_SECT_NUM)) {
				return &nv_nand_cache[i];
		}
	}
	//LOG_INF("sect_off 0x%x not in cache", sec_off);
	return NULL;
}

/*
	if have invaid ,use invalid, else use same
*/
static struct nv_nand_cache_item * _nv_new_cache_item(struct device *dev, u32_t sec_off)
{
	int i;
	struct nv_nand_cache_item *invalid, *valid, *new_item;

	valid = invalid = NULL;
	for(i = 0; i < NV_CACEH_NUM_ITEM; i++) {
		if(nv_nand_cache[i].cache_valid){
			if(valid == NULL){
				valid = &nv_nand_cache[i];
			}else{
				if(valid->seq_index > nv_nand_cache[i].seq_index)
					valid = &nv_nand_cache[i];
			}
		}else{
			invalid = &nv_nand_cache[i];
			break;
		}	
	}
	if(invalid){
		new_item = invalid;
	}else{
		new_item = valid;	
	}
	//LOG_INF("new item sect_off 0x%x, 0x%x is_write=%d", sec_off, sec_off*NV_NAND_SECTOR_SIZE, is_write);
	_nv_flush_cache(dev, new_item);
	_nv_update_cache(dev, new_item, sec_off);
	return new_item;

}

void nvram_storage_flush(struct device *dev)
{
	int i;
	for(i = 0; i < NV_CACEH_NUM_ITEM; i++)
		_nv_flush_cache(dev, &nv_nand_cache[i]);
}

/*brwe = 0 (read), 1(write), 2(erase)*/
static void _nand_storage_read_write(struct device *dev, uint32_t addr,
			uint8_t *buf, int32_t size, int brwe)
{
	int32_t wsize, len;
	uint32_t sector_off, addr_in_sec,cache_sec_index;
	static struct nv_nand_cache_item * item;

	wsize = size;
	sector_off = addr / NV_NAND_SECTOR_SIZE;
	addr_in_sec = addr - (sector_off*NV_NAND_SECTOR_SIZE);
	//LOG_INF("sect_off 0x%x, size 0x%x iswrite %d", sector_off, size, brwe);
	while(wsize > 0) {		
		item = _nv_find_cache_item(sector_off);
		if(item == NULL)
			item = _nv_new_cache_item(dev, sector_off);
		cache_sec_index = sector_off -item->cache_start_sector;		
		if(addr_in_sec){//not sector algin			
			len = NV_NAND_SECTOR_SIZE - addr_in_sec;
			if(len > wsize)
				len = wsize;
			if(brwe == 1)
				memcpy(&item->cache_data[cache_sec_index*NV_NAND_SECTOR_SIZE+addr_in_sec],buf, len);
			else if(brwe == 0)
				memcpy(buf, &item->cache_data[cache_sec_index*NV_NAND_SECTOR_SIZE+addr_in_sec], len);
			else
				memset(&item->cache_data[cache_sec_index*NV_NAND_SECTOR_SIZE+addr_in_sec], 0xff, len);
			buf += len;
			wsize -= len;
			cache_sec_index++;
			addr_in_sec = 0; // next align
		}
		for( ; wsize && (cache_sec_index < NV_CACHE_SECT_NUM);  cache_sec_index++){
			if(wsize < NV_NAND_SECTOR_SIZE)
				len = wsize;
			else
				len = NV_NAND_SECTOR_SIZE;

			if(brwe == 1)
				memcpy(&item->cache_data[cache_sec_index*NV_NAND_SECTOR_SIZE],buf, len);
			else if(brwe == 0)
				memcpy(buf, &item->cache_data[cache_sec_index*NV_NAND_SECTOR_SIZE], len);
			else
				memset(&item->cache_data[cache_sec_index*NV_NAND_SECTOR_SIZE], 0xff, len);
			buf += len;
			wsize -= len;
		}
		if(brwe)
			item->write_valid = 1; //dirty

		sector_off = item->cache_start_sector + NV_CACHE_SECT_NUM;

	}

}

static int _nand_storage_write(struct device *dev, uint32_t addr,
			uint8_t *buf, int32_t size)
{

	_nand_storage_read_write(dev, addr, buf, size, 1);
	//LOG_INF("nand write end 0x%x len 0x%x", addr, size);
	return 0;
}


static int _nand_storage_read(struct device *dev, uint32_t addr,
								 void *buf, int32_t size)
{
	_nand_storage_read_write(dev, addr, buf, size, 0);
	//LOG_INF("nand read end 0x%x len 0x%x", addr, size);
	return 0;

}

static int _nand_storage_erase(struct device *dev, uint32_t addr,
				      int32_t size)
{
	uint8_t buf[1]; // not use;
	_nand_storage_read_write(dev, addr, buf, size, 2);
	//LOG_INF("nand erase end 0x%x len 0x%x", addr, size);
	return 0;
}


#endif



#define NVRAM_STORAGE_READ_SEGMENT_SIZE		256
#define NVRAM_STORAGE_WRITE_SEGMENT_SIZE	32

int nvram_storage_write(struct device *dev, uint64_t addr,
			const void *buf, int32_t size)
{
	LOG_INF("write addr 0x%llx len 0x%x", addr, size);

#ifdef CONFIG_NVRAM_STORAGE_RAMSIM
	memcpy(nvram_mem_buf + addr - NVRAM_MEM_OFFS, buf, size);
	return 0;
#else
#if  defined(CONFIG_BOARD_NANDBOOT) || defined(CONFIG_BOARD_EMMCBOOT)
	_nand_storage_write(dev, addr, (uint8_t *)buf, size);
#else
	int err;
	int wlen = NVRAM_STORAGE_WRITE_SEGMENT_SIZE;

	while (size > 0) {
		if (size < NVRAM_STORAGE_WRITE_SEGMENT_SIZE)
			wlen = size;

		err = flash_write(dev, addr, buf, wlen);
		if (err < 0) {
			LOG_ERR("write error %d, addr 0x%llx, buf %p, size %d",
				err, addr, buf, size);
			return -EIO;
		}

		addr += wlen;
		buf = (void*)((uint8_t *)buf + wlen);
		size -= wlen;
	}
#endif
	return 0;
#endif
}

int nvram_storage_read(struct device *dev, uint64_t addr,
				     void *buf, int32_t size)
{
	LOG_DBG("read addr 0x%llx len 0x%x", addr, size);

#ifdef CONFIG_NVRAM_STORAGE_RAMSIM
	memcpy(buf, nvram_mem_buf + addr - NVRAM_MEM_OFFS, size);
	return 0;
#else
#if  defined(CONFIG_BOARD_NANDBOOT) || defined(CONFIG_BOARD_EMMCBOOT)
		_nand_storage_read(dev, addr, (uint8_t *)buf, size);
#else
	int err;
	int rlen = NVRAM_STORAGE_READ_SEGMENT_SIZE;

	while (size > 0) {
		if (size < NVRAM_STORAGE_READ_SEGMENT_SIZE)
			rlen = size;

		err = flash_read(dev, addr, buf, rlen);
		if (err < 0) {
			LOG_ERR("read error %d, addr 0x%llx, buf %p, size %d", err, addr, buf, size);
			return -EIO;
		}

		addr += rlen;
		buf = (void*)((uint8_t *)buf + rlen);
		size -= rlen;
	}
#endif
	return 0;
#endif
}

int nvram_storage_erase(struct device *dev, uint64_t addr,
				      int32_t size)
{
	LOG_INF("erase addr 0x%llx size 0x%x", addr, size);

#ifdef CONFIG_NVRAM_STORAGE_RAMSIM
	memset(nvram_mem_buf + addr - NVRAM_MEM_OFFS, 0xff, size);
	return 0;
#else
#if  defined(CONFIG_BOARD_NANDBOOT) || defined(CONFIG_BOARD_EMMCBOOT)

	return _nand_storage_erase(dev, addr, size);
#else
	return flash_erase(dev, (off_t)addr, (size_t)size);
#endif
#endif
}

struct device *nvram_storage_init(void)
{
	struct device *dev;

	printk("init nvram storage\n");

#ifdef CONFIG_NVRAM_STORAGE_RAMSIM
	LOG_INF("init simulated nvram storage: addr 0x%x size 0x%x",
		    NVRAM_MEM_OFFS, NVRAM_MEM_SIZE);

	// memset(nvram_mem_buf, 0xff, NVRAM_MEM_SIZE);

	/* dummy device pointer */
	dev = (struct device *)-1;
#else


	#if  defined(CONFIG_BOARD_NANDBOOT) 
	#define FLASH_DEV_NAME "spinand"
	_nv_nand_cache_init();
	#elif defined(CONFIG_BOARD_EMMCBOOT)
	#define FLASH_DEV_NAME CONFIG_SD_NAME
	_nv_nand_cache_init();
	#else
	#define FLASH_DEV_NAME CONFIG_SPI_FLASH_NAME
	#endif
	dev = (struct device *)device_get_binding(FLASH_DEV_NAME);
	if (!dev) {
		LOG_ERR("cannot found device \'%s\'",
			    FLASH_DEV_NAME);
		return NULL;
	}
#endif
	return dev;
}
