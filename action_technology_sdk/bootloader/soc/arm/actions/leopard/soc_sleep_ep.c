/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file soc_sleep.c  sleep  for Actions SoC
 */


#include <zephyr.h>
#include <soc.h>
#include <arch/arm/aarch32/cortex_m/cmsis.h>
#include <string.h>
#include <drivers/timer/system_timer.h>
#include <linker/linker-defs.h>


#define MEM_S2_ITEM_MAX	10
struct mem_backup {
	void *buf;
	uint32_t len;
};

/* COREPLL RW fields range from 0 t0 7 */
//static uint32_t ram_power_backup;
/* Save and restore the registers */
static const uint32_t backup_regs_addr[] = {
	CMU_SYSCLK,
	CMU_SPI0CLK,
	SPI1_CACHE_CTL,
	ADC_REF_LDO_CTL,
	AVDDLDO_CTL,
	PMUADC_CTL,
	WIO0_CTL,
	WIO1_CTL,
	CMU_MEMCLKEN0,
	CMU_MEMCLKEN1,
	CMU_DEVCLKEN1,
	CMU_DEVCLKEN0,
	RMU_MRCR1,
	RMU_MRCR0,
	NVIC_ISER0,
	NVIC_ISER1,
};

static void suspend_check(void)
{
}

static void wakeup_check(void)
{
}

struct sleep_wk_data {
	uint16_t wksrc;
	uint16_t wk_en_bit;
	const char *wksrc_msg;
};

struct sleep_wk_data wk_msg[] = {
	{SLEEP_WK_SRC_BT, 		IRQ_ID_BT,  	"BT" },
	{SLEEP_WK_SRC_GPIO, 	IRQ_ID_GPIO,    "GPIO" },
	{SLEEP_WK_SRC_PMU, 		IRQ_ID_PMU,  	"PMU" },
	{SLEEP_WK_SRC_T0,		IRQ_ID_TIMER0, 	"T0" },
	{SLEEP_WK_SRC_T1,		IRQ_ID_TIMER1,	"T1" },
	{SLEEP_WK_SRC_TWS,		IRQ_ID_TWS,     "TWS"},

};
#define SLEEP_WKSRC_NUM ARRAY_SIZE(wk_msg)

static volatile uint16_t g_sleep_wksrc_en, g_sleep_wksrc_src;


void sys_s3_wksrc_set(enum S_WK_SRC_TYPE src)
{
	g_sleep_wksrc_en |= (1 << src);
}
enum S_WK_SRC_TYPE sys_s3_wksrc_get(void)
{
	return g_sleep_wksrc_src;
}

static enum S_WK_SRC_TYPE sys_sleep_check_wksrc(void)
{
	int i;
	uint32_t wk_pd0, wk_pd1,wkbit;
	g_sleep_wksrc_src = 0;
	wk_pd0 = sys_read32(NVIC_ISPR0);
	wk_pd1 = sys_read32(NVIC_ISPR1);
	//printk("WK NVIC_ISPR0=0x%x\n", wk_pd0);
	//printk("WK NVIC_ISPR1=0x%x\n", wk_pd1);
	for(i = 0; i < SLEEP_WKSRC_NUM; i++){
		if((1<<wk_msg[i].wksrc) & g_sleep_wksrc_en){
			wkbit = wk_msg[i].wk_en_bit;
			if(wkbit >= 32){
				wkbit -= 32;
				if(wk_pd1 & (1<<wkbit))
					break;
			}else{
				if(wk_pd0 & (1<<wkbit))
					break;

			}
		}
	}
	if(i != SLEEP_WKSRC_NUM){
		g_sleep_wksrc_src = wk_msg[i].wksrc;
		printk("wksrc=%s\n", wk_msg[i].wksrc_msg);
	}else{
		printk("no wksrc\n");
		g_sleep_wksrc_src = 0;
	}

	return g_sleep_wksrc_src;

}
static void sys_set_wksrc_before_sleep(void)
{
	int i;
	uint32_t wk_en0, wk_en1;

	//printk("NVIC_ISPR0-1=0x%x,0x%x\n", sys_read32(NVIC_ISPR0), sys_read32(NVIC_ISPR1));
	//printk("NVIC_ISER0-1=0x%x,0x%x\n", sys_read32(NVIC_ISER0), sys_read32(NVIC_ISER1));
	//printk("NVIC_IABR0-1=0x%x,0x%x\n", sys_read32(NVIC_IABR0), sys_read32(NVIC_IABR1));
	//printk("g_sleep_wksrc_en =0x%x\n", g_sleep_wksrc_en);
	sys_write32(sys_read32(NVIC_ISER0), NVIC_ICER0);
	sys_write32(sys_read32(NVIC_ISER1), NVIC_ICER1);
	sys_write32(0xffffffff, NVIC_ICPR0);
	sys_write32(0xffffffff, NVIC_ICPR1);
	if(g_sleep_wksrc_en){
		wk_en0 = wk_en1 = 0;
		for(i = 0; i < SLEEP_WKSRC_NUM; i++){
			if((1 << wk_msg[i].wksrc) & g_sleep_wksrc_en){
				printk("%d wksrc=%s \n",i, wk_msg[i].wksrc_msg);
				if(wk_msg[i].wk_en_bit >= 32){
					wk_en1 |=  1 << (wk_msg[i].wk_en_bit-32);
				}else{
					wk_en0 |=  1 << (wk_msg[i].wk_en_bit);
				}
			}
		}
		if(wk_en0)
			sys_write32(wk_en0, NVIC_ISER0);
		if(wk_en1)
			sys_write32(wk_en1, NVIC_ISER1);
	}
	//printk("NVIC_ISPR0-1=0x%x,0x%x\n", sys_read32(NVIC_ISPR0), sys_read32(NVIC_ISPR1));
	//printk("NVIC_ISER0-1=0x%x,0x%x\n", sys_read32(NVIC_ISER0), sys_read32(NVIC_ISER1));
	//printk("NVIC_IABR0-1=0x%x,0x%x\n", sys_read32(NVIC_IABR0), sys_read32(NVIC_IABR1));

}

static uint32_t s2_reg_backups[ARRAY_SIZE(backup_regs_addr)];

static void sys_pm_backup_registers(void)
{
	int i;


	//ram_power_backup = sys_read32(PWRGATE_RAM);

	for (i = 0; i < ARRAY_SIZE(backup_regs_addr); i++)
		s2_reg_backups[i] = sys_read32(backup_regs_addr[i]);
}

 static void sys_pm_restore_registers(void)
{

	int i;
	for (i = ARRAY_SIZE(backup_regs_addr) - 1; i >= 0; i--)
		sys_write32(s2_reg_backups[i], backup_regs_addr[i]);
}



static struct mem_backup s2_mem_save[MEM_S2_ITEM_MAX];
static uint8_t save_num_item;
static uint32_t save_mem_len;
static void sys_sleep_retention_mem(void)
{
	struct mem_backup *bk;
	unsigned int i, len = 0;;
	char *sbuf = (char *)_image_ram_end; 
	for(i = 0; i < save_num_item; i++) {
		bk = &s2_mem_save[i];
		if((bk->buf == NULL) || (bk->len == 0))
			continue;
		
		len += bk->len;
		memcpy(sbuf, bk->buf, bk->len);
		sbuf += bk->len;
	}
	save_mem_len = len;
	printk("retention mem len=%d, num=%d\n", len, save_num_item);
}

static void sys_wakeup_recover_mem(void)
{
	struct mem_backup *bk;
	unsigned int i, len = 0;;
	char *sbuf = (char *)_image_ram_end; 
	for(i = 0; i < save_num_item; i++) {
		bk = &s2_mem_save[i];
		if((bk->buf == NULL) || (bk->len == 0))
			continue;		
		len += bk->len;
		memcpy(bk->buf, sbuf, bk->len);
		sbuf += bk->len;
		bk->buf = NULL;
		bk->len = 0;
	}
	printk("wakeup mem len=%d, num=%d\n", len, save_num_item);
	save_num_item = 0;
}

void dump_reg(const char *promt)
{

	printk("%s: reg dump\n", promt);
#if 0
	printk("RMU_MRCR0=0x%x\n", sys_read32(RMU_MRCR0));
	printk("RMU_MRCR1=0x%x\n", sys_read32(RMU_MRCR1));
	printk("CMU_DEVCLKEN0=0x%x\n", sys_read32(CMU_DEVCLKEN0));
	printk("CMU_DEVCLKEN1=0x%x\n", sys_read32(CMU_DEVCLKEN1));
	printk("PMU_DET=0x%x\n", sys_read32(PMU_DET));
	printk("CMU_S1CLKCTL=0x%x\n", sys_read32(CMU_S1CLKCTL));
	printk("CMU_S1BTCLKCTL=0x%x\n", sys_read32(CMU_S1BTCLKCTL));
	printk("CMU_S2HCLKCTL=0x%x\n", sys_read32(CMU_S2HCLKCTL));
	printk("CMU_S2SCLKCTL=0x%x\n", sys_read32(CMU_S2SCLKCTL));
	printk("CMU_S3CLKCTL=0x%x\n", sys_read32(CMU_S3CLKCTL));
	printk("CMU_PMUWKUPCLKCTL=0x%x\n", sys_read32(CMU_PMUWKUPCLKCTL));
	printk("HOSCOK_CTL=0x%x\n", sys_read32(HOSCOK_CTL));
	printk("RC4M_CTL=0x%x\n", sys_read32(RC4M_CTL));
	printk("WIO0_CTL=0x%x\n", sys_read32(WIO0_CTL));
	printk("PMUINTMASK=0x%x\n", sys_read32(PMU_INTMASK));
	printk("WKEN_CTL_SVCC=0x%x\n", sys_read32(WKEN_CTL_SVCC));
#endif
	printk("PWRGATE_DIG=0x%x\n", sys_read32(PWRGATE_DIG));
	printk("CMU_MEMCLKSRC0=0x%x\n", sys_read32(CMU_MEMCLKSRC0));
	printk("CMU_MEMCLKSRC1=0x%x\n", sys_read32(CMU_MEMCLKSRC1));
	printk("CMU_MEMCLKEN0=0x%x\n", sys_read32(CMU_MEMCLKEN0));
	printk("CMU_MEMCLKEN1=0x%x\n", sys_read32(CMU_MEMCLKEN1));
}



static uint32_t pwgate_dis_bak;

void powergate_prepare_sleep(int isdeep)
{
	uint32_t sram_end, use, num, reg_val, i,reg_clk;

	printk("PWRGATE_DIG_ACK=0x%x\n", sys_read32(PWRGATE_DIG_ACK));

	#if 0
	pwgate_dis_bak = sys_read32(PWRGATE_DIG);
	if(isdeep)
		reg_val = (pwgate_dis_bak & 0xfffff) | 0xb0700000;//m4f & bt on & spicache & io & aduio power on
	else
		reg_val = (pwgate_dis_bak &0xfffff) | 0xa8700000; // bt off
	sys_write32(reg_val, PWRGATE_DIG);
	#endif

}

void powergate_prepare_wakeup(void)
{
#if 0
	int i;
	uint32_t reg_val;
	reg_val = sys_read32(PWRGATE_RAM);
	for(i = 0; i < 17; i++) {
		if(pwgate_ram_bak & (1<<i)) {
			if(!(reg_val & (1<<i))) {
				sys_set_bit(PWRGATE_RAM, i);
				udelay_loop(2);
			}	
		}
	}
	sys_write32(pwgate_dis_bak, PWRGATE_DIG);
#endif
	sys_write32((sys_read32(CMU_MEMCLKSRC0)&~(0x7<<5))|(0x1<<5) , CMU_MEMCLKSRC0);// share ram clk switch to hosc
}



static void soc_cmu_sleep_prepare(int isdeep)
{
	sys_sleep_retention_mem();
	sys_pm_backup_registers();
	powergate_prepare_sleep(isdeep);
	SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk ;  // deepsleep
	/*spi0 clk switch to hosc*/
	sys_write32(0x0, CMU_SPI0CLK);	
	sys_write32(sys_read32(CMU_MEMCLKSRC0)&~(0x7<<5) , CMU_MEMCLKSRC0);// share ram clk switch to RC4M

	sys_set_wksrc_before_sleep();
}

#if defined(CONFIG_BOARD_NANDBOOT) || !defined(CONFIG_SPI_FLASH_ACTS)
void  sys_norflash_power_ctrl(uint32_t is_powerdown)
{

}
#else
extern  void  sys_norflash_power_ctrl(uint32_t is_powerdown);
#endif


#ifdef CONFIG_DISABLE_IRQ_STAT
static inline  unsigned int n_irq_lock(void)
{
	unsigned int key;
	unsigned int tmp;
	__asm__ volatile(
		"mov %1, %2;"
		"mrs %0, BASEPRI;"
		"msr BASEPRI, %1;"
		"isb;"
		: "=r"(key), "=r"(tmp)
		: "i"(_EXC_IRQ_DEFAULT_PRIO)
		: "memory");

	return key;
}

static inline  void n_irq_unlock(unsigned int key)
{
	__asm__ volatile(
		"msr BASEPRI, %0;"
		"isb;"
		:  : "r"(key) : "memory");
}
#endif


__ramfunc static void cpu_enter_sleep(void)
{
#if 1
	volatile int loop;
	uint32_t corepll_backup;

	//jtag_enable();

	//sys_norflash_power_ctrl(1);/*nor enter deep power down */

	/* switch cpu to rc4m */
	sys_write32(0x3, CMU_S1CLKCTL); // rc4m+hosc

	corepll_backup = sys_read32(COREPLL_CTL);
	sys_write32(0x0, CMU_SYSCLK); /*cpu clk select rc4M*/	
	sys_write32(sys_read32(COREPLL_CTL) & ~(1 << 7), COREPLL_CTL);
	sys_write32(0, COREPLL_CTL);

	/*spi0 & spi1 cache disable*/
	sys_clear_bit(SPICACHE_CTL, 0); //bit0 disable spi 0 cache

	sys_clear_bit(AVDDLDO_CTL, 0);  /*disable avdd, corepll use must enable*/
	loop=100;
	while(loop)loop--;	
	

	/*enter sleep*/
	__asm__ volatile("cpsid	i");
#ifdef CONFIG_DISABLE_IRQ_STAT
	n_irq_unlock(0);
#else
	irq_unlock(0);
#endif		
	__asm__ volatile("dsb");
	__asm__ volatile("wfi");
#ifdef CONFIG_DISABLE_IRQ_STAT
	n_irq_lock();
#else
    irq_lock();
#endif
	__asm__ volatile("cpsie	i");
	sys_set_bit(AVDDLDO_CTL, 0);  /*enable avdd, for pll*/

	loop=300;
	while(loop)loop--; /*for avdd*/

	/*spi0 & spi1 cache enable*/
	sys_set_bit(SPICACHE_CTL, 0); //enable spi 0 cache

	sys_write32(corepll_backup, COREPLL_CTL);
	
	sys_write32(0x0, CMU_SPI0CLK);// hosc
	loop=200;
	while(loop--); /*for avdd*/

	//sys_norflash_power_ctrl(0);//;//nor exit deep power down
	
#else
	k_busy_wait(2000000); //wati 2s

#endif

}


static void soc_cmu_sleep_exit(void)
{
	SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk ; 
	powergate_prepare_wakeup();
	sys_pm_restore_registers();	
	sys_wakeup_recover_mem();	
}


static int check_exit_deep_sleep(void)
{
	uint32_t wksrc;
	wksrc = sys_sleep_check_wksrc();

	return 1;
}
void soc_enter_deep_sleep(void)
{

	z_clock_set_timeout(CONFIG_SYS_CLOCK_TICKS_PER_SEC*5, false);
	sys_s3_wksrc_set(SLEEP_WK_SRC_BT);
	sys_s3_wksrc_set(SLEEP_WK_SRC_T1);
	sys_s3_wksrc_set(SLEEP_WK_SRC_TWS);
	sys_s3_wksrc_set(SLEEP_WK_SRC_PMU);
	suspend_check();
	do{
		dump_reg("before");
		soc_cmu_sleep_prepare(1);
		dump_reg("middle");
		cpu_enter_sleep();//wfi,enter to sleep
		soc_cmu_sleep_exit();
		dump_reg("BT after");
		check_exit_deep_sleep();
	}while(0);
	wakeup_check();

}

void soc_enter_light_sleep(void)
{
	z_clock_set_timeout(CONFIG_SYS_CLOCK_TICKS_PER_SEC*5, false);
	dump_reg("before");  	
	soc_cmu_sleep_prepare(0);	
	cpu_enter_sleep();//wfi,enter to sleep 
	soc_cmu_sleep_exit();	
	dump_reg("after");
}


int sleep_mem_save(void *buf, unsigned int len)
{
	if(save_num_item >= MEM_S2_ITEM_MAX) {
		printk("sleep:error: save mem fail\n");
		return -1;
	}
	s2_mem_save[save_num_item].buf = buf;
	s2_mem_save[save_num_item].len = len;
	save_num_item++;
	return 0;
}


