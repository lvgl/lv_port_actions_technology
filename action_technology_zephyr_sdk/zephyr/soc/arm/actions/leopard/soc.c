/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
 * Copyright (c) 2016 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief System/hardware module for ATJ215X processor
 */

#include <device.h>
#include <init.h>
#include <arch/cpu.h>
#include "soc.h"
#include "soc_atp.h"
#include <linker/linker-defs.h>
//#include <arch/arm/aarch32/cortex_m/cmsis.h>


static void jtag_config(unsigned int group_id)
{
	printk("jtag switch to group=%d\n", group_id);
	if (group_id < 3)
		sys_write32((sys_read32(JTAG_CTL) & ~(3 << 0)) | (group_id << 0) | (1 << 4), JTAG_CTL);

}

void jtag_set(void)
{
	jtag_config(0);
}
/**
 * \brief  clear watchdog
 */
void soc_watchdog_clear(void)
{
	sys_set_bit(WD_CTL, 0);
}
static void wd_check_wdreset_cnt(void)
{
	uint32_t reset_cnt;
	soc_watchdog_clear();
	soc_pstore_get(SOC_PSTORE_TAG_WD_RESET_CNT, &reset_cnt);
	printk("wd cnt=%d, WD_CTL=0x%x\n", reset_cnt, sys_read32(WD_CTL));
	if(reset_cnt > 10){
		printk("reboot ota\n");
		soc_pstore_set(SOC_PSTORE_TAG_WD_RESET_CNT, 0);
		sys_pm_reboot(REBOOT_TYPE_GOTO_OTA);
	}else{
		reset_cnt++;
		soc_pstore_set(SOC_PSTORE_TAG_WD_RESET_CNT, reset_cnt);
	}
}
 /**
  * \brief	if boot to main clear wd reset cnt
  */
 void wd_clear_wdreset_cnt(void)
{
	soc_pstore_set(SOC_PSTORE_TAG_WD_RESET_CNT, 0);
}

__sleepfunc int soc_dvfs_opt(void)
{
	return (sys_read32(UID1) >> 17) & 0x7;
}

uint8_t ipmsg_btc_get_ic_pkt(void)
{
	uint32_t val;

	val = sys_read32(UID1);
	return (uint8_t)((val >> 24) & 0xF);        /* bit24~27: IC Package */
}

#if 0
static void cpu_init(void)
{
	int i;
	unsigned int val;
	printk("rc192 cal\n");
	sys_write32(0x80011481, RC192M_CTL);
	soc_udelay(20);
	sys_write32(0x8001148d, RC192M_CTL);
	soc_udelay(200);
	while(1){
		if(sys_read32(RC192M_CTL)& (1<<24)){
			for(i = 0; i < 3; i++){ // 3 cal done is ok
				soc_udelay(20);
				if(!(sys_read32(RC192M_CTL)& (1<<24)))
					break;
			}
			if(i == 3)
				break;
		}
	}

	val = (sys_read32(RC192M_CTL) & (0x7f<<25)) >> 25;
	if(val)
		val -= 1;
	sys_write32(0x80011001 | (val<<4) , RC192M_CTL); 
	soc_udelay(200);
	sys_write32(0x215, CMU_SYSCLK);// CPU rcM256/2 =128, ahb div 1
}
#endif

__sleepfunc static void leopard_set_hosc_ctrl()
{
	/*
	IO_WRITE(HOSC_CTL, (0x601f1b6 | (CAP<<17)));
	如果是首次配置（HOSC_CTL[25]: 0->1）需要将CPUCLK切换到RC4M，等待HOSC_CTL[28]为1后再将CPUCLK切回HOSC/COREPLL
	*/
	uint32_t val;

	/* switch cpuclk src to 4MRC */
	val = sys_read32(CMU_SYSCLK);
	sys_write32(val & ~0x7, CMU_SYSCLK);

	/* update HOSC_CTL & wait HOSC_READY */
	sys_write32(0x600f7f6 | (sys_read32(HOSC_CTL) & HOSC_CTL_HOSC_CAP_MASK), HOSC_CTL);
	while(!(sys_read32(HOSC_CTL) & (1<< HOSC_CTL_HOSC_READY)));

	/* backup cpuclk src to COREPLL */
	sys_write32(val, CMU_SYSCLK);
}
/**
 * @brief Perform basic hardware initialization at boot.
 *
 * This needs to be run from the very beginning.
 * So the init priority has to be 0 (zero).
 *
 * @return 0
 */
static int leopard_init(const struct device *arg)
{
	uint32_t key;
	uint32_t val;


	ARG_UNUSED(arg);
	soc_udelay(50);
	leopard_set_hosc_ctrl();
	soc_udelay(10);
	key = irq_lock();
	/* Install default handler that simply resets the CPU
	 * if configured in the kernel, NOP otherwise
	 */
	NMI_INIT();

	irq_unlock(key);
	soc_powergate_init();
	sys_write32(0x10e, CMU_GPUCLK);//COREPLL/1.5
	acts_clock_peripheral_enable(CLOCK_ID_GPU); //for the gpu reset, because the gpu is in an unstable state
	soc_udelay(1);
	acts_reset_peripheral_deassert(RESET_ID_GPU);
	soc_udelay(1);
	acts_reset_peripheral_assert(RESET_ID_GPU);//
	soc_udelay(2);
	acts_clock_peripheral_disable(CLOCK_ID_GPU);//
	/* disable gpu power gating */
	//sys_write32(sys_read32(PWRGATE_DIG) & ~(0x1 << 25), PWRGATE_DIG);
	if(soc_powergate_is_poweron(POWERGATE_GPU_PG_DEV))
		soc_powergate_set(POWERGATE_GPU_PG_DEV, false);

	//cpu_init();

	wd_check_wdreset_cnt();
	/* Update CMSIS SystemCoreClock variable (HCLK) */
	/* At reset, system core clock is set to 16 MHz from HSI */
	//SystemCoreClock = 16000000;
	//while(!arg);

	//sys_write32(0x0, WIO0_CTL); //default set wio0 to gpio func

	/*for lowpower*/
	//sys_write32(0x30F, SPI1_CLKGATING);

	/* init ppi */
	ppi_init();

	/* Initialize SDMA */
	acts_reset_peripheral_assert(RESET_ID_SDMA);
	acts_clock_peripheral_enable(CLOCK_ID_SDMA);
	acts_reset_peripheral_deassert(RESET_ID_SDMA);

	/* Enable SDMA, DE access SPI0 NOR and SPI1 PSRAM, and GPU accessing SPI1 PSRAM */
	//sys_write32(sys_read32(SPICACHE_CTL) | BIT(10) | BIT(11), SPICACHE_CTL);
	sys_write32((sys_read32(SPICACHE_CTL) & ~(0x3 << 5)) | (0x1 << 5) | BIT(10) | BIT(11), SPICACHE_CTL);

	if (soc_dvfs_opt()) {
		sys_write32((sys_read32(SPI1_DELAYCHAIN)  & ~(0x3f << 8)) \
					| (SPI1_DELAYCHAIN_CLKOUT << 8), SPI1_DELAYCHAIN);
		sys_write32(sys_read32(SPI1_CACHE_CTL) | BIT(8) | BIT(10) | BIT(11) | BIT(14) | BIT(15), SPI1_CACHE_CTL);
	} else {
		sys_write32(sys_read32(SPI1_CACHE_CTL) | BIT(8) | BIT(10) | BIT(11) | BIT(14) /*| BIT(15)*/, SPI1_CACHE_CTL);
	}

	sys_write32(BIT(24) | (0<<4) | (0xa<<8) | (0x1<<0), SPI1_GPU_CTL);

	/* Share RAM clock, select HOSC(32MHZ)*/
	val = sys_read32(CMU_MEMCLKSRC0);
	val = (val & ~(7 << 25)) | (1 << 25);
	sys_write32(val, CMU_MEMCLKSRC0);

	val = sys_read32(CMU_MEMCLKEN0);
	val |= (1 << 25);
	sys_write32(val, CMU_MEMCLKEN0);

	val = 0;
	if(soc_atp_get_pmu_calib(12, &val)){
		printk("get psram pg fail\n");
	}else{
		printk("psram=%d, 0x%x\n", val, sys_read32(VOUT_CTL0));
		if(val){
			sys_clear_bit(VOUT_CTL0, 31);
			printk("VOUT0= 0x%x\n", sys_read32(VOUT_CTL0) );
		}			
	}
	printk("apm=%d\n", soc_psram_is_apm());
	//val = sys_read32(VOUT_CTL1_S1M) ;
	//val = (val & ~(0xFff)) | 0xedd;   // vdd set 1.2
	//sys_write32(val, VOUT_CTL1_S1M);
	//sys_write32(0x12, CMU_SYSCLK);// cpu 200
	return 0;
}

SYS_INIT(leopard_init, PRE_KERNEL_1, 0);


/*if CONFIG_WDOG_ACTS enable , wd driver call wd_clear_wdreset_cnt */
#ifndef CONFIG_WDOG_ACTS
/**
 * @brief Perform basic hardware initialization at boot.
 *
 * before boot to main clear wd reset cnt
 * @return 0
 */
static int wd_reset_cnt_init(const struct device *arg)
{
	wd_clear_wdreset_cnt();
	sys_write32(0, WD_CTL); /*disable watch dog*/
	return 0;
}

SYS_INIT(wd_reset_cnt_init, APPLICATION, 91);
#endif

