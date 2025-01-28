/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file ramdump_core
 */

#include <string.h>
#include <stdio.h>
#include <linker/linker-defs.h>
#include <kernel.h>
#include <kernel_internal.h>
#include <errno.h>
#include <init.h>
#include <device.h>
#include <drivers/flash.h>
#include <drivers/rtc.h>
#include <drivers/ipmsg.h>
#include <drivers/bluetooth/bt_drv.h>
#include <partition/partition.h>
#include <board_cfg.h>
#include <crc.h>
#include <logging/log.h>
#include "ramdump_core.h"
#include "fastlz.h"

LOG_MODULE_REGISTER(ramdump, CONFIG_KERNEL_LOG_LEVEL);

typedef struct ramd_data_s {
	const struct partition_entry *part;     /* TEMP partition */
	const struct device *flash_dev;         /* flash device */
	const struct device *rtc_dev;           /* rtc device */
	const struct device *btc_dev;           /* btc device */
	char *compr_buf;                        /* compress output buffer */
	uint16_t inc_uid;                       /* unique id (auto increment) */
	uint16_t blk_off;                       /* block aligned offset */
	ramd_head_t head;                       /* ramdump header */
	ramd_region_t region;                   /* ramdump region header */
	compr_head_t compr_head;                /* compress block header */
	mcpu_dbg_t mcpu_dbg;                    /* maincpu debug info */
	btcpu_dbg_t btcpu_dbg;                  /* btcpu debug info */
	ramd_addr_t addr_dbg;                   /* debug address info */
	char flash_buf[RAMD_SECTOR_SZ];         /* flash rw buffer */
} ramd_data_t;

static ramd_data_t *ramd_pdata = NULL;

extern void cpu_trace_enable(int enable);

static int ramdump_ops_flash(ramd_data_t* ramd, int wr, uint64_t offset, uint8_t* buf, int len)
{
	uint64_t byte_off, slen;

	// first sector
	byte_off = offset & (RAMD_SECTOR_SZ - 1);
	slen = ((RAMD_SECTOR_SZ - byte_off) > len) ? len : (RAMD_SECTOR_SZ - byte_off);
	if (slen > 0) {
		flash_read(ramd->flash_dev, offset - byte_off, ramd->flash_buf, RAMD_SECTOR_SZ);
		if (wr) {
			memcpy(ramd->flash_buf + byte_off, buf, slen);
			flash_write(ramd->flash_dev, offset - byte_off, ramd->flash_buf, RAMD_SECTOR_SZ);
			LOG_DBG("write1 offset=0x%x, len=%d", offset, slen);
		} else {
			memcpy(buf, ramd->flash_buf + byte_off, slen);
			LOG_DBG("read1 offset=0x%x, len=%d", offset, slen);
		}
		offset += slen;
		buf += slen;
		len -= slen;
	}

	// aligned sectors
	slen = (len / RAMD_SECTOR_SZ) * RAMD_SECTOR_SZ;
	if (slen > 0) {
		if (wr) {
			flash_write(ramd->flash_dev, offset, buf, slen);
			LOG_DBG("write2 offset=0x%x, len=%d", offset, slen);
		} else {
			flash_read(ramd->flash_dev, offset, buf, slen);
			LOG_DBG("read2 offset=0x%x, len=%d", offset, slen);
		}
		offset += slen;
		buf += slen;
		len -= slen;
	}

	// last sector
	if (len > 0) {
		flash_read(ramd->flash_dev, offset, ramd->flash_buf, RAMD_SECTOR_SZ);
		if (wr) {
			memcpy(ramd->flash_buf, buf, len);
			flash_write(ramd->flash_dev, offset, ramd->flash_buf, RAMD_SECTOR_SZ);
			LOG_DBG("write3 offset=0x%x, len=%d", offset, len);
		} else {
			memcpy(buf, ramd->flash_buf, len);
			LOG_DBG("read3 offset=0x%x, len=%d", offset, len);
		}
	}

	return 0;
}

static int ramdump_rewind_flash(ramd_data_t* ramd, int wr, int offset, uint8_t* buf, int len)
{
	uint32_t slen;

	// check partition end
	if (offset >= ramd->part->size) {
		offset -= ramd->part->size;
	}
	if ((offset + len) > ramd->part->size) {
		// rw the first part
		slen = ramd->part->size - offset;
		ramdump_ops_flash(ramd, wr, ramd->part->offset + offset, buf, slen);

		// rewind partition
		offset = 0;
		buf += slen;
		len -= slen;
	}

	// rw the other part
	ramdump_ops_flash(ramd, wr, ramd->part->offset + offset, buf, len);

	return 0;
}

static int ramdump_read_flash(ramd_data_t* ramd, int offset, uint8_t* buf, int len)
{
	return ramdump_rewind_flash(ramd, 0, offset, buf, len);
}

static int ramdump_write_flash(ramd_data_t* ramd, int offset, uint8_t* buf, int len)
{
	return ramdump_rewind_flash(ramd, 1, offset, buf, len);
}

static int ramdump_erase_flash(ramd_data_t* ramd, int offset, int len)
{
	uint32_t blk_start, blk_end;
	int rewind_len = 0, erase_len;

	// check partition end
	if (offset >= ramd->part->size) {
		offset -= ramd->part->size;
	}
	if ((offset + len) > ramd->part->size) {
		rewind_len = offset + len - ramd->part->size;
		len -= rewind_len;
	}

	while (1) {
		blk_start = ROUND_UP(ramd->part->offset + offset, RAMD_ERASE_BLK_SZ);
		blk_end = ROUND_UP(ramd->part->offset + offset + len, RAMD_ERASE_BLK_SZ);
		if (blk_end > (ramd->part->offset + ramd->part->size)) {
			blk_end = ramd->part->offset + ramd->part->size;
		}

		// erase first unaligned part
		erase_len = blk_start - ramd->part->offset;
		if ((offset == 0) && (erase_len > 0)) {
			flash_erase(ramd->flash_dev, ramd->part->offset, erase_len);
			LOG_DBG("erase first offset=0x%x, len=%d", ramd->part->offset, erase_len);
		}

		// erase last aligned part
		erase_len = blk_end - blk_start;
		if (erase_len > 0) {
			flash_erase(ramd->flash_dev, blk_start, erase_len);
			LOG_DBG("erase last offset=0x%x, len=%d", blk_start, erase_len);
		}

		// erase rewind part
		if (rewind_len > 0) {
			offset = 0;
			len = rewind_len;
			rewind_len = 0;
		} else {
			break;
		}
	}

	return 0;
}

static ramd_data_t* ramdump_get_data(void)
{
	return ramd_pdata;
}

static uint32_t ramdump_get_header(ramd_data_t *ramd, int offset, ramd_head_t *phead, int *next)
{
	uint32_t ret = 0;

	// read ramd header
	ramdump_read_flash(ramd, offset, (uint8_t*)phead, sizeof(ramd_head_t));

	// check header magic
	if (phead->magic == MAGIC_RAMD) {
		// check header crc
		if (phead->hdr_chksum == utils_crc32(0, (const uint8_t*)phead, phead->hdr_sz - 4)) {
			ret = sizeof(ramd_head_t);
		} else {
			LOG_ERR("verify header error! (offset 0x%x)", offset);
		}
	}

	// get next offset
	if (next) {
		if (ret == 0) {
			*next = offset + RAMD_HEADER_BLK_SZ;
		} else {
			*next = offset + ROUND_UP(phead->hdr_sz + phead->img_sz, RAMD_HEADER_BLK_SZ);
		}
	}

	return ret;
}

static uint32_t ramdump_get_region(ramd_data_t *ramd, int offset, ramd_region_t *pregion)
{
	uint32_t ret = 0;

	// read ramd header
	ramdump_read_flash(ramd, offset, (uint8_t*)pregion, sizeof(ramd_region_t));

	// check header magic
	if (pregion->magic == MAGIC_RAMR) {
		// check header size of region
		if (pregion->hdr_sz == sizeof(ramd_region_t)) {
			ret = sizeof(ramd_region_t);
		} else {
			LOG_ERR("Region header error! (offset 0x%x)", offset);
		}
	}

	return ret;
}

static uint32_t ramdump_get_free(ramd_data_t *ramd)
{
#if 0
	ramd_head_t *phead = &ramd->head;
	uint32_t free_off, found = 0, offset, next, out_sz;
	uint16_t min_uid = 0xffff;

	free_off = ramd->blk_off;
	offset = ramd->blk_off;
	while (offset < ramd->part->size) {
		// find ramdump header
		out_sz = ramdump_get_header(ramd, offset, phead, &next);
		if (out_sz == sizeof(ramd_head_t)) {
			// update max uid
			if (phead->inc_uid > ramd->inc_uid) {
				ramd->inc_uid = phead->inc_uid;
			}
			// save offset of min uid
			if (phead->inc_uid < min_uid) {
				free_off = offset;
				min_uid = phead->inc_uid;
			}
			found = 1;
		} else {
			if (found) {
				free_off = offset;
				break;
			}
		}
		offset = next;
	}

	return free_off;
#else
	return ramd->blk_off;
#endif
}

static uint32_t ramdump_get_chksum(ramd_data_t* ramd, int offset, int size)
{
	uint32_t chksum = 0, rlen;

	// compute image crc
	while (size > 0) {
		if (size < RAMD_COMPR_BLK_SZ) {
			rlen = size;
		} else {
			rlen = RAMD_COMPR_BLK_SZ;
		}
		if(!k_is_in_isr()) {
			k_sched_lock();
		}

		ramdump_read_flash(ramd, offset, ramd->compr_buf, rlen);
		chksum = utils_crc32(chksum, ramd->compr_buf, rlen);

		if(!k_is_in_isr()) {
			k_sched_unlock();
		}
		size -= rlen;
		offset += rlen;
	}

	return chksum;
}

static uint32_t ramdump_get_datetime(ramd_data_t* ramd, uint8_t *buf, int len)
{
	buf[0] = '\0';

#ifdef CONFIG_RTC_ACTS
	// get date & time
	struct rtc_time tm;
	uint8_t tmp_buf[22];

	if (ramd->rtc_dev) {
		rtc_get_time(ramd->rtc_dev, &tm);
		snprintf(tmp_buf, sizeof(tmp_buf), "%04d%02d%02d-%02d%02d%02d",
			1900 + tm.tm_year, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
		if (len > sizeof(tmp_buf)) {
			len = sizeof(tmp_buf);
		}
		memcpy(buf, tmp_buf, len);
	}
#endif

	return strlen(buf);
}

static uint32_t ramdump_save_mcpu(mcpu_dbg_t *mcpu_dbg, const z_arch_esf_t *esf)
{
	const struct _callee_saved *callee = esf->extra_info.callee;

	memset(mcpu_dbg, 0, sizeof(mcpu_dbg_t));
	mcpu_dbg->r[0] = esf->basic.a1;
	mcpu_dbg->r[1] = esf->basic.a2;
	mcpu_dbg->r[2] = esf->basic.a3;
	mcpu_dbg->r[3] = esf->basic.a4;
	mcpu_dbg->r[4] = callee->v1;
	mcpu_dbg->r[5] = callee->v2;
	mcpu_dbg->r[6] = callee->v3;
	mcpu_dbg->r[7] = callee->v4;
	mcpu_dbg->r[8] = callee->v5;
	mcpu_dbg->r[9] = callee->v6;
	mcpu_dbg->r[10] = callee->v7;
	mcpu_dbg->r[11] = callee->v8;
	mcpu_dbg->r[12] = esf->basic.ip;
	// EXC_RETURN(bit2): SPSEL (0-MSP, 1-PSP)
	if (esf->extra_info.exc_return & (1 << 2)) {
		mcpu_dbg->r[13] = callee->psp;
	} else {
		mcpu_dbg->r[13] = esf->extra_info.msp;
	}
	mcpu_dbg->r[14] = esf->basic.lr;
	mcpu_dbg->r[15] = esf->basic.pc;
	mcpu_dbg->xpsr = esf->basic.xpsr;
	mcpu_dbg->msp = esf->extra_info.msp;
	mcpu_dbg->psp = mcpu_dbg->r[13];
	mcpu_dbg->exc_ret = esf->extra_info.exc_return;

	// EXC_RETURN(bit4): frame type (0-ext, 1-std)
	if (mcpu_dbg->exc_ret & (1 << 4)) {
		// correct sp without FP
		mcpu_dbg->r[13] += 0x20;
		mcpu_dbg->psp += 0x20;

		// CONTROL(bit2): FPCA=0
		mcpu_dbg->control &= ~(1 << 2);
	} else {
		// correct sp with FP
		mcpu_dbg->r[13] += 0x68;
		mcpu_dbg->psp += 0x68;

		// CONTROL(bit2): FPCA=1
		mcpu_dbg->control |= (1 << 2);
	}

	// EXC_RETURN(bit2): SPSEL (0-MSP, 1-PSP)
	if (mcpu_dbg->exc_ret & (1 << 2)) {
		// CONTROL(bit1): SPSEL=1
		mcpu_dbg->control |= (1 << 1);
	} else {
		// CONTROL(bit1): SPSEL=0
		mcpu_dbg->control &= ~(1 << 1);
	}

	return sizeof(mcpu_dbg_t);
}

static uint32_t ramdump_save_btcpu(btcpu_dbg_t *btcpu_dbg, void *bt_info)
{
	memset(btcpu_dbg, 0, sizeof(btcpu_dbg_t));
	if (bt_info) {
		memcpy(btcpu_dbg->bt_info, bt_info, sizeof(btcpu_dbg->bt_info));
	}

	return sizeof(btcpu_dbg_t);
}

static uint32_t ramdump_save_block(ramd_data_t* ramd, int offset, char* src, int ilen)
{
	compr_head_t *phead = &ramd->compr_head;
	uint32_t out_sz, byte;

	if(!k_is_in_isr()) {
		k_sched_lock();
	}

	// copy src for AHB/PPB
	if ((uint32_t)src >= RAMD_AHB_START) {
		memcpy(ramd->compr_buf + RAMD_COMPR_BLK_SZ, src, ilen);
		src = ramd->compr_buf + RAMD_COMPR_BLK_SZ;
	}

	// compress buffer
	out_sz = fastlz_compress_level(1, src, ilen, ramd->compr_buf);
	if (out_sz <= 0) {
		if(!k_is_in_isr()) {
			k_sched_unlock();
		}
		LOG_ERR("compress error!");
		return 0;
	}

	// fill header
	phead->magic = MAGIC_FLZ;
	phead->hdr_size = sizeof(compr_head_t);
	phead->org_sz = ilen;
	phead->img_sz = out_sz;

	// align output
	byte = (out_sz % 4);
	if (byte > 0) {
		memset(ramd->compr_buf + out_sz, 0, 4 - byte);
		out_sz += 4 - byte;
	}

	// erase ramd header
	ramdump_erase_flash(ramd, offset, sizeof(compr_head_t) + out_sz);

	// write flash
	ramdump_write_flash(ramd, offset, (uint8_t*)phead, sizeof(compr_head_t));
	ramdump_write_flash(ramd, offset + sizeof(compr_head_t), ramd->compr_buf, out_sz);

	if(!k_is_in_isr()) {
		k_sched_unlock();
	}

	return (sizeof(compr_head_t) + out_sz);
}

static uint32_t ramdump_save_region(ramd_data_t* ramd, int offset, uint16_t type, ramd_addr_t *addr)
{
	ramd_region_t *pregion = &ramd->region;
	uint32_t in_sz, out_sz, in_off;

	// erase ramd header
	ramdump_erase_flash(ramd, offset, sizeof(ramd_region_t));

	// fill ramd region header
	memset(pregion, 0, sizeof(ramd_region_t));
	pregion->magic = MAGIC_RAMR;
	pregion->type = type;
	pregion->hdr_sz = sizeof(ramd_region_t);
	pregion->org_sz = addr->next - addr->start;
	pregion->address = addr->start;

	// compress block and write flash
	in_off = 0;
	while (in_off < pregion->org_sz) {
		in_sz = pregion->org_sz - in_off;
		if (in_sz > RAMD_COMPR_BLK_SZ) {
			in_sz = RAMD_COMPR_BLK_SZ;
		}
		out_sz = ramdump_save_block(ramd, offset + sizeof(ramd_region_t) + pregion->img_sz,
					(char*)(pregion->address + in_off), in_sz);
		if (out_sz <= 0) {
			return 0;
		}

		// next block
		in_off += in_sz;
		pregion->img_sz += out_sz;
	}

	// write ramd region header
	ramdump_write_flash(ramd, offset, (uint8_t*)pregion, sizeof(ramd_region_t));

	LOG_INF("[region] type=%d, mem_addr=0x%08x, mem_sz=%d, img_sz=%d",
		pregion->type, pregion->address, pregion->org_sz, pregion->img_sz);

	return (sizeof(ramd_region_t) + pregion->img_sz);
}

static uint32_t ramdump_save_header(ramd_data_t* ramd, int offset, int img_sz, int org_sz)
{
	ramd_head_t *phead = &ramd->head;

	// fill ramd header
	memset(phead, 0, sizeof(ramd_head_t));
	phead->magic = MAGIC_RAMD;
	phead->version = RAMD_VERSION;
	phead->hdr_sz = sizeof(ramd_head_t);
	phead->img_sz = img_sz;
	phead->org_sz = org_sz;

	// get uid & cpuid
	ramd->inc_uid ++;
	phead->inc_uid = ramd->inc_uid;
	phead->cpu_id = ((SCB->CPUID & SCB_CPUID_PARTNO_Msk) >> SCB_CPUID_PARTNO_Pos);

	// get date & time
	ramdump_get_datetime(ramd, phead->datetime, sizeof(phead->datetime));

	// compute image crc
	phead->img_chksum = ramdump_get_chksum(ramd, offset + phead->hdr_sz, img_sz);

	// compute header crc
	phead->hdr_chksum = utils_crc32(0, (const uint8_t*)phead, phead->hdr_sz - 4);

	// write ramd header
	ramdump_write_flash(ramd, offset, (uint8_t*)phead, sizeof(ramd_head_t));

	LOG_INF("[header v%d] %s org_sz=%d, img_sz=%d, uid=%d, offset=0x%x",
		phead->version, phead->datetime, phead->org_sz, phead->img_sz, phead->inc_uid, offset);

	flash_flush(ramd->flash_dev, false);

	return (sizeof(ramd_head_t));
}

int ramdump_save(const z_arch_esf_t *esf, int btcpu_info)
{
	ramd_data_t *ramd = ramdump_get_data();
	ramd_addr_t *paddr;
	uint32_t offset, out_sz, img_sz, org_sz;
	uint32_t cycle_start, cost_ms;

	if (!ramd) {
		LOG_ERR("init failed!");
		return -1;
	}

	// disable MTB trace
#ifdef CONFIG_SOC_LEOPARD
	if (!esf) {
		cpu_trace_enable(0);
	}
#endif

	// get free offset
	offset = ramdump_get_free(ramd);
	LOG_INF("save offset 0x%x", offset);

	// erase ramd header
	ramdump_erase_flash(ramd, offset, sizeof(ramd_head_t));
	offset += sizeof(ramd_head_t);

	// save ramd regions
	img_sz = 0;
	org_sz = 0;
	cycle_start = k_cycle_get_32();

	// save maincpu register
	if (esf) {
		ramdump_save_mcpu(&ramd->mcpu_dbg, esf);

		ramd->addr_dbg.start = (uintptr_t)&ramd->mcpu_dbg;
		ramd->addr_dbg.next = ramd->addr_dbg.start + sizeof(mcpu_dbg_t);
		out_sz = ramdump_save_region(ramd, offset, TYPE_MCPU_DBG, &ramd->addr_dbg);
		if (out_sz <= 0) {
			return -2;
		}

		// next region
		org_sz += sizeof(mcpu_dbg_t);
		img_sz += out_sz;
		offset += out_sz;
	}

	// save btcpu debug info
	if (btcpu_info) {
		// stop btcpu
		if (ramd->btc_dev) {
			ipmsg_stop((struct device*)ramd->btc_dev);
		}
		ramdump_save_btcpu(&ramd->btcpu_dbg, btdrv_dump_btcpu_info());

		ramd->addr_dbg.start = (uintptr_t)&ramd->btcpu_dbg;
		ramd->addr_dbg.next = ramd->addr_dbg.start + sizeof(btcpu_dbg_t);
		out_sz = ramdump_save_region(ramd, offset, TYPE_BTCPU_DBG, &ramd->addr_dbg);
		if (out_sz <= 0) {
			return -3;
		}

		// next region
		org_sz += sizeof(btcpu_dbg_t);
		img_sz += out_sz;
		offset += out_sz;
	}

	// save memory/register with address
	paddr = (ramd_addr_t*)ramd_mem_regions;
	while ((paddr->start != 0) && (paddr->next != 0)) {
		// filter by type
		switch(paddr->filter) {
			case TYPE_MCPU_DBG:
				if (!esf) {
					paddr ++;
					continue;
				}
				break;
			case TYPE_BTCPU_DBG:
				if (!btcpu_info) {
					paddr ++;
					continue;
				}
				break;
			case TYPE_ADDR:
			default:
				break;
		}

		out_sz = ramdump_save_region(ramd, offset, paddr->filter, paddr);
		if (out_sz <= 0) {
			return -2;
		}

		// next region
		org_sz += (paddr->next - paddr->start);
		img_sz += out_sz;
		offset += out_sz;
		paddr ++;
	}

	// save ramd header
	ramdump_save_header(ramd, offset - img_sz - sizeof(ramd_head_t), img_sz, org_sz);

	cost_ms = k_cyc_to_ms_floor32(k_cycle_get_32() - cycle_start);
	LOG_INF("save cost %d ms", cost_ms);

	// enable MTB trace
#ifdef CONFIG_SOC_LEOPARD
	if (!esf) {
		cpu_trace_enable(1);
	}
#endif

	return 0;
}

int ramdump_transfer(int (*traverse_cb)(uint8_t *data, uint32_t max_len))
{
	ramd_data_t *ramd = ramdump_get_data();
	ramd_head_t *phead = &ramd->head;
	ramd_region_t *pregion = &ramd->region;
	uint32_t offset, next, out_sz;
	uint8_t *xfer_buf = NULL;
	int xfer_len = 0;

	if (!ramd) {
		LOG_ERR("init failed!");
		return 0;
	}

	if (traverse_cb) {
		xfer_buf = k_malloc(RAMD_XFER_BLK_SZ);
		if (!xfer_buf) {
			LOG_ERR("malloc failed!");
			return 0;
		}
	}

	offset = ramd->blk_off;
	while (offset < ramd->part->size) {
		// find ramdump header
		out_sz = ramdump_get_header(ramd, offset, phead, &next);
		if (out_sz == sizeof(ramd_head_t)) {
			LOG_INF("[header v%d] %s org_sz=%d, img_sz=%d, uid=%d, offset=0x%x",
				phead->version, phead->datetime, phead->org_sz, phead->img_sz, phead->inc_uid, offset);
			offset += sizeof(ramd_head_t);

			// verify checksum
			if (phead->img_chksum != ramdump_get_chksum(ramd, offset, phead->img_sz)) {
				LOG_ERR("verify image failed");
			}

			if (traverse_cb) {
				traverse_cb((uint8_t*)phead, sizeof(ramd_head_t));
				xfer_len += sizeof(ramd_head_t);
			}

			while (phead->img_sz > sizeof(ramd_region_t)) {
				// get ramdump region
				out_sz = ramdump_get_region(ramd, offset, pregion);
				if (out_sz == sizeof(ramd_region_t)) {
					LOG_INF("[region] type=%d, mem_addr=0x%08x, mem_sz=%d, img_sz=%d",
						pregion->type, pregion->address, pregion->org_sz, pregion->img_sz);
					offset += sizeof(ramd_region_t);
					phead->img_sz -= sizeof(ramd_region_t);

					if (traverse_cb) {
						traverse_cb((uint8_t*)pregion, sizeof(ramd_region_t));
						xfer_len += sizeof(ramd_region_t);

						while (pregion->img_sz > 0) {
							if (pregion->img_sz >= RAMD_XFER_BLK_SZ) {
								out_sz = RAMD_XFER_BLK_SZ;
							} else {
								out_sz = pregion->img_sz;
							}

							ramdump_read_flash(ramd, offset, xfer_buf, out_sz);
							offset += out_sz;
							pregion->img_sz -= out_sz;
							phead->img_sz -= out_sz;

							traverse_cb(xfer_buf, out_sz);
							xfer_len += out_sz;
						}
					} else {
						offset += pregion->img_sz;
						phead->img_sz -= pregion->img_sz;
					}
				}
			}
		}
		offset = next;
	}

	if (traverse_cb) {
		k_free(xfer_buf);
	}

	return xfer_len;
}

uint32_t ramdump_get_offset(void)
{
	ramd_data_t *ramd = ramd_pdata;

	return ramd->part->offset + ramd->blk_off;
}

uint32_t ramdump_get_size(void)
{
	ramd_data_t *ramd = ramdump_get_data();
	ramd_head_t *phead = &ramd->head;
	uint32_t out_sz, img_chksum;
	uint32_t xfer_len = 0;

	// check ramdump header
	out_sz = ramdump_get_header(ramd, ramd->blk_off, phead, NULL);
	if (out_sz == sizeof(ramd_head_t)) {
		// verify checksum
		img_chksum = ramdump_get_chksum(ramd, ramd->blk_off + sizeof(ramd_head_t), phead->img_sz);
		if (phead->img_chksum == img_chksum) {
			xfer_len = sizeof(ramd_head_t) + phead->img_sz;
		}
	}

	return xfer_len;
}

int ramdump_dump(void)
{
	LOG_INF("ramdump: offset=0x%08x, size=%u", ramdump_get_offset(), ramdump_get_size());

	return ramdump_transfer(NULL);
}

int ramdump_reset(void)
{
	ramd_data_t *ramd = ramdump_get_data();

	flash_erase(ramd->flash_dev, ramd->part->offset, ramd->part->size);

	return 0;
}

static int ramdump_init(const struct device *dev)
{
	ramd_data_t *ramd = ramd_pdata;

	ARG_UNUSED(dev);

	if (ramd == NULL) {
		ramd_pdata = k_malloc(sizeof(ramd_data_t));
		ramd = ramd_pdata;
	}

	if (!ramd) {
		LOG_ERR("init failed!");
		return -1;
	}
	memset(ramd, 0, sizeof(ramd_data_t));

	// init TEMP partiton and device
	for(int i = 0; i < STORAGE_ID_MAX; i++){
		ramd->part = partition_get_stf_part(i, PARTITION_FILE_ID_OTA_TEMP);
		if (ramd->part) {
			ramd->flash_dev = partition_get_storage_dev(ramd->part);
	#ifdef CONFIG_RTC_ACTS
			ramd->rtc_dev = device_get_binding(CONFIG_RTC_0_NAME);
	#endif
			ramd->btc_dev = device_get_binding("BTC");
			ramd->blk_off = ROUND_UP(ramd->part->offset, RAMD_HEADER_BLK_SZ) - ramd->part->offset;
			break;
		}
	}

	// init compress output buffer
	ramd->compr_buf = __ramdump_sram_end;

	// init fastlz buffer
	fastlz_init(ramd->compr_buf + RAMD_COMPR_BLK_SZ * 2, FASTLZ_BUF_SIZE);
	LOG_INF("init compr_buf 0x%x", ramd->compr_buf);

	return 0;
}

SYS_INIT(ramdump_init, POST_KERNEL, 99);

