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
#include <drivers/rtc.h>
#include <board_cfg.h>
#include <drivers/nvram_config.h>
#include <drivers/flash.h>


#define REBOOT_REASON_MAGIC 		0x4252	/* 'RB' */


int sys_pm_get_wakeup_source(union sys_pm_wakeup_src *src)
{
	uint32_t wk_pd;

	if (!src)
		return -EINVAL;

	src->data = 0;

	wk_pd = soc_pmu_get_wakeup_source();

	if (wk_pd & BIT(0))
		src->t.long_onoff = 1;

	if (wk_pd & BIT(1))
		src->t.short_onoff = 1;

	if (wk_pd & BIT(13))
		src->t.bat = 1;

	if (wk_pd & BIT(5))
		src->t.wio = 1;

	if (wk_pd & BIT(12))
		src->t.remote = 1;

	if (wk_pd & BIT(4))
		src->t.alarm = 1;

	if (wk_pd & BIT(11))
		src->t.batlv = 1;

	if (wk_pd & BIT(10))
		src->t.dc5vlv = 1;

	if (wk_pd & BIT(2))
		src->t.dc5vin = 1;

	if (soc_boot_get_watchdog_is_reboot() == 1)
		src->t.watchdog = 1;

	return 0;
}

void sys_pm_set_wakeup_src(void)
{
	uint32_t key, val;

	key = irq_lock();

	val = sys_read32(WKEN_CTL_SVCC) & (~0x1fff);

	val = WAKE_CTL_LONG_WKEN | WAKE_CTL_ALARM8HZ_WKEN | WAKE_CTL_WIO0LV_DETEN;


	if(!soc_pmu_get_dc5v_status()) {
		val |= WAKE_CTL_DC5VIN_WKEN;
	}

	sys_write32(val, WKEN_CTL_SVCC);
	k_busy_wait(500);

	irq_unlock(key);
}

/*
**  system power off to cal rtc
*/
static void sys_pm_poweroff_rtc(void)
{
	unsigned int key;
#ifdef CONFIG_ACTIONS_PRINTK_DMA
		printk_dma_switch(0);
#endif
	sys_write32(sys_read32(WAKE_PD_SVCC), WAKE_PD_SVCC);
	printk("system power down!WKEN_CTL=0x%x\n", sys_read32(WKEN_CTL_SVCC));	
	key = irq_lock();
#ifdef CONFIG_PM_DEVICE
	printk("dev power off\n");
	pm_power_off_devices();
	printk("dev power end\n");
#endif

#ifdef CONFIG_SPINAND_ACTS
    flash_flush(device_get_binding(CONFIG_SPINAND_FLASH_NAME), 0);
#endif

	while(1) {
		sys_write32(0, POWER_CTL_SVCC);
		/* wait 10ms */
		k_busy_wait(10000);
		printk("poweroff fail, need reboot!\n");
	}
}


/*@brief 8hzchcle cal to ms by rc32k 
rc32k_sum = rc32k_old + rc32k_new 
*/
static uint32_t sys_pmu_8hzcycle_to_ms(uint32_t cycle, uint32_t rc32k_sum)
{
	uint64_t  cal = cycle;
	uint32_t ms;
	ms = cal*(32768*125*2)/rc32k_sum; // cal	interval ms 
	return ms;
}

static void sys_pm_rtc_backup_check(void)
{
	int ret;
	union sys_pm_wakeup_src wk_src;	
	struct sys_pm_backup_time pm_bak_time = {0};
	uint32_t rc32k_freq_sum, new_rc32k_freq;
	uint32_t counter8hz_cycles, interval_cycles, cur_time_sec, ms, sec;

	soc_pstore_set(SOC_PSTORE_TAG_RTC_RC32K_CAL, 1);
	sys_pm_get_wakeup_source(&wk_src);

	if(!wk_src.t.alarm) 
		return;
	ret = nvram_config_get(CONFIG_PM_BACKUP_TIME_NVRAM_ITEM_NAME,
			&pm_bak_time, sizeof(struct sys_pm_backup_time));
	if (ret != sizeof(struct sys_pm_backup_time)) {
		printk("boot:failed to get pm backup time to nvram =%d\n", ret);
		return;
	}
	if(pm_bak_time.is_user_cur_use){
		printk("the user's alarm\n");// go to system handle
		pm_bak_time.is_user_alarm_on = 0;
		ret = nvram_config_set(CONFIG_PM_BACKUP_TIME_NVRAM_ITEM_NAME,
						&pm_bak_time, sizeof(struct sys_pm_backup_time));
		if (ret) {
			printk("failed to save pm backup time to nvram");
		}
		return;
	}
	if(pm_bak_time.is_use_alarm_cal && pm_bak_time.is_backup_time_valid){
		uint32_t alram_ms = 1000*60*60; // 1h wakeup
		new_rc32k_freq =  acts_clock_rc32k_set_cal_cyc(200);
		printk("rc32k_freq=%d,old=%d\n", new_rc32k_freq, pm_bak_time.rc32k_freq);
		ret = soc_pmu_get_counter8hz_cycles(false);
		if(ret < 0 )
			return;
		counter8hz_cycles = ret;
		printk("current 8hz: %d\n", counter8hz_cycles);
		/* counter8hz overflow */
		if (counter8hz_cycles < pm_bak_time.counter8hz_cycles) {
			interval_cycles = PMU_COUTNER8HZ_MAX - pm_bak_time.counter8hz_cycles + counter8hz_cycles;
		} else {
			interval_cycles = counter8hz_cycles - pm_bak_time.counter8hz_cycles;
		}
		cur_time_sec = pm_bak_time.rtc_time_sec;

		rc32k_freq_sum = (pm_bak_time.rc32k_freq + new_rc32k_freq); // old + new
		ms = sys_pmu_8hzcycle_to_ms(interval_cycles, rc32k_freq_sum) + pm_bak_time.rtc_time_msec; 

		cur_time_sec += ms / 1000;
		ms = ms % 1000 ;
		sec = cur_time_sec - pm_bak_time.rtc_time_sec;
		pm_bak_time.rtc_time_sec = cur_time_sec;
		pm_bak_time.rtc_time_msec = ms;
		pm_bak_time.counter8hz_cycles = counter8hz_cycles;
		pm_bak_time.rc32k_freq = new_rc32k_freq;
		printk("time: %ds, %dms, interval=%d\n", cur_time_sec, ms, sec);

		if(pm_bak_time.is_user_alarm_on){// user alarm on ,check if next hour is user alarm
			if(pm_bak_time.user_alarm_cycles > counter8hz_cycles){
				interval_cycles = pm_bak_time.user_alarm_cycles - counter8hz_cycles;
			}else{
				interval_cycles = PMU_COUTNER8HZ_MAX - counter8hz_cycles +  pm_bak_time.user_alarm_cycles;
			}
			ms =  (uint64_t)interval_cycles*32768*125 /new_rc32k_freq; // cal real ms
			
			printk("b user set alarm after %d ms\n", ms);
			if(ms < alram_ms*2) { // It is multiplied by 2 because the user's alarm may be lost due to calculation error
				alram_ms = ms;	// use  user alarm
				pm_bak_time.is_user_cur_use = 1;
				printk("b set user alarm\n");
			}
		}

		ret = nvram_config_set(CONFIG_PM_BACKUP_TIME_NVRAM_ITEM_NAME,
						&pm_bak_time, sizeof(struct sys_pm_backup_time));
		if (ret) {
			printk("failed to save pm backup time to nvram");
			return ;
		}
		soc_pmu_alarm8hz_enable(alram_ms);
		printk("bootloader pwoer off set next alarm=%d ms\n", alram_ms);
		sys_pm_poweroff_rtc();
	}

}
static int sys_pm_rtc_backup_init(const struct device *arg)
{
	sys_pm_rtc_backup_check();
	return 0;
}
SYS_INIT(sys_pm_rtc_backup_init, APPLICATION, 99);


/*
**  system power off
*/
void sys_pm_poweroff(void)
{
	unsigned int key;
	/* wait 10ms, avoid trigger onoff wakeup pending */
	k_busy_wait(10000);
#ifdef CONFIG_ACTIONS_PRINTK_DMA
		printk_dma_switch(0);
#endif
	sys_pm_set_wakeup_src();

	printk("system power down!WKEN_CTL=0x%x\n", sys_read32(WKEN_CTL_SVCC));

	key = irq_lock();
#ifdef CONFIG_PM_DEVICE
	printk("dev power off\n");
	pm_power_off_devices();
	printk("dev power end\n");
#endif

#ifdef CONFIG_SPINAND_ACTS
    flash_flush(device_get_binding("spinand"), 0);
#endif

	while(1) {
		sys_write32(0, POWER_CTL_SVCC);
		/* wait 10ms */
		k_busy_wait(10000);
		printk("poweroff fail, need reboot!\n");
		sys_pm_reboot(0);
	}

	/* never return... */
}


void sys_pm_reboot(int type)
{
	unsigned int key;

#ifdef CONFIG_ACTIONS_PRINTK_DMA
	printk_dma_switch(0);
#endif
	if(type == REBOOT_TYPE_GOTO_SWJTAG){
		printk("set jtag flag\n");
		type = REBOOT_TYPE_NORMAL;
		sys_set_bit(RTC_REMAIN2, 0); //bit 0 adfu flag
	}
	printk("system reboot, type 0x%x!\n", type);

	key = irq_lock();

#ifdef CONFIG_SPINAND_ACTS
    flash_flush(device_get_binding("spinand"), 0);
#endif
	/* store reboot reason in RTC_REMAIN0 for bootloader */
	sys_write32((REBOOT_REASON_MAGIC << 16) | (type & 0xffff), RTC_REMAIN3);
	k_busy_wait(500);
	sys_write32(0x5f, WD_CTL);
	while (1) {
		;
	}

	/* never return... */
}

int sys_pm_get_reboot_reason(u16_t *reboot_type, u8_t *reason)
{

	uint32_t reg_val;
	reg_val = soc_boot_get_reboot_reason();
	*reboot_type = reg_val & 0xff00;
	*reason      =  reg_val & 0xff;
	return 0;

}

static void  ctk_close(void)
{
	int i;
	for(i = 0; i < 20; i++) // tpkey poweroff
	{
		sys_write32(0x40000009, CTK_CTL);
		sys_write32(0x4000000d, CTK_CTL);
	}
}

__ramfunc void soc_udelay(uint32_t us)
{
	uint32_t cycles_per_1us, wait_cycles;
	volatile uint32_t i;
	uint8_t cpuclk_src = sys_read32(CMU_SYSCLK) & 0x7;

	if (cpuclk_src == 0)
		cycles_per_1us = 4;
	else if (cpuclk_src == 1)
		cycles_per_1us = 25;
	else if (cpuclk_src == 3)
		cycles_per_1us = 56; /* %15 deviation */
	else
		cycles_per_1us = 74;

	wait_cycles = cycles_per_1us * us / 10;

	for (i = 0; i < wait_cycles; i++) { /* totally 13 instruction cycles */
		;
	}
}

static int soc_pm_init(const struct device *arg)
{

	printk("WAKE_PD_SVCC = 0x%x\n", sys_read32(WAKE_PD_SVCC));
	
	sys_write32(0x3, CMU_S1CLKCTL); // hosc+rc4m
	sys_write32(0x3, CMU_S1BTCLKCTL); // hosc+rc4m
	sys_write32(0x1, CMU_S2SCLKCTL); // RC4M enable
	sys_write32(0x8, CMU_S3CLKCTL); // S3 colse hosc and RC4M, enable RAM4 CLK SPIMT ICMT CLK ENABLE
	sys_write32(0x0, CMU_PMUWKUPCLKCTL); //select wk clk RC32K, if sel RC4M/4 ,must enable RC4M IN sleep
	sys_write32(0x1, CMU_GPIOCLKCTL); //select gpio clk RC4M

	sys_write32(0x06202053, VOUT_CTL1_S3);//VDD=0.7, vdd1.2=0.7
	sys_write32(0x5958, RC4M_CTL);

	ctk_close();
	return 0;
}

SYS_INIT(soc_pm_init, PRE_KERNEL_1, 20);




