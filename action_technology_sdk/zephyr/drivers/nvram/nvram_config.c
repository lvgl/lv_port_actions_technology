/*
 * Copyright (c) 2017 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Non-volatile memory driver
 */
#ifndef PC
#include <errno.h>
#include <sys/__assert.h>
#include <stdbool.h>
#include <kernel.h>
#include <device.h>
#include <init.h>
#include <drivers/nvram_config.h>
#include <string.h>
#include <storage/flash_map.h>
#include <partition/partition.h>
#include "nvram_storage.h"

#define LOG_LEVEL 2
#include <logging/log.h>
LOG_MODULE_REGISTER(nvram_cfg);
#else
#include "nvram_test_simulate.h"
#include <ffs.h>
#endif

#define CONFIG_NVRAM_FAST_SEARCH

#define NVRAM_REGION_SEG_VERSION	0x1

/* name size + item size <= buffer size */
#define NVRAM_ITEM_ALIGN_SIZE		0x10
#define NVRAM_ERASE_ALIGN_SIZE		0x1000
#define NVRAM_BUFFER_SIZE			128

enum {
	ITEM_STATUS_EMPTY = 0,
	ITEM_STATUS_VALID,
	ITEM_STATUS_OBSOLETE,
	ITEM_STATUS_INVALID,
	ITEM_STATUS_CRCERR,
};

struct __packed region_seg_header
{
	uint32_t magic;
	uint8_t state;
	uint8_t crc;
	uint8_t version;
	uint8_t reserved;
	uint8_t seq_id;
	uint8_t head_size;
	uint16_t seg_size;
	uint8_t reserved2[4];
};

struct __packed nvram_item {
	uint8_t magic;
	uint8_t state;
	uint8_t crc;
	uint8_t hash;
	uint8_t reserved;
	uint8_t name_size;
	uint16_t data_size;
	char data[0];
};

struct region_info
{
	struct device *storage;

	char name[16];
	uint32_t flag;

	/* write regions base address */
	uint32_t base_addr;
	/* total write regions size, aligned to erase size */
	int32_t total_size;

	/* current segment info */
	int32_t seg_size;
	uint32_t seg_offset;
	uint32_t seg_write_offset;
	uint8_t seg_seq_id;

#ifdef CONFIG_NVRAM_FAST_SEARCH
	uint32_t *seg_item_map;
	int seg_item_map_size;
#endif
};

/* region segment magic: 'NVRS' */
#define NVRAM_REGION_SEG_MAGIC			0x5253564E
/* region item magic: 'I' */
#define NVRAM_REGION_ITEM_MAGIC			0x49

#define NVRAM_REGION_SEG_HEADER_CRC_OFFSET	6
#define NVRAM_REGION_ITEM_CRC_OFFSET		3


#define NVRAM_REGION_SEG_STATE_VALID		0xff
#define NVRAM_REGION_SEG_STATE_OBSOLETE		0x5a

#define NVRAM_ITEM_STATE_VALID			0xff
#define NVRAM_ITEM_STATE_OBSOLETE		0x5a

#define NVRAM_SEG_ITEM_START_OFFSET	(ROUND_UP(sizeof(struct region_seg_header), NVRAM_ITEM_ALIGN_SIZE))


#ifdef CONFIG_NVRAM_FAST_SEARCH
uint32_t user_region_item_map[CONFIG_NVRAM_USER_REGION_SEGMENT_SIZE / NVRAM_ITEM_ALIGN_SIZE / 32];
#endif

/* user config region */
struct region_info user_nvram_region = {
	.name = "User Config",
	.base_addr = 0, /*init by nvram_config_init, load from partition table*/
	.total_size = 0, /*init by nvram_config_init, load from partition table*/

	.seg_size = CONFIG_NVRAM_USER_REGION_SEGMENT_SIZE,
#ifdef CONFIG_NVRAM_FAST_SEARCH
	.seg_item_map = user_region_item_map,
	.seg_item_map_size = sizeof(user_region_item_map),
#endif
};

/* factory config region */
struct region_info factory_nvram_region = {
	.name = "Factory Config",
	.base_addr = 0, /*init by nvram_config_init, load from partition table*/
	.total_size = 0, /*init by nvram_config_init, load from partition table*/
	.seg_size = CONFIG_NVRAM_FACTORY_REGION_SEGMENT_SIZE,
#ifdef CONFIG_NVRAM_FAST_SEARCH
	.seg_item_map = NULL,
	.seg_item_map_size = 0,
#endif
};

#ifdef CONFIG_NVRAM_STORAGE_FACTORY_RW_REGION
/* factory writeable config region
 * used to store machine specific configs, like SN,MAC...
 */
struct region_info factory_rw_nvram_region = {
	.name = "Factory RW",
	.base_addr = 0, /*init by nvram_config_init, load from partition table*/
	.total_size = 0, /*init by nvram_config_init, load from partition table*/
	.seg_size = CONFIG_NVRAM_FACTORY_RW_REGION_SEGMENT_SIZE,
#ifdef CONFIG_NVRAM_FAST_SEARCH
	.seg_item_map = NULL,
	.seg_item_map_size = 0,
#endif
};
#endif

static uint8_t __act_s2_notsave nvram_buf[NVRAM_BUFFER_SIZE];

#ifndef PC
static K_SEM_DEFINE(nvram_lock, 1, 1);
#else
int32_t nvram_lock;
#endif

static int region_read(struct region_info *region, uint32_t offset, uint8_t *buf, int len);
static int region_write(struct region_info *region, uint32_t offset, const uint8_t *buf, int len);
static int region_erase(struct region_info *region, uint32_t offset, int size);
static int region_copy(struct region_info *region, uint32_t src_offset, uint32_t dest_offset, int len);
static int region_is_empy(struct region_info *region, uint32_t offset, int32_t size);

static uint8_t calc_hash(const uint8_t *key, int len)
{
	uint8_t hash = 0;

	while(len--)
		hash += *key++;

	return (hash ^ 0xa5);
}

/*
 * Name:    CRC-8/MAXIM         x8+x5+x4+1
 * Poly:    0x31
 * Init:    0x00
 * Refin:   True
 * Refout:  True
 * Xorout:  0x00
 * Alias:   DOW-CRC,CRC-8/IBUTTON
 * Use:     Maxim(Dallas)'s some devices,e.g. DS18B20
 */
static const unsigned char crc8_table[32] = {
    0x00, 0x5E, 0xBC, 0xE2, 0x61, 0x3F, 0xDD, 0x83,
    0xC2, 0x9C, 0x7E, 0x20, 0xA3, 0xFD, 0x1F, 0x41,
    0x00, 0x9D, 0x23, 0xBE, 0x46, 0xDB, 0x65, 0xF8,
    0x8C, 0x11, 0xAF, 0x32, 0xCA, 0x57, 0xE9, 0x74
};

static unsigned char calc_crc8(const unsigned char *addr, int len, uint8_t initial_crc)
{
    unsigned char crc = initial_crc;

    while (len--) {
        crc = *addr++ ^ crc;
        crc = crc8_table[crc & 0x0f] ^ crc8_table[16 + ((crc >> 4) & 0x0f)];
    }

    return crc;
}

static int region_read(struct region_info *region, uint32_t offset, uint8_t *buf, int len)
{
	LOG_DBG("offset 0x%x len 0x%x, buf 0x%x", offset, len, (uint32_t)buf);

	if (!region->storage || (offset + len > region->total_size)) {
		LOG_ERR("invalid param storage %p, offset 0x%x, len 0x%x",
			region->storage, offset, len);
		return -EINVAL;
	}

	return nvram_storage_read(region->storage, region->base_addr + offset,
		buf, len);
}

static int region_write(struct region_info *region, uint32_t offset,
			      const uint8_t *buf, int len)
{
	LOG_DBG("offset 0x%x len 0x%x, buf 0x%x", offset, len, (uint32_t)buf);

	if (!region->storage || (offset + len > region->total_size)) {
		LOG_ERR("invalid param storage %p, offset 0x%x, len 0x%x",
			region->storage, offset, len);
		return -EINVAL;
	}

	return nvram_storage_write(region->storage, region->base_addr + offset,
		buf, len);
}

static int region_copy(struct region_info *region, uint32_t src_offset,
	uint32_t dest_offset, int len)
{
	int err, write_size;
	uint8_t *buf;

	LOG_DBG("src_offset 0x%x dest_offset 0x%x len 0x%x", src_offset, dest_offset, len);

	buf = nvram_buf;
	write_size = NVRAM_BUFFER_SIZE;

	while (len > 0) {
		if (len < write_size)
			write_size = len;

		err = region_read(region, src_offset, buf, write_size);
		if (err) {
			return err;
		}

		region_write(region, dest_offset, buf, write_size);
		if (err) {
			return err;
		}

		src_offset += write_size;
		dest_offset += write_size;
		len -= write_size;
	}

	return 0;
}

static int region_is_empy(struct region_info *region, uint32_t offset, int32_t size)
{
	int32_t read_size, i;
	uint32_t *pdat;

	read_size = NVRAM_BUFFER_SIZE;
	while (size > 0) {
		if (size < read_size)
			read_size = size;
		nvram_storage_read(region->storage, region->base_addr + offset,
			nvram_buf, read_size);

		pdat = (uint32_t *)nvram_buf;
		for (i = 0; i < read_size / 4; i++) {
			if (*pdat++ != 0xffffffff)
				return false;
		}

		size -= read_size;
		offset += read_size;
	}

	return true;
}

static int region_erase(struct region_info *region, uint32_t offset, int size)
{
	LOG_DBG("offset 0x%x len 0x%x", offset, size);

	if (!region->storage || (offset + size > region->total_size) ||
		(offset & (NVRAM_ERASE_ALIGN_SIZE - 1) ||
		size & (NVRAM_ERASE_ALIGN_SIZE - 1))) {
		LOG_ERR("invalid param storage %p, offset 0x%x, len 0x%x",
			region->storage, offset, size);
		return -EINVAL;
	}

	while (size > 0) {
		if (!region_is_empy(region, offset, NVRAM_ERASE_ALIGN_SIZE)) {
			nvram_storage_erase(region->storage, region->base_addr + offset,
				NVRAM_ERASE_ALIGN_SIZE);
		}

		offset += NVRAM_ERASE_ALIGN_SIZE;
		size -= NVRAM_ERASE_ALIGN_SIZE;
	}

	return 0;
}

static int region_clear(struct region_info *region, int clear_size)
{
	uint32_t offset, erase_size;

	clear_size = ROUND_UP(clear_size, region->seg_size);

	if (clear_size > (region->total_size - region->seg_size)) {
		LOG_ERR("clear_size 0x%x is too large", clear_size);
		clear_size = (region->total_size - region->seg_size);
	}

	offset = region->seg_offset + region->seg_size;

	if ((offset + clear_size) > region->total_size) {
		erase_size = region->total_size - offset;
		region_erase(region, offset, erase_size);

		/* wrap to region start pos */
		offset = 0;
		clear_size -= erase_size;
	}

	region_erase(region, offset, clear_size);

	return 0;
}

#ifdef CONFIG_NVRAM_FAST_SEARCH
//#define BITS_PER_LONG		32
#define BITOP_WORD(nr)		((nr) / BITS_PER_LONG)


/*
 * Find the first set bit in a memory region.
 */
static unsigned int find_first_bit(const unsigned int *addr, unsigned int size)
{
	const unsigned int *p = addr;
	unsigned int result = 0;
	unsigned int tmp;

	while (size & ~(BITS_PER_LONG - 1)) {
		if ((tmp = *(p++)))
			goto found;
		result += BITS_PER_LONG;
		size -= BITS_PER_LONG;
	}
	if (!size)
		return result;

	tmp = (*p) & (~0UL >> (BITS_PER_LONG - size));
	if (tmp == 0UL)		/* Are any bits set? */
		return result + size;	/* Nope. */
found:
	return result + (find_lsb_set(tmp) - 1);
}

/*
 * Find the next set bit in a memory region.
 */
static unsigned int find_next_bit(const unsigned int *addr, unsigned int size,
	unsigned int offset)
{
	const unsigned int *p = addr + BITOP_WORD(offset);
	unsigned int result = offset & ~(BITS_PER_LONG - 1);
	unsigned int tmp;

	if (offset >= size)
		return size;
	size -= result;
	offset %= BITS_PER_LONG;
	if (offset) {
		tmp = *(p++);
		tmp &= (~0UL << offset);
		if (size < BITS_PER_LONG)
			goto found_first;
		if (tmp)
			goto found_middle;
		size -= BITS_PER_LONG;
		result += BITS_PER_LONG;
	}
	while (size & ~(BITS_PER_LONG - 1)) {
		if ((tmp = *(p++)))
			goto found_middle;
		result += BITS_PER_LONG;
		size -= BITS_PER_LONG;
	}
	if (!size)
		return result;
	tmp = *p;

found_first:
	tmp &= (~0UL >> (BITS_PER_LONG - size));
	if (tmp == 0UL)		/* Are any bits set? */
		return result + size;	/* Nope. */
found_middle:
	return result + (find_lsb_set(tmp) - 1);
}

#define ITEM_BITMAP_BIT_TO_OFFSET(bit)		((bit) * NVRAM_ITEM_ALIGN_SIZE)
#define ITEM_BITMAP_OFFSET_TO_BIT(offset)	((offset) / NVRAM_ITEM_ALIGN_SIZE)

static int item_bitmap_first_offset(const unsigned int *seg_item_map, int seg_size)
{
	int bit;

	if (!seg_item_map)
		return NVRAM_SEG_ITEM_START_OFFSET;

	bit = find_first_bit(seg_item_map, ITEM_BITMAP_OFFSET_TO_BIT(seg_size));

	return (ITEM_BITMAP_BIT_TO_OFFSET(bit));
}

static int item_bitmap_next_offset(const unsigned int *seg_item_map, int seg_size, int offset)
{
	int bit;

	if (!seg_item_map)
		return offset;

	bit = find_next_bit(seg_item_map, ITEM_BITMAP_OFFSET_TO_BIT(seg_size),
		ITEM_BITMAP_OFFSET_TO_BIT(offset));

	return (ITEM_BITMAP_BIT_TO_OFFSET(bit));
}

static void item_bitmap_update(unsigned int *seg_item_map, int offset, int is_set)
{
	int bit, w, b;

	if (!seg_item_map)
		return;

	bit = ITEM_BITMAP_OFFSET_TO_BIT(offset);

	w = bit / 32;
	b = bit % 32;

	if (is_set)
		seg_item_map[w] |= 1u << b;
	else
		seg_item_map[w] &= ~(1u << b);
}

static void item_bitmap_clear_all(unsigned int *seg_item_map, int item_map_size)
{
	if (!seg_item_map)
		return;

	memset(seg_item_map, 0, item_map_size);
}
#endif


static int item_is_empty(struct nvram_item *item)
{
	uint8_t *buf = (uint8_t *)item;
	int size = sizeof(struct nvram_item);

	while (size > 0) {
		if (*buf++ != 0xff)
			return false;

		size--;
	}

	return true;
}

static int item_calc_real_size(uint8_t name_size, uint16_t data_size)
{
	return (sizeof(struct nvram_item) + name_size + data_size);
}

static int item_calc_aligned_size(uint8_t name_size, uint16_t data_size)
{
	return ROUND_UP(item_calc_real_size(name_size, data_size), NVRAM_ITEM_ALIGN_SIZE);
}

static int item_get_real_size(struct nvram_item *item)
{
	return item_calc_real_size(item->name_size, item->data_size);
}

static int item_get_aligned_size(struct nvram_item *item)
{
	return item_calc_aligned_size(item->name_size, item->data_size);
}

static void item_update_state(struct region_info *region, int item_offset, uint8_t state)
{
	int state_offs;

	state_offs = item_offset + (int)(&((struct nvram_item *)0)->state);
	LOG_DBG("set state_offs 0x%x state to 0x%x", state_offs, state);

	region_write(region, state_offs, &state, sizeof(state));
}

static uint8_t item_calc_data_crc(struct region_info *region, uint32_t offset, int32_t size, uint8_t crc)
{
	int32_t read_size;

	read_size = NVRAM_BUFFER_SIZE;

	while (size > 0) {
		if (size < read_size)
			read_size = size;

		region_read(region, offset, nvram_buf, read_size);

		crc = calc_crc8(nvram_buf, read_size, crc);

		offset += read_size;
		size -= read_size;
	}

	return crc;
}

static int item_get_item_name(struct region_info *region, int item_offs, struct nvram_item *item, uint8_t *buf, uint8_t len)
{
	uint32_t name_offset;

	if (!buf){
        return -1;
	}

	if (item->magic != NVRAM_REGION_ITEM_MAGIC) {
		if (!item_is_empty(item)) {
			LOG_ERR("invalid item maigc 0x%x", item->magic);
			return -1;
		}

		return -1;
	}

	if (item->state != NVRAM_ITEM_STATE_VALID) {
		if (item->state != NVRAM_ITEM_STATE_OBSOLETE) {
			LOG_ERR("invalid item status 0x%x", item->state);
			return -1;
		}

		return -1;
	}

	if (len < item->name_size){
        return -1;
	}else{
        len = item->name_size;
	}

	name_offset = item_offs + sizeof(struct nvram_item);
	/* check config name */
	region_read(region, name_offset, buf, len);

    return 0;
}

static int item_check_validity(struct region_info *region, int item_offs, struct nvram_item *item,
			       int check_crc)
{
	uint8_t name_hash, crc;
	uint32_t name_offset;

	if (item->magic != NVRAM_REGION_ITEM_MAGIC) {
		if (!item_is_empty(item)) {
			LOG_ERR("invalid item maigc 0x%x", item->magic);
			return ITEM_STATUS_INVALID;
		}

		return ITEM_STATUS_EMPTY;
	}

	if (item->state != NVRAM_ITEM_STATE_VALID) {
		if (item->state != NVRAM_ITEM_STATE_OBSOLETE) {
			LOG_ERR("invalid item status 0x%x", item->state);
			return ITEM_STATUS_INVALID;
		}

		return ITEM_STATUS_OBSOLETE;
	}

	name_offset = item_offs + sizeof(struct nvram_item);
	/* check config name */
	region_read(region, name_offset, nvram_buf, item->name_size);

	name_hash = calc_hash(nvram_buf, item->name_size);
	if (item->hash != name_hash) {
		return ITEM_STATUS_INVALID;
	}

	if (check_crc) {
		crc = calc_crc8((uint8_t *)item + NVRAM_REGION_ITEM_CRC_OFFSET,
			sizeof(struct nvram_item) - NVRAM_REGION_ITEM_CRC_OFFSET, 0);
		crc = calc_crc8(nvram_buf, item->name_size, crc);
		crc = item_calc_data_crc(region, name_offset + item->name_size, item->data_size, crc);

		if (item->crc != crc) {
			LOG_ERR("item crc error! offset 0x%x, crc 0x%x != item->crc 0x%x",
				item_offs, crc, item->crc);

			item_update_state(region, item_offs, NVRAM_ITEM_STATE_OBSOLETE);
			return ITEM_STATUS_INVALID;
		}
	}

	return ITEM_STATUS_VALID;
}

static int region_find_item(struct region_info *region, const char *name,
				struct nvram_item *item)
{
	uint32_t item_offs;
	uint16_t hash;
	int32_t offs;

	if (!name || !item)
		return -EINVAL;

	hash = calc_hash((const uint8_t*)name, strlen(name) + 1);

#ifdef CONFIG_NVRAM_FAST_SEARCH
	offs = item_bitmap_first_offset(region->seg_item_map, region->seg_size);
#else
	offs = NVRAM_SEG_ITEM_START_OFFSET;
#endif
	while (offs < region->seg_size) {

		item_offs = region->seg_offset + offs;

		/* read item header */
		region_read(region, item_offs, (uint8_t *)item, sizeof(struct nvram_item));

		if (item->magic != NVRAM_REGION_ITEM_MAGIC)
			break;

		if (item->state != NVRAM_ITEM_STATE_VALID) {
			goto next;
		}

		if (item->hash == hash) {
			/* read config name */
			region_read(region, item_offs + sizeof(struct nvram_item),
				nvram_buf, item->name_size);

			if (!memcmp(name, (const char *)nvram_buf, item->name_size)) {
				/* TODO: check data crc? */

				/* founded! */
				return item_offs;
			}
		}
next:
		offs += item_get_aligned_size(item);

#ifdef CONFIG_NVRAM_FAST_SEARCH
		offs = item_bitmap_next_offset(region->seg_item_map,
					       region->seg_size, offs);
#endif
	}

	return -ENOENT;
}

static void region_seg_update_state(struct region_info *region, uint32_t seg_offset, uint8_t state)
{
	int state_offs;

	state_offs = seg_offset + (int)(&((struct region_seg_header *)0)->state);
	LOG_DBG("set seg_offset 0x%x state to 0x%x", region->seg_offset, state);

	region_write(region, state_offs, &state, sizeof(state));
}

static int region_write_data(struct region_info *region, uint32_t offset,
	const uint8_t *data, int32_t len)
{
	int32_t size, wsize;
	uint32_t woffs;

	wsize = NVRAM_BUFFER_SIZE;
	woffs = 0;
	size = len;

	while (size > 0) {
		if (size < wsize)
			wsize = size;

		memcpy(nvram_buf, data + woffs, wsize);

		region_write(region, offset + woffs, nvram_buf, wsize);

		woffs += wsize;
		size -= wsize;
	}

	return 0;
}

static int region_write_item(struct region_info *region, uint32_t offset,
	const char *name, const uint8_t *data, int32_t len)
{
	struct nvram_item *item;
	uint32_t item_data_offs, item_size;

	item = (struct nvram_item *)nvram_buf;

	memset(item, 0x0, sizeof(struct nvram_item));
	item->magic = NVRAM_REGION_ITEM_MAGIC;
	item->state = NVRAM_ITEM_STATE_VALID;
	item->name_size = strlen(name) + 1;
	item->hash = calc_hash((const uint8_t*)name, item->name_size);
	item->data_size = (uint16_t)len;

	item_data_offs = sizeof(struct nvram_item) + item->name_size;
	item_size = item_data_offs + item->data_size;

	item->crc = calc_crc8((uint8_t *)item + NVRAM_REGION_ITEM_CRC_OFFSET,
		sizeof(struct nvram_item) - NVRAM_REGION_ITEM_CRC_OFFSET, 0);
	item->crc = calc_crc8((uint8_t *)name, item->name_size, item->crc);
	item->crc = calc_crc8(data, item->data_size, item->crc);

	/* write item header & name */
	if (item_data_offs > NVRAM_BUFFER_SIZE) {
		LOG_ERR("BUG! invalid item name size %d", item->name_size);
		return -EINVAL;
	}

	memcpy(&item->data[0], name, item->name_size);
	region_write(region, offset, (uint8_t *)item, item_data_offs);

	/* write item data */
#if 0
	/* use original data buffer to write data */
	region_write(region, offset + item_data_offs, data, len);
#else
	/* use nvram buffer to write data to avoid cache-miss when access spinor mapping memory */
	region_write_data(region, offset + item_data_offs, data, len);
#endif

	return item_size;
}


int region_copy_seg(struct region_info *region, uint32_t new_seg_offset,
		    uint32_t old_seg_offset, int copy_size, int check_crc)
{
	struct nvram_item item;
	int item_offs, new_item_offs;
	int item_size, item_total_size;
	int status;

#ifdef CONFIG_NVRAM_FAST_SEARCH
	/* clear item bitmap for new segment */
	item_bitmap_clear_all(region->seg_item_map, region->seg_item_map_size);
#endif

	item_offs = old_seg_offset + NVRAM_SEG_ITEM_START_OFFSET;
	new_item_offs = new_seg_offset + NVRAM_SEG_ITEM_START_OFFSET;

	while (item_offs < (old_seg_offset + copy_size)) {
		/* read item header */
		region_read(region, item_offs, (uint8_t *)&item, sizeof(struct nvram_item));

		/* check item */
		status = item_check_validity(region, item_offs, &item, check_crc);
		LOG_DBG("item_offs: status 0x%x", status);

		if (status == ITEM_STATUS_INVALID || status == ITEM_STATUS_EMPTY) {
			/* invalid */
			LOG_ERR("BUG! invalid nvram region item 0x%x, status 0x%x",
				item_offs, status);
			break;
		}

		item_total_size = item_get_aligned_size(&item);

		if (status == ITEM_STATUS_VALID) {
			item_size = item_get_real_size(&item);
			LOG_DBG("valid item: copy from 0x%x to 0x%x, len 0x%x",
				item_offs, new_item_offs, item_size);
			region_copy(region, item_offs, new_item_offs, item_size);

#ifdef CONFIG_NVRAM_FAST_SEARCH
			item_bitmap_update(region->seg_item_map,
				new_item_offs - new_seg_offset, 1);
#endif
			new_item_offs += item_total_size;
		}

		item_offs += item_total_size;
	}

	region->seg_write_offset = new_item_offs;
	LOG_DBG("new_seg write offset 0x%x", region->seg_write_offset);

	return 0;
}

static int region_init_new_seg(struct region_info *region, uint32_t seg_offset)
{
	struct region_seg_header seg_hdr;

	memset(&seg_hdr, 0x0, sizeof(struct region_seg_header));

	seg_hdr.magic = NVRAM_REGION_SEG_MAGIC;
	seg_hdr.state = NVRAM_REGION_SEG_STATE_VALID;
	seg_hdr.version = NVRAM_REGION_SEG_VERSION;
	seg_hdr.head_size = sizeof(struct region_seg_header);
	seg_hdr.seg_size = region->seg_size;
	seg_hdr.seq_id = region->seg_seq_id + 1;
	seg_hdr.crc = calc_crc8(((uint8_t *)&seg_hdr) + NVRAM_REGION_SEG_HEADER_CRC_OFFSET,
		sizeof(struct region_seg_header) - NVRAM_REGION_SEG_HEADER_CRC_OFFSET, 0);

	region_erase(region, seg_offset, region->seg_size);
	region_write(region, seg_offset, (uint8_t *)&seg_hdr, sizeof(struct region_seg_header));

	region->seg_seq_id = seg_hdr.seq_id;
	region->seg_offset = seg_offset;
	region->seg_write_offset = region->seg_offset + NVRAM_SEG_ITEM_START_OFFSET;

#ifdef CONFIG_NVRAM_FAST_SEARCH
	item_bitmap_clear_all(region->seg_item_map, region->seg_item_map_size);
#endif

	return 0;
}

static int region_purge_seg(struct region_info *region, int check_crc)
{
	uint32_t new_seg_offset, old_seg_offset, seg_copy_size;

	LOG_DBG("purge seg offset 0x%x", region->seg_offset);

	new_seg_offset = region->seg_offset + region->seg_size;
	if (new_seg_offset >= region->total_size) {
		/* wrap to begin of region */
		new_seg_offset = 0;
	}

	/* check new seg */
	if (!region_is_empy(region, new_seg_offset, region->seg_size)) {
		/* sorry, must erase in this context */
		LOG_WRN("new region seg 0x%x is not empty, need erase", new_seg_offset);
		region_erase(region, new_seg_offset, region->seg_size);
	}

	old_seg_offset = region->seg_offset;
	seg_copy_size = region->seg_write_offset - region->seg_offset;

	/* init new seg */
	region_init_new_seg(region, new_seg_offset);

	/* copy valid data to new seg */
	region_copy_seg(region, new_seg_offset, old_seg_offset, seg_copy_size, check_crc);

	/* current seg set to obsolete */
	region_seg_update_state(region, old_seg_offset, NVRAM_REGION_SEG_STATE_OBSOLETE);

	return 0;
}

static int region_prepare_write_item(struct region_info *region, int item_size)
{
	if ((region->seg_write_offset + item_size) > (region->seg_offset + region->seg_size)) {
		region_purge_seg(region, 0);
	}

	return 0;
}

static int region_is_same_config_data(struct region_info *region, const char *name,
	const void *data, int len)
{
	struct nvram_item item;

	int old_item_offs, offset;

	int32_t pos, read_size;

	old_item_offs = region_find_item(region, name, &item);

	if(old_item_offs > 0 && len == item.data_size){
		pos = 0;
		read_size = NVRAM_BUFFER_SIZE;
		offset = old_item_offs + sizeof(struct nvram_item) + item.name_size;

		while (len > 0) {
			if (len < read_size)
				read_size = len;

			region_read(region, offset + pos, nvram_buf, read_size);
			if(memcmp(nvram_buf ,(uint8_t *)data + pos, read_size) != 0){
				return false;
			}

			pos += read_size;
			len -= read_size;
		}

		if(len == 0){
			return true;
		}
	}

	return false;

}

static int region_set(struct region_info *region, const char *name,
	const void *data, int len)
{
	struct nvram_item item;
	int32_t name_len, new_item_size, item_len;
	int old_item_offs;

	if (!name || (!data && len) || len > NVRAM_MAX_DATA_SIZE)
		return -EINVAL;

	LOG_DBG("set config '%s', len %d", name, len);

	name_len = strlen(name) + 1;
	if (name_len > NVRAM_MAX_NAME_SIZE)
		return -EINVAL;

	if (len > 0) {
		if(region_is_same_config_data(region, name, data, len)){
			return 0;
		}

		/* write new config */
		new_item_size = item_calc_aligned_size(name_len, len);

		region_prepare_write_item(region, new_item_size);

		old_item_offs = region_find_item(region, name, &item);

		item_len = region_write_item(region, region->seg_write_offset, name, data, len);
		if (item_len > new_item_size) {
			LOG_ERR("BUG! new_item_size 0x%x, write item_len 0x%x",
				new_item_size, item_len);
		}
#ifdef CONFIG_NVRAM_FAST_SEARCH
		item_bitmap_update(region->seg_item_map,
			region->seg_write_offset - region->seg_offset, 1);
#endif
		region->seg_write_offset += new_item_size;
	}
	else {
		old_item_offs = region_find_item(region, name, &item);
	}

	LOG_DBG("name %s, old_item_offs %d", name, old_item_offs);

	if (old_item_offs > 0) {
		/* set the old item state to obsolete */
		item_update_state(region, old_item_offs, NVRAM_ITEM_STATE_OBSOLETE);

#ifdef CONFIG_NVRAM_FAST_SEARCH
		item_bitmap_update(region->seg_item_map, old_item_offs - region->seg_offset, 0);
#endif
	}

	return 0;
}

static int region_get(struct region_info *region, const char *name,
	void *data, int max_len)
{
	struct nvram_item item;
	uint32_t item_offs, data_offs, len;
	uint32_t crc;

	if (!name || !data || !max_len)
		return -EINVAL;

	LOG_DBG("get config '%s', max_len %d", name, max_len);

	/* search write region firstly */
	item_offs = region_find_item(region, name, &item);
	if ((int32_t)item_offs < 0)
		return -ENOENT;
	crc = calc_crc8((u8_t *)&item + NVRAM_REGION_ITEM_CRC_OFFSET,
		sizeof(struct nvram_item) - NVRAM_REGION_ITEM_CRC_OFFSET, 0);

	crc = calc_crc8(nvram_buf, item.name_size, crc);

	crc = item_calc_data_crc(region, item_offs + sizeof(struct nvram_item) + item.name_size, item.data_size, crc);

	if (item.crc != crc) {
		LOG_ERR("item crc error! offset 0x%x, crc 0x%x != item->crc 0x%x",
			item_offs, crc, item.crc);
		return -EACCES;
	}

	data_offs = item_offs + sizeof(struct nvram_item) + item.name_size;
	if (max_len < item.data_size)
		len = max_len;
	else
		len = item.data_size;

	region_read(region, data_offs, data, len);

	return len;
}

extern void print_buffer(const struct shell *shell,const char *addr, int width,
						int count, int linelen, unsigned int disp_addr);
static void region_dump_data(const struct shell *shell, struct region_info *region, uint32_t offset, int32_t size)
{
	int32_t pos, read_size;

	pos = 0;
	read_size = NVRAM_BUFFER_SIZE;

	while (size > 0) {
		if (size < read_size)
			read_size = size;

		region_read(region, offset + pos, nvram_buf, read_size);
	#ifdef CONFIG_SHELL_DBG
		print_buffer(shell, (const char *)nvram_buf, 1, read_size, 16, pos);
	#endif
		pos += read_size;
		size -= read_size;

		/*  sleep a while to avoid print buffer overflow */
		k_busy_wait(1000);
	}
}

static void region_dump(const struct shell *shell, struct region_info *region, int detailed)
{
	struct nvram_item item;
	uint32_t offs, item_offs;
	int i;

	printk("region %s: base addr 0x%x total size 0x%x\n",
		region->name, region->base_addr, region->total_size);

	printk("region segment offs 0x%x, size 0x%x, seq_id 0x%x, write offset 0x%x\n",
		region->seg_offset, region->seg_size, region->seg_seq_id,
		region->seg_write_offset);

	if (!detailed)
		return;

	i = 0;
	offs = 0;

#ifdef CONFIG_NVRAM_FAST_SEARCH
	offs = item_bitmap_first_offset(region->seg_item_map, region->seg_size);
#else
	offs = NVRAM_SEG_ITEM_START_OFFSET;
#endif
	while (offs < region->seg_size) {
		item_offs = region->seg_offset + offs;

		/* read item header */
		region_read(region, item_offs, (uint8_t *)&item, sizeof(struct nvram_item));

		if (item.magic != NVRAM_REGION_ITEM_MAGIC)
			break;

		if (item.state != NVRAM_ITEM_STATE_VALID) {
			goto next;
		}

		printk("[%2d] config item_offs 0x%x size 0x%x data size 0x%x\n",
			i, item_offs, item.name_size, item.data_size);

		/* read config name */
		region_read(region, item_offs + sizeof(struct nvram_item),
			nvram_buf, item.name_size);

		printk("     config name: %s\n", nvram_buf);
		printk("     config data:\n");

		region_dump_data(shell, region, item_offs + sizeof(struct nvram_item) + item.name_size,
			item.data_size);
next:
		offs += item_get_aligned_size(&item);

#ifdef CONFIG_NVRAM_FAST_SEARCH
		offs = item_bitmap_next_offset(region->seg_item_map,
					       region->seg_size, offs);
#endif
		i++;
	}
}

static bool is_valid_region_seg_header(struct region_seg_header *hdr)
{
	uint32_t crc;

	/* validate magic number and state*/
	if (hdr->magic != NVRAM_REGION_SEG_MAGIC ||
	    hdr->state != NVRAM_REGION_SEG_STATE_VALID)
		return false;

	/* validate header crc */
	crc = calc_crc8((uint8_t *)hdr + NVRAM_REGION_SEG_HEADER_CRC_OFFSET,
		sizeof(struct region_seg_header) - NVRAM_REGION_SEG_HEADER_CRC_OFFSET, 0);
	if (hdr->crc != crc) {
		LOG_ERR("invalid header crc 0x%x != hdr->crc 0x%x", crc, hdr->crc);
		return false;
	}

	return true;
}

static int region_seg_scan(struct region_info *region)
{
	struct nvram_item item;
	int err, offs, item_offs, status;
	int need_purge = 0;
	uint8_t item_name[NVRAM_MAX_NAME_SIZE];
    int old_item_offs;
    struct nvram_item old_item;

#ifdef CONFIG_NVRAM_FAST_SEARCH
	item_bitmap_clear_all(region->seg_item_map, region->seg_item_map_size);
#endif

	offs = NVRAM_SEG_ITEM_START_OFFSET;
	item_offs = region->seg_offset + offs;
#ifdef PC
    nvram_count = 0;
    memset(nvram_data, 0, sizeof(nvram_data));
#endif
	while (offs < region->seg_size) {
		/* read item header */
		err = region_read(region, item_offs, (uint8_t *)&item,
				  sizeof(struct nvram_item));
		if (err) {
			LOG_ERR("read item_offs 0x%x error", item_offs);
			return err;
		}

		/* read item header */
		status = item_check_validity(region, item_offs, &item, 1);
		LOG_DBG("item_offs 0x%x: status 0x%x", item_offs, status);

		if (status == ITEM_STATUS_VALID) {
            memset(item_name, 0, sizeof(item_name));
		    if(item_get_item_name(region, item_offs, &item, item_name, sizeof(item_name)) == 0){
                old_item_offs = region_find_item(region, (const char*)item_name, &old_item);

                if(old_item_offs > 0 && (old_item_offs != item_offs)){
                    LOG_WRN("same item %s found new:0x%x old 0x%x", item_name, item_offs, old_item_offs);
                    /* set the old item state to obsolete */
                    item_update_state(region, old_item_offs, NVRAM_ITEM_STATE_OBSOLETE);

#ifdef CONFIG_NVRAM_FAST_SEARCH
                    item_bitmap_update(region->seg_item_map, old_item_offs - region->seg_offset, 0);
#endif
    		    }
    		}
#ifdef CONFIG_NVRAM_FAST_SEARCH
			item_bitmap_update(region->seg_item_map, offs, 1);
#endif
#ifdef PC
    memcpy(nvram_data[nvram_count].name, item_name, sizeof(item_name));
    nvram_data[nvram_count].name[item.name_size] = 0;
	nvram_count++;
#endif
		} else if (status == ITEM_STATUS_EMPTY) {
			break;
		} else if (status != ITEM_STATUS_OBSOLETE) {
			/* invalid */
			LOG_ERR("found invalid item, item_offs 0x%x status %d", item_offs, status);
			// need_purge = 1;
			// break;
		}

		offs += item_get_aligned_size(&item);
		item_offs = region->seg_offset + offs;
	}

	region->seg_write_offset = item_offs;
	LOG_DBG("seg_offset 0x%x, write_offset 0x%x", region->seg_offset,
		region->seg_write_offset);

	if (offs >= region->seg_size) {
		/* region is full */
		need_purge = 1;
	} else {
		if (!region_is_empy(region, item_offs, region->seg_size - offs)) {
			LOG_ERR("region seg is not clean after write_offset 0x%x, need purge",
				    item_offs);
			need_purge = 1;
		}
	}

	if (need_purge) {
		region_purge_seg(region, 1);
	}

#ifdef PC
    if (nvram_count > 0)
	{
		Nvram_ValidData_Save(region->name, region->seg_offset);
	}
#endif
	return 0;
}

static int region_scan(struct region_info *region, bool b_init_seg)
{
	struct region_seg_header hdr;
	uint32_t offs = 0;
	int err, found = 0;

	while (offs < region->total_size) {
		err = region_read(region, offs, (uint8_t *)&hdr, sizeof(struct region_seg_header));
		if (err) {
			return err;
		}

		if (is_valid_region_seg_header(&hdr) &&
		    (!found || ((int8_t)(hdr.seq_id - region->seg_seq_id)) > 0)) {
			region->seg_seq_id = hdr.seq_id;
			region->seg_offset = offs;
			found = 1;
		}

		offs += region->seg_size;
	}

	if (found) {
		LOG_DBG("found region seg offset 0x%x\n", region->seg_offset);
		err = region_seg_scan(region);
		if (err) {
			LOG_ERR("invalid region seg offset 0x%x\n", region->seg_offset);
			return -1;
		}
	}
	else {
		if(b_init_seg){
			LOG_DBG("region %s: first init\n", region->name);
			region_init_new_seg(region, 0);
		}else{
			LOG_ERR("region %s: init err\n", region->name);
		}
	}

	return 0;
}

int nvram_config_set_factory(const char *name, const void *data, int len)
{
	int ret;

	k_sem_take(&nvram_lock, K_FOREVER);
#ifdef CONFIG_NVRAM_STORAGE_FACTORY_RW_REGION
	ret = region_set(&factory_rw_nvram_region, name ,data, len);
	nvram_storage_flush(factory_rw_nvram_region.storage);
#else
	ret = region_set(&factory_nvram_region, name ,data, len);
	nvram_storage_flush(factory_nvram_region.storage);
#endif
	k_sem_give(&nvram_lock);

	return ret;
}

int nvram_config_set(const char *name, const void *data, int len)
{
	int ret;

	k_sem_take(&nvram_lock, K_FOREVER);
	ret = region_set(&user_nvram_region, name, data, len);
	nvram_storage_flush(user_nvram_region.storage);
	k_sem_give(&nvram_lock);

	return ret;
}

int nvram_config_get_factory(const char *name, void *data, int max_len)
{
	int ret;

	k_sem_take(&nvram_lock, K_FOREVER);
#ifdef CONFIG_NVRAM_STORAGE_FACTORY_RW_REGION
	ret = region_get(&factory_rw_nvram_region, name, data, max_len);
	if (ret >= 0) {
		k_sem_give(&nvram_lock);
		return ret;
	}
#endif
	ret = region_get(&factory_nvram_region, name, data, max_len);
	k_sem_give(&nvram_lock);

	return ret;
}

int nvram_config_get(const char *name, void *data, int max_len)
{
	int ret;

	k_sem_take(&nvram_lock, K_FOREVER);

	/* config priority: user > factory rw >  factory ro */
	/* search user nvram region */
	ret = region_get(&user_nvram_region, name, data, max_len);
	if (ret >= 0){
		k_sem_give(&nvram_lock);
		return ret;
	}
#ifdef CONFIG_NVRAM_STORAGE_FACTORY_RW_REGION
	ret = region_get(&factory_rw_nvram_region, name, data, max_len);
	if (ret >= 0) {
		k_sem_give(&nvram_lock);
		return ret;
	}
#endif
	/* search factory nvram region */
	ret = region_get(&factory_nvram_region, name, data, max_len);
	if (ret < 0){
		LOG_DBG("cannot found config %s", name);
	}
	k_sem_give(&nvram_lock);
	return ret;
}

void nvram_config_dump(const struct shell *shell)
{
#ifdef CONFIG_NVRAM_STORAGE_FACTORY_RW_REGION
	region_dump(shell, &factory_rw_nvram_region, 1);
#endif
	region_dump(shell, &factory_nvram_region, 1);
	region_dump(shell, &user_nvram_region, 1);
}

int nvram_config_clear(int len)
{
	int ret;
	ret = region_clear(&user_nvram_region, len);
	nvram_storage_flush(user_nvram_region.storage);
	return ret;
}

int nvram_config_clear_all(void)
{
	struct region_info *region = &user_nvram_region;

	LOG_WRN("clear all user nvram config");

	k_sem_take(&nvram_lock, K_FOREVER);

	/* erase region */
	region_erase(region, 0, region->total_size);

	/* rescan region */
	region_scan(region, true);

	nvram_storage_flush(region->storage);
	k_sem_give(&nvram_lock);

	return 0;
}

int nvram_config_defrag_user(void)
{
	struct region_info *region = &user_nvram_region;

	LOG_WRN("defrag user nvram config");

	k_sem_take(&nvram_lock, K_FOREVER);

	/* copy to first region */
	if (region->seg_offset > 0) {
		region_erase(region, 0, region->seg_size);
		region_copy(region, region->seg_offset, 0, region->seg_size);
	}
	/* erase other region */
	region_erase(region, region->seg_size, region->total_size - region->seg_size);

	/* rescan region */
	region_scan(region, true);

	nvram_storage_flush(region->storage);
	k_sem_give(&nvram_lock);

	return 0;
}

void nvram_region_info_init(void)
{
	const struct partition_entry *nvram_part;

	nvram_part = partition_get_part(PARTITION_FILE_ID_NVRAM_FACTORY);
	if (nvram_part) {
		factory_nvram_region.base_addr = nvram_part->offset;
		factory_nvram_region.total_size = nvram_part->size;
		LOG_INF("nvram fac:0x%x,0x%x", factory_nvram_region.base_addr, factory_nvram_region.total_size);
	} else {
		LOG_ERR("nvram fac partition NOT find");
	}

	nvram_part = partition_get_part(PARTITION_FILE_ID_NVRAM_USER);
	if (nvram_part) {
		user_nvram_region.base_addr = nvram_part->offset;
		user_nvram_region.total_size = nvram_part->size;
		LOG_INF("nvram user:0x%x,0x%x", user_nvram_region.base_addr, user_nvram_region.total_size);
	} else {
		LOG_ERR("nvram user partition NOT find");
	}

#ifdef CONFIG_NVRAM_STORAGE_FACTORY_RW_REGION
	nvram_part = partition_get_part(PARTITION_FILE_ID_NVRAM_FACTORY_RW);
	if (nvram_part) {
		factory_rw_nvram_region.base_addr = nvram_part->offset;
		factory_rw_nvram_region.total_size = nvram_part->size;
		LOG_INF("nvram fac rw:0x%x,0x%x", factory_rw_nvram_region.base_addr, factory_rw_nvram_region.total_size);
	} else {
		LOG_ERR("nvram fac rw partition NOT find");
	}
#endif
}


int nvram_config_init(const struct device *dev)
{
	struct device *storage;

	LOG_INF("init nvram config");
	nvram_region_info_init();

	storage = nvram_storage_init();
	if (!storage) {
		LOG_ERR("NVRAM storage driver was not found!");
		return -ENODEV;
	}

	factory_nvram_region.storage = storage;
	user_nvram_region.storage = storage;
#ifdef CONFIG_NVRAM_STORAGE_FACTORY_RW_REGION
	factory_rw_nvram_region.storage = storage;
	region_scan(&factory_rw_nvram_region, true);
#endif
	region_scan(&factory_nvram_region, false);
	region_scan(&user_nvram_region, true);

	/* clear next write region to avoid erasing in system */
	region_clear(&user_nvram_region, user_nvram_region.total_size / 2);
	nvram_storage_flush(user_nvram_region.storage);
	return 0;
}

#ifndef PC
SYS_INIT(nvram_config_init, POST_KERNEL, CONFIG_NVRAM_CONFIG_INIT_PRIORITY);
#else
static int nvram_item_data_restore(struct region_info *region, int item_offs, struct nvram_item *item,
	struct nvram_data_t *nvram_item_data)
{
	uint8_t name_hash, crc;
	uint32_t name_offset;

	if (item->magic != NVRAM_REGION_ITEM_MAGIC) {
		if (!item_is_empty(item)) {
			LOG_ERR("invalid item maigc 0x%x", item->magic);
			return ITEM_STATUS_INVALID;
		}
		return ITEM_STATUS_EMPTY;
	}

	name_offset = item_offs + sizeof(struct nvram_item);
	/* check config name */
	region_read(region, name_offset, (uint8_t*)nvram_item_data->name, item->name_size);
	nvram_item_data->name[item->name_size] = 0;

	name_hash = calc_hash((const uint8_t*)nvram_item_data->name, item->name_size);
	if (item->hash != name_hash) {
		return ITEM_STATUS_INVALID;
	}

	crc = calc_crc8((uint8_t *)item + NVRAM_REGION_ITEM_CRC_OFFSET,
		sizeof(struct nvram_item) - NVRAM_REGION_ITEM_CRC_OFFSET, 0);
	crc = calc_crc8((uint8_t *)nvram_item_data->name, item->name_size, crc);

	region_read(region, name_offset + item->name_size, nvram_item_data->data, item->data_size);
	crc = calc_crc8(nvram_item_data->data, item->data_size, crc);
	nvram_item_data->data_size = item->data_size;

	if (item->crc != crc) {
		LOG_ERR("item crc error! offset 0x%x, crc 0x%x != item->crc 0x%x",
			item_offs, crc, item->crc);

		return ITEM_STATUS_CRCERR;
	}

	return ITEM_STATUS_VALID;
}

static int nvram_section_restore(struct region_info *region)
{
	struct nvram_item item;
	int err, offs, item_offs, status, i;

    nvram_count = 0;
    i = 0;
    memset(nvram_data, 0, sizeof(nvram_data));

	offs = NVRAM_SEG_ITEM_START_OFFSET;
	item_offs = region->seg_offset + offs;

	while (offs < region->seg_size) {
		/* read item header */
		err = region_read(region, item_offs, (uint8_t *)&item,
				  sizeof(struct nvram_item));
		if (err) {
			LOG_ERR("read item_offs 0x%x error", item_offs);
			return err;
		}

		/* read item header */
		status = nvram_item_data_restore(region, item_offs, &item, &nvram_data[i]);
		LOG_DBG("item_offs 0x%x: status 0x%x", item_offs, status);
		if ((ITEM_STATUS_VALID == status) || (ITEM_STATUS_CRCERR == status))
		{
			nvram_data[i].state = status;
			i++;
		}

		offs += item_get_aligned_size(&item);
		item_offs = region->seg_offset + offs;
	}

	nvram_count = i;
	return i;
}

static bool has_valid_region_seg_header(struct region_seg_header *hdr)
{
	uint32_t crc;

	/* validate magic number and state*/
	if (hdr->magic != NVRAM_REGION_SEG_MAGIC)
		return false;

	/* validate header crc */
	crc = calc_crc8((uint8_t *)hdr + NVRAM_REGION_SEG_HEADER_CRC_OFFSET,
		sizeof(struct region_seg_header) - NVRAM_REGION_SEG_HEADER_CRC_OFFSET, 0);
	if (hdr->crc != crc) {
		LOG_ERR("invalid header crc 0x%x != hdr->crc 0x%x", crc, hdr->crc);
		return false;
	}

	return true;
}

int nvram_region_restore(struct region_info *region)
{
    struct region_seg_header hdr;
	uint32_t offs = 0;
	int err;

	while (offs < region->total_size) {
		err = region_read(region, offs, (uint8_t *)&hdr, sizeof(struct region_seg_header));
		if (err) {
			return err;
		}

		if (has_valid_region_seg_header(&hdr)) {
			region->seg_seq_id = hdr.seq_id;
			region->seg_offset = offs;
			
            nvram_section_restore(region);
			Nvram_HistoryData_Save(region->name, offs);
		}
        else {
            LOG_ERR("Invalid nvram section in 0x%x of %s", offs, region->name);
        }

		offs += region->seg_size;
	}

	return 0;
}
#endif

