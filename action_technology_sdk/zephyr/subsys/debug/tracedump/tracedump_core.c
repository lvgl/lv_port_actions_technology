/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file tracedump_core
 */

#include <string.h>
#include <stdio.h>
#include <linker/linker-defs.h>
#include <kernel.h>
#include <kernel_internal.h>
#include <errno.h>
#include <init.h>
#include <device.h>
#include <board_cfg.h>
#include <logging/log.h>
#include <debug/tracedump.h>
#include "tracedump_core.h"

LOG_MODULE_REGISTER(tracedump, CONFIG_KERNEL_LOG_LEVEL);

static traced_data_t *traced_pdata = (traced_data_t*)&__tracedump_ram_start;

extern K_KERNEL_STACK_ARRAY_DEFINE(z_interrupt_stacks, CONFIG_MP_NUM_CPUS,
				   CONFIG_ISR_STACK_SIZE);

static inline traced_data_t* tracedump_get_data(void)
{
	return traced_pdata;
}

static inline traced_info_t* tracedump_get_info(void)
{
	return (traced_info_t*)((uint32_t)traced_pdata + sizeof(traced_data_t));
}

static inline void tracedump_print_info(int idx, void *buf, uint32_t size)
{
	uint32_t *buf32 = (uint32_t*)buf;
	uint32_t size32 = size / sizeof(uint32_t);
	
	printk("[%d]", idx);

	for (int i = 0; i < size32; i ++) {
		printk(" 0x%08x", buf32[i]);
	}

	printk("\n");
}

static inline int tracedump_is_filtered(traced_data_t *traced, int event, uint32_t data1, uint32_t data2)
{
	if ((traced->event_mask & event) == 0) {
		return 0;
	}

	if (traced->data_sz == 0) {
		return 1;
	}

	if (traced->data_off == 0) {
		return (data2 == traced->data_sz);  // filter size
	}

	return (data1 >= traced->data_off) && (data1 < (traced->data_off + traced->data_sz));  // filter aera
}

static void tracedump_get_stack(uint32_t *sp, uint32_t *sp_len)
{
	struct k_thread *th = k_current_get();
	uint32_t stack_start, stack_sz;
	
	if (k_is_in_isr()) {
		*sp = __get_MSP();
		stack_start = (uint32_t)Z_KERNEL_STACK_BUFFER(z_interrupt_stacks[0]);
		stack_sz = K_KERNEL_STACK_SIZEOF(z_interrupt_stacks[0]);
	} else {
		*sp = __get_PSP();
		stack_start = (uint32_t)th->stack_info.start;
		stack_sz = th->stack_info.size;
		
	}

	*sp_len = (stack_start + stack_sz - *sp) / sizeof(uint32_t);
}

static int tracedump_is_funcs(uint32_t pc)
{
	if ((pc >= (unsigned long)&__text_region_start) && (pc <= (unsigned long)&__text_region_end)) {
		return 1;
	}

	if ((pc >= (unsigned long)&__sensor_func_start) && (pc <= (unsigned long)&__sensor_func_end)) {
		return 1;
	}

	if ((pc >= (unsigned long)&_sram_func_start) && (pc <= (unsigned long)&_sram_func_end)) {
		return 1;
	}

	if ((pc >= (unsigned long)&__ramfunc_start) && (pc <= (unsigned long)&__ramfunc_end)) {
		return 1;
	}

	return 0;
}

static void tracedump_get_funcs(uint32_t *buf, uint32_t len, uint32_t *func, uint32_t num)
{
	uint32_t pc;

	while ((len > 0) && (num > 0)) {
		pc = *buf;
		buf ++;
		len --;
		
		if (tracedump_is_funcs(pc)) {
			*func = pc & 0xfffffffe;  // clear thumb bit
			func ++;
			num --;
		}
	}

	while (num > 0) {
		*func = 0;
		func ++;
		num --;
	}
}

int tracedump_set_enable(int event, int enable)
{
	traced_data_t *traced = tracedump_get_data();
	uint32_t key;

	key = irq_lock();
	
	if (enable) {
		traced->event_mask |= event;
	} else {
		traced->event_mask &= ~event;
	}

	irq_unlock(key);

	return 0;
}

int tracedump_get_enable(int event)
{
	traced_data_t *traced = tracedump_get_data();
	
	return (traced->event_mask & event) ? 1 : 0;
}

int tracedump_save(int event, uint32_t data1, uint32_t data2)
{
	traced_data_t *traced = tracedump_get_data();
	traced_info_t *info = tracedump_get_info();
	uint32_t sp, sp_len, new_idx;
	uint32_t key;

	if (atomic_get(&traced->locked) > 0) {
		return -1;
	}

	if (!tracedump_is_filtered(traced, event, data1, data2)) {
		return -1;
	}

	if (traced->cur_cnt == traced->max_cnt) {
		traced->drop_flag = 1;
	}

	key = irq_lock();

	traced->uniq_id ++;

	// alloc item
	if (traced->cur_cnt == 0) {
		new_idx = traced->end_idx;
	} else {
		new_idx = (traced->end_idx + 1) % traced->max_cnt;
	}
	if (new_idx != traced->start_idx) {
		// index not full: alloc new item
		if (traced->cur_cnt > 0) {
			traced->end_idx = new_idx;
		}
	} else {
		// index full
		if (traced->cur_cnt == traced->max_cnt) {
			// item full: overwrite oldest item
			traced->end_idx = new_idx;
			traced->start_idx = (new_idx + 1) % traced->max_cnt;
		} else {
			// item not full: find free item
			new_idx = traced->start_idx;
			for (int i = 0; i < traced->max_cnt; i ++) {
				if (info[new_idx].func[0] == 0) {
					break;
				}
				new_idx = (new_idx + 1) % traced->max_cnt;
			}
		}
	}
	info = &info[new_idx];

	// save data
	info->uniq_id = traced->uniq_id;
	info->data[0] = data1;
	info->data[1] = data2;

	// save backtrace
	tracedump_get_stack(&sp, &sp_len);
	tracedump_get_funcs((uint32_t*)sp, sp_len, info->func, TRACED_FUNC_CNT);

	// increase cnt
	if (traced->cur_cnt < traced->max_cnt) {
		traced->cur_cnt ++;
	}

	irq_unlock(key);

	return 0;
}

int tracedump_remove(int event, uint32_t data1)
{
	traced_data_t *traced = tracedump_get_data();
	traced_info_t *info = tracedump_get_info();
	uint32_t idx;
	uint32_t key;
	int ret = -1;

	if (!tracedump_is_filtered(traced, event, data1, traced->data_sz)) {
		return -1;
	}

	key = irq_lock();

	idx = traced->start_idx;

	for (int i = 0; i < traced->max_cnt; i ++) {
		if ((info[idx].data[0] == data1) && (info[idx].func[0] != 0)) {
			info[idx].func[0] = 0;  // clear func
			traced->cur_cnt --;     // decrease cnt
			if ((idx == traced->end_idx) && (traced->cur_cnt > 0)) {
				if (traced->end_idx == 0) {
					traced->end_idx = traced->max_cnt - 1;
				} else {
					traced->end_idx --;
				}
			}
			ret = 0;
			break;
		}
		idx = (idx + 1) % traced->max_cnt;
	}

	irq_unlock(key);

	return ret;
}

int tracedump_transfer(int (*traverse_cb)(uint8_t *data, uint32_t max_len))
{
	traced_data_t *traced = tracedump_get_data();
	traced_info_t *info;
	uint32_t idx = traced->start_idx;
	uint32_t len = 0;

	uint32_t key;
	if (!traverse_cb) {
#ifdef CONFIG_ACTIONS_PRINTK_DMA
		printk_dma_switch(0);
#endif
		key = irq_lock();
	}

	// print header
	if (traverse_cb) {
		traverse_cb((uint8_t*)traced, sizeof(traced_data_t));
	} else {
		tracedump_print_info(-1, traced, sizeof(traced_data_t));
	}
	len += sizeof(traced_info_t);

	// print data and backtrace
	info = tracedump_get_info();
	for (int i = 0; i < traced->max_cnt; i ++) {
		if (info[idx].func[0] != 0) {
			if (traverse_cb) {
				traverse_cb((uint8_t*)&info[idx], sizeof(traced_info_t));
			} else {
				tracedump_print_info(idx, &info[idx], sizeof(traced_info_t));
			}
			len += sizeof(traced_info_t);
		}
		idx = (idx + 1) % traced->max_cnt;
	}

	if (!traverse_cb) {
		irq_unlock(key);
#ifdef CONFIG_ACTIONS_PRINTK_DMA
		printk_dma_switch(1);
#endif
	}

	return len;
}

uint32_t tracedump_get_offset(void)
{
	return (uint32_t)tracedump_get_data();
}

uint32_t tracedump_get_size(void)
{
	traced_data_t *traced = tracedump_get_data();

	return sizeof(traced_data_t) + traced->max_cnt * sizeof(traced_info_t);
}

int tracedump_dump(void)
{
	traced_data_t *traced = tracedump_get_data();
	int len = tracedump_transfer(NULL);

	LOG_INF("[0x%x] [0x%08x] (0x%08x + 0x%x) (0x%x + 0x%x) (%d ~ %d) (%d / %d) %c", 
		traced->event_mask, traced->uniq_id, tracedump_get_offset(), tracedump_get_size(),
		traced->data_off, traced->data_sz, traced->start_idx, traced->end_idx,
		traced->cur_cnt, traced->max_cnt, traced->drop_flag ? '*' : ' ');

	return len;
}

int tracedump_reset(void)
{
	traced_data_t *traced = tracedump_get_data();
	traced_info_t *info = tracedump_get_info();

	if (atomic_get(&traced->locked) > 0) {
		return -1;
	}

	uint32_t key = irq_lock();

	traced->cur_cnt = 0;
	traced->start_idx = 0;
	traced->end_idx = 0;
	traced->uniq_id = 0;
	traced->drop_flag = 0;
	memset(info, 0, traced->max_cnt * sizeof(traced_info_t));

	irq_unlock(key);

	return 0;
}

int tracedump_lock(void)
{
	traced_data_t *traced = tracedump_get_data();

	atomic_inc(&traced->locked);
	return 0;
}

int tracedump_unlock(void)
{
	traced_data_t *traced = tracedump_get_data();

	if (atomic_get(&traced->locked) > 0) {
		atomic_dec(&traced->locked);
	}
	return 0;
}

int tracedump_set_filter(uint32_t data_off, uint32_t data_sz)
{
	traced_data_t *traced = tracedump_get_data();

	if (atomic_get(&traced->locked) > 0) {
		return -1;
	}

	uint32_t key = irq_lock();

	traced->data_off = data_off;
	traced->data_sz = data_sz;

	irq_unlock(key);

	return 0;
}

static int tracedump_init(const struct device *dev)
{
	traced_data_t *traced = tracedump_get_data();
	traced_info_t *info;
	uint32_t info_sz;

	ARG_UNUSED(dev);

	/* enable JPEG OUTRAM */
	sys_write32(sys_read32(CMU_MEMCLKEN1) | (0x1f << 24), CMU_MEMCLKEN1);

	memset(traced, 0, sizeof(traced_data_t));

	traced->magic = TRACED_MAGIC;
	info_sz = (uint32_t)&__tracedump_ram_end - (uint32_t)&__tracedump_ram_start - sizeof(traced_data_t);
	traced->max_cnt = (uint16_t)(info_sz / sizeof(traced_info_t));

	info = tracedump_get_info();
	memset(info, 0, info_sz);

	LOG_INF("tracedump init 0x%08x (%d)", (uint32_t)traced, traced->max_cnt);

	return 0;
}

SYS_INIT(tracedump_init, POST_KERNEL, 10);

