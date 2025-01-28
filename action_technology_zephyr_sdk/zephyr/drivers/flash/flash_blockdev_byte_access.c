/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <drivers/flash.h>
#include <init.h>
#include <kernel.h>
#include <sys/util.h>
#include <random/rand32.h>
#include <stats/stats.h>
#include <string.h>
#include <logging/log.h>
#include <board_cfg.h>

LOG_MODULE_REGISTER(block_dev_acts, CONFIG_FLASH_LOG_LEVEL);

#define BLOCK_DEV_PAGE_SIZE 2048
#define BLOCK_DEV_SECTOR_SIZE 512
#define BLOCK_DEV_CACHE_SECT_NUM	(BLOCK_DEV_PAGE_SIZE/BLOCK_DEV_SECTOR_SIZE)
#define BLOCK_DEV_CACHE_NUM_ITEM	2

struct  block_dev_cache_item {
	u16_t cache_valid:1;
	u16_t write_valid:1;
	u16_t reserved:14;
	u16_t seq_index;

	u32_t cache_start_sector;
	u8_t  cache_data[BLOCK_DEV_PAGE_SIZE];
};

struct block_dev_flash_data{
	const struct device *flash_dev;
	u16_t g_seq_index;
	struct  block_dev_cache_item item[BLOCK_DEV_CACHE_NUM_ITEM];
};

struct block_dev_config {
	const char * dev_name;
};

#define DEV_CFG(dev) \
	((const struct block_dev_config *const)(dev)->config)
#define DEV_DATA(dev) \
	((struct block_dev_flash_data *)(dev)->data)

static void _block_dev_cache_init(struct block_dev_flash_data *data)
{
	int i;
	for(i = 0; i < BLOCK_DEV_CACHE_NUM_ITEM; i++) {
		data->item[i].cache_valid = 0;
		data->item[i].write_valid = 0;
	}
}

static int  _block_dev_flush_cache(const struct device *dev, struct block_dev_cache_item *item)
{
	int err;
	if(item->write_valid && item->cache_valid){
		item->write_valid = 0;
		LOG_DBG("nand flush startsec 0x%x, buf %p",	item->cache_start_sector, item->cache_data);
		err = flash_write(dev, (item->cache_start_sector)<<9, item->cache_data, BLOCK_DEV_CACHE_SECT_NUM<<9);
		if (err < 0) {
			LOG_ERR("nand write error %d, offsec 0x%x, buf %p",
				err, item->cache_start_sector, item->cache_data);
			return -EIO;
		}
	}
	return 0;
}
static int  _block_dev_update_cache(const struct device *dev, struct block_dev_cache_item *item, u32_t sec_off, u16_t *seq)
{
	int err;
	u32_t page_off = sec_off/BLOCK_DEV_CACHE_SECT_NUM;
	item->cache_start_sector = page_off * BLOCK_DEV_CACHE_SECT_NUM;
	item->cache_valid = 1;
	item->write_valid = 0;
	item->seq_index = *seq;
	*seq = *seq + 1;
	LOG_DBG("nand update start_sec 0x%x, buf %p, sec_off=0x%x,0x%x",
		item->cache_start_sector, item->cache_data, sec_off, sec_off*BLOCK_DEV_SECTOR_SIZE);

	err = flash_read(dev, (item->cache_start_sector)<<9,item->cache_data, BLOCK_DEV_CACHE_SECT_NUM<<9);
	if (err < 0) {
		LOG_ERR("nand read error %d, offsec 0x%x, buf %p",
			err, item->cache_start_sector, item->cache_data);
		return -EIO;
	}
	return 0;
}


static struct block_dev_cache_item * _block_dev_find_cache_item(struct block_dev_flash_data *data, u32_t sec_off)
{
	for (int i = 0; i < BLOCK_DEV_CACHE_NUM_ITEM; i++) {
		if (data->item[i].cache_valid && (data->item[i].cache_start_sector <= sec_off)
			&& (sec_off < data->item[i].cache_start_sector+BLOCK_DEV_CACHE_SECT_NUM)) {
				return &data->item[i];
		}
	}
	//LOG_INF("sect_off 0x%x not in cache", sec_off);
	return NULL;
}

/*
	if have invaid ,use invalid, else use same
*/
static struct block_dev_cache_item * _block_dev_new_cache_item(struct block_dev_flash_data *data, u32_t sec_off)
{
	int i;
	struct block_dev_cache_item *invalid, *valid, *new_item;

	valid = invalid = NULL;
	for(i = 0; i < BLOCK_DEV_CACHE_NUM_ITEM; i++) {
		if(data->item[i].cache_valid){
			if(valid == NULL){
				valid = &data->item[i];
			}else{
				if(valid->seq_index > data->item[i].seq_index)
					valid = &data->item[i];
			}
		}else{
			invalid = &data->item[i];
			break;
		}
	}
	if(invalid){
		new_item = invalid;
	}else{
		new_item = valid;
	}
	//LOG_INF("new item sect_off 0x%x, 0x%x is_write=%d", sec_off, sec_off*BLOCK_DEV_SECTOR_SIZE, is_write);
	_block_dev_flush_cache(data->flash_dev, new_item);
	_block_dev_update_cache(data->flash_dev, new_item, sec_off, &data->g_seq_index);
	return new_item;

}

#ifndef CONFIG_BOARD_NANDBOOT
#if 0
void nvram_storage_flush(struct block_dev_flash_data *data)
{
	int i;
	for(i = 0; i < BLOCK_DEV_CACHE_NUM_ITEM; i++)
		_block_dev_flush_cache(data->flash_dev, &data->item[i]);
}
#endif
#endif

/*brwe = 0 (read), 1(write), 2(erase)*/
static void block_dev_storage_read_write(struct block_dev_flash_data *data, uint64_t addr,
			uint8_t *buf, uint64_t size, int brwe)
{
	int32_t wsize, len;
	uint32_t sector_off, addr_in_sec,cache_sec_index;
	static struct block_dev_cache_item * item;

	wsize = size;
	sector_off = addr / BLOCK_DEV_SECTOR_SIZE;
	addr_in_sec = addr - (sector_off*BLOCK_DEV_SECTOR_SIZE);
	//LOG_INF("sect_off 0x%x, size 0x%x iswrite %d", sector_off, size, brwe);
	while(wsize > 0) {
		item = _block_dev_find_cache_item(data, sector_off);
		if(item == NULL)
			item = _block_dev_new_cache_item(data, sector_off);
		cache_sec_index = sector_off -item->cache_start_sector;
		if(addr_in_sec){//not sector algin
			len = BLOCK_DEV_SECTOR_SIZE - addr_in_sec;
			if(len > wsize)
				len = wsize;
			if(brwe == 1)
				memcpy(&item->cache_data[cache_sec_index*BLOCK_DEV_SECTOR_SIZE+addr_in_sec],buf, len);
			else if(brwe == 0)
				memcpy(buf, &item->cache_data[cache_sec_index*BLOCK_DEV_SECTOR_SIZE+addr_in_sec], len);
			else
				memset(&item->cache_data[cache_sec_index*BLOCK_DEV_SECTOR_SIZE+addr_in_sec], 0xff, len);
			buf += len;
			wsize -= len;
			cache_sec_index++;
			addr_in_sec = 0; // next align
		}
		for( ; wsize && (cache_sec_index < BLOCK_DEV_CACHE_SECT_NUM);  cache_sec_index++){
			if(wsize < BLOCK_DEV_SECTOR_SIZE)
				len = wsize;
			else
				len = BLOCK_DEV_SECTOR_SIZE;

			if(brwe == 1)
				memcpy(&item->cache_data[cache_sec_index*BLOCK_DEV_SECTOR_SIZE],buf, len);
			else if(brwe == 0)
				memcpy(buf, &item->cache_data[cache_sec_index*BLOCK_DEV_SECTOR_SIZE], len);
			else
				memset(&item->cache_data[cache_sec_index*BLOCK_DEV_SECTOR_SIZE], 0xff, len);
			buf += len;
			wsize -= len;
		}
		if(brwe)
			item->write_valid = 1; //dirty

		sector_off = item->cache_start_sector + BLOCK_DEV_CACHE_SECT_NUM;

	}

}

static int block_dev_storage_write(const struct device *dev, uint64_t offset,
			      const void *data,
			      uint64_t len)
{
	struct block_dev_flash_data *dev_data = DEV_DATA(dev);

	block_dev_storage_read_write(dev_data, offset, (uint8_t *)data, len, 1);
	//LOG_INF("nand write end 0x%x len 0x%x", addr, size);
	return 0;
}


static int block_dev_storage_read(const struct device *dev, uint64_t offset,
			      void *data,
			      uint64_t len)
{
	struct block_dev_flash_data *dev_data = DEV_DATA(dev);

	block_dev_storage_read_write(dev_data, offset, (uint8_t *)data, len, 0);
	//LOG_INF("nand read end 0x%x len 0x%x", addr, size);
	return 0;

}

static int block_dev_storage_erase(const struct device *dev, uint64_t offset,
			       uint64_t size)
{
	uint8_t buf[1]; // not use;
	struct block_dev_flash_data *dev_data = DEV_DATA(dev);

	block_dev_storage_read_write(dev_data, offset, buf, size, 2);
	//LOG_INF("nand erase end 0x%x len 0x%x", addr, size);
	return 0;
}


static const struct flash_parameters flash_blockdev_parameters = {
	.write_block_size = 0x1000,
	.erase_value = 0xff,
};

static const struct flash_parameters *
block_dev_storage_get_parameters(const struct device *dev)
{
	ARG_UNUSED(dev);

	return &flash_blockdev_parameters;
}

static const struct flash_driver_api flash_block_dev_api = {
	.read = block_dev_storage_read,
	.write = block_dev_storage_write,
	.erase = block_dev_storage_erase,
	.get_parameters = block_dev_storage_get_parameters,
};

int block_dev_pm_control(const struct device *dev, enum pm_device_action action)
{
#ifdef CONFIG_PM_DEVICE
    int i;
    int ret;
    int err;
    u8_t *buf;
    u32_t offset;
    struct block_dev_flash_data *data = DEV_DATA(dev);

    switch(action) {
        case PM_DEVICE_ACTION_TURN_OFF:
            for(i = 0; i < BLOCK_DEV_CACHE_NUM_ITEM; i++) {
                if (data->item[i].write_valid && data->item[i].cache_valid) {
                    data->item[i].write_valid = 0;
                    buf = data->item[i].cache_data;
                    offset = data->item[i].cache_start_sector;
                    LOG_DBG("nand flush startsec 0x%x, buf %p\n", offset, buf);
                    err = flash_write(data->flash_dev, offset<<9, buf, BLOCK_DEV_CACHE_SECT_NUM<<9);
                    if (err < 0) {
                        LOG_ERR("nand write error %d, offsec 0x%x, buf %p.\n", err, offset, buf);
                        return -EIO;
                    }
                }
            }
		#ifdef CONFIG_SPINAND_ACTS
            flash_flush(data->flash_dev, 0);
		#endif
            break;
        default:
            ret = -EINVAL;
    }
#endif
    return 0;
}

static int block_dev_flash_init(const struct device *dev)
{
	struct block_dev_flash_data *data = DEV_DATA(dev);
	const struct block_dev_config *config = DEV_CFG(dev);

	data->flash_dev = device_get_binding(config->dev_name);

	if(!data->flash_dev){
		return -EINVAL;
	}

	_block_dev_cache_init(data);

	return 0;
}

static struct block_dev_flash_data block_dev_flash_acts_data;

static const struct block_dev_config  block_dev_flash_acts_config = {
#ifdef CONFIG_SPINAND_ACTS
	.dev_name = CONFIG_SPINAND_FLASH_NAME,
#elif defined(CONFIG_MMC_ACTS)
	.dev_name = CONFIG_SD_NAME,
#endif
};
DEVICE_DEFINE(block_dev_acts, CONFIG_BLOCK_DEV_FLASH_NAME, &block_dev_flash_init, block_dev_pm_control, &block_dev_flash_acts_data,
		    &block_dev_flash_acts_config, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &flash_block_dev_api);

