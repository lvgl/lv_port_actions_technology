/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file system reboot interface for Actions SoC
 */

#include <device.h>
#include <init.h>
#include <soc.h>
#include <pm/pm.h>
#include <linker/linker-defs.h>

#define     PWRGATE_MAINCPU_PG_BIT                                       31
#define     PWRGATE_BT_PG_BIT                                            28
#define     PWRGATE_DSP_AU_PG_BIT                                        26
#define     PWRGATE_GPU_PG_BIT                                           25
#define     PWRGATE_DISPLAY_PG_BIT                                       21


static uint8_t g_pd_cnt[POWERGATE_MAX_DEV];

static uint32_t soc_powerate_bit(uint8_t pg_dev)
{
	switch(pg_dev){
	case POWERGATE_DISPLAY_PG_DEV:
		return PWRGATE_DISPLAY_PG_BIT;
	case POWERGATE_GPU_PG_DEV:
		return PWRGATE_GPU_PG_BIT;
	case POWERGATE_DSP_AU_PG_DEV:
		return PWRGATE_DSP_AU_PG_BIT;
	case POWERGATE_BT_PG_DEV:
		return PWRGATE_BT_PG_BIT;
	case PWRGATE_MAINCPU_PG_DEV:
		return PWRGATE_MAINCPU_PG_BIT;
	}
	return 0;
}
bool soc_powergate_is_poweron(uint8_t pg_dev)
{
	uint32_t bit_num;
	if(pg_dev >= POWERGATE_MAX_DEV)
		return false;
	bit_num = soc_powerate_bit(pg_dev);
	return !!(sys_test_bit(PWRGATE_DIG, bit_num));
}


int soc_powergate_set(uint8_t pg_dev,  bool power_on)
{
	uint32_t key, bit_num, val, timeout;
	bool is_update;
	if(pg_dev >= POWERGATE_MAX_DEV){
		printk("error-pg:no dev=%d\n", pg_dev);
		return -1;
	}
	key = irq_lock();
	if(power_on){
		if(g_pd_cnt[pg_dev] > 128){
			printk("error-pg: dev=%d is poweron over max num\n", pg_dev);
		}else{
			g_pd_cnt[pg_dev]++;
		}		
	}else{
		if(g_pd_cnt[pg_dev]){
			g_pd_cnt[pg_dev]--;
		}else{
			printk("error-pg: dev=%d is poweroff\n", pg_dev);
		}
	}

	bit_num = soc_powerate_bit(pg_dev);
	val = sys_read32(PWRGATE_DIG);
	sl_dbg("pg:dev=%d num=%d, onoff=%d,reg=0x%x\n", pg_dev, g_pd_cnt[pg_dev], power_on, val);

	is_update = false;
	if(g_pd_cnt[pg_dev]){
		if(!(val & (1<<bit_num))){// if power off,  power on
			val |= (1<<bit_num);
			is_update = true;
			sl_dbg("pg:dev=%d is poweron\n", pg_dev);
		}
	}else{
		if(val & (1<<bit_num)){// if power on ,  power off
			val &= ~(1<<bit_num);
			is_update = true;
			sl_dbg("pg: dev=%d is poweroff\n", pg_dev);
		}
	}

	if(is_update) {
		sys_write32(val, PWRGATE_DIG);
		timeout = 1000;  /* 1ms */
		while (timeout > 0) {
			if(sys_test_bit(PWRGATE_DIG_ACK, bit_num))
				break;
			soc_udelay(10);
			timeout -= 10;
		}
	}
	irq_unlock(key);
	return g_pd_cnt[pg_dev];
}

void soc_powergate_init(void)
{
	uint32_t i, bit_num, val;
	val = sys_read32(PWRGATE_DIG);
	printk("PWRGATE_DIG=0x%x\n", val);
	for(i = 0; i < POWERGATE_MAX_DEV; i++){
		bit_num = soc_powerate_bit(i);
		if(val & (1 << bit_num)){
			g_pd_cnt[i] = 1;
			printk("pg: dev=%d is poweron, num=%d\n", i, g_pd_cnt[i]);
		}else{
			g_pd_cnt[i] = 0;
			printk("pg: dev=%d is poweroff, num=%d\n", i, g_pd_cnt[i]);
		}
	}
}

void soc_powergate_dump(void)
{
	uint32_t i, bit_num, val;
	val = sys_read32(PWRGATE_DIG);
	for(i = 0; i < POWERGATE_MAX_DEV; i++){
		bit_num = soc_powerate_bit(i);
		if(val & (1 << bit_num)){
			printk("pg: dev=%d is poweron, num=%d\n", i, g_pd_cnt[i]);
		}else{
			printk("pg: dev=%d is poweroff, num=%d\n", i, g_pd_cnt[i]);
		}
	}
}




