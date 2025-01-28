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

#if 0
#define MEM_S2_ITEM_MAX	10
struct mem_backup {
	void *buf;
	uint32_t len;
};

/* COREPLL RW fields range from 0 t0 7 */
//static uint32_t ram_power_backup;
/* Save and restore the registers */
static const uint32_t backup_regs_addr[] = {
	CMU_SPI0CLK,
	PMUADC_CTL,
	CMU_MEMCLKEN0,
	CMU_MEMCLKEN1,
	NVIC_ISER0,
	NVIC_ISER1,
	PWRGATE_DIG,
};

#define GPION_CTL(n)  (GPIO_REG_BASE+4+(n)*4)

/*gpio36-48, GPIO0-3 ,6,7 handle in sleep fun*/
/*gpio not use int sleep*/
static const uint32_t backup_regs_gpio[] = {
	/*misc */
	//GPION_CTL(4), /*not use defaut highz*/
	//GPION_CTL(22), /*not use defaut highz*/
	//GPION_CTL(23), /*not use defaut highz*/

	/*spinand */
	GPION_CTL(8),
	GPION_CTL(9),
	GPION_CTL(10),
	GPION_CTL(11),
	GPION_CTL(12),
	GPION_CTL(13),
	//GPION_CTL(64), /*power enable, use in sleep*/
	/*LCD */
	//GPION_CTL(5), /*lcd backlight enable*/
	GPION_CTL(14),
	GPION_CTL(15),
	GPION_CTL(16),
	GPION_CTL(17),
	GPION_CTL(30),
	GPION_CTL(34),
	GPION_CTL(35),

	/*sensor*/
	//GPION_CTL(18), /*not use defaut highz*/
	//GPION_CTL(19), /*EN_NTC. user in sleep*/
	//GPION_CTL(20), /*not use defaut highz*/
	//GPION_CTL(21), /*sensor irq ,use in sleep*/
	//GPION_CTL(24), /* HR_PWR_EN ,use in sleep*/
	//GPION_CTL(25) /*VDD1.8 eanble ,use in sleep*/
	/*TP*/
	//GPION_CTL(26), /*not use defaut highz*/
	//GPION_CTL(27), /*not use，defaut highz*/
	/*debug uart0 */
	//GPION_CTL(28), /*use in sleep*/
	//GPION_CTL(29), /*use in sleep*/


};



#define SP_IN_SLEEP (CONFIG_SRAM_BASE_ADDRESS+CONFIG_SRAM_SIZE*1024 - 4)
#define SRAM_SAVE_BASE_ADDR 	((uint32_t)(&__kernel_ram_start))
#define SRAM_SAVE_END_ADDR 	((uint32_t)(&__kernel_ram_save_end))
#define SRAM_SAVE_LEN		(SRAM_SAVE_END_ADDR - SRAM_SAVE_BASE_ADDR)

#define SENSOR_CODE_MAX_LEN		(SP_IN_SLEEP - SRAM_SAVE_BASE_ADDR-1024)   /*1024 is for sp*/

#define PSRAM_SAVE_MAXLEN 0x10000
static uint32_t psram_bak_sram[PSRAM_SAVE_MAXLEN/4];
static uint32_t psram_check_sum;

static void *sensor_code_addr;
static uint32_t sensor_code_len;


int sleep_sensor_code_set(void *code_addr, uint32_t code_len)
{
	printk("sensor code:base=0x%x, maxlen=0x%x, code_len=0x%x\n",
			SRAM_SAVE_BASE_ADDR, SENSOR_CODE_MAX_LEN,  code_len);
	if((code_addr == NULL) || (code_len > SENSOR_CODE_MAX_LEN)){
		printk("sensor code invalid\n");
		return -1;
	}
	sensor_code_addr = code_addr;
	sensor_code_len = code_len;
	return 0;
}

static uint32_t check_sum(uint32_t *buf, int len)
{
	int i;
	uint32_t chk = 0;
	for(i = 0; i < (len/4); i++)
		chk += buf[i];
	return chk;
}
static void suspend_check(void)
{

	uint32_t chk;
	chk = check_sum((uint32_t *)psram_bak_sram, PSRAM_SAVE_MAXLEN);
	printk("psram suspend chk=0x%x\n", chk);
	psram_check_sum = chk;

}

static void wakeup_check(void)
{
	uint32_t chk;
	chk = check_sum((uint32_t *)psram_bak_sram, PSRAM_SAVE_MAXLEN);
	printk("psram wakeup chk=0x%x, save=0x%x\n", chk, psram_check_sum);
	if(psram_check_sum != chk)
		printk("-------error psram check-----------\n");

}

static void mem_sram_save_to_psram(void)
{
	memcpy(psram_bak_sram, (void*)SRAM_SAVE_BASE_ADDR, SRAM_SAVE_LEN);
	printk("save addr=0x%x len=0x%x\n", SRAM_SAVE_BASE_ADDR, SRAM_SAVE_LEN);
	suspend_check();
	if((sensor_code_addr != NULL) && sensor_code_len){
		printk("copy sensor code 0x%x to sram\n", sensor_code_len);
		memcpy((void*)SRAM_SAVE_BASE_ADDR, sensor_code_addr, sensor_code_len);
	}else{
		printk("set sram base=0x%x, L=0x%x\n", SRAM_SAVE_BASE_ADDR, SENSOR_CODE_MAX_LEN);
		memset((void*)SRAM_SAVE_BASE_ADDR, 0, SENSOR_CODE_MAX_LEN);
	}
}

static void mem_psram_recovery_tosram(void)
{
	memcpy((void*)SRAM_SAVE_BASE_ADDR, psram_bak_sram, SRAM_SAVE_LEN);
	wakeup_check();
}


struct sleep_wk_data {
	uint16_t wksrc;
	uint16_t wk_en_bit;
	const char *wksrc_msg;
};

struct sleep_wk_cb {
	sleep_wk_callback_t wk_cb;
	enum S_WK_SRC_TYPE src;
};



const struct sleep_wk_data wk_msg[] = {
	{SLEEP_WK_SRC_BT, 		IRQ_ID_BT,  	"BT" },
	{SLEEP_WK_SRC_GPIO, 	IRQ_ID_GPIO,    "GPIO" },
	{SLEEP_WK_SRC_PMU, 		IRQ_ID_PMU,  	"PMU" },
	{SLEEP_WK_SRC_T0,		IRQ_ID_TIMER0, 	"T0" },
	{SLEEP_WK_SRC_T1,		IRQ_ID_TIMER1,	"T1" },
	{SLEEP_WK_SRC_T2,		IRQ_ID_TIMER2,	"T2" },
	{SLEEP_WK_SRC_T3,		IRQ_ID_TIMER3,	"T3" },
	{SLEEP_WK_SRC_TWS,		IRQ_ID_TWS, 	"TWS" },
	{SLEEP_WK_SRC_SPI0MT,	IRQ_ID_SPI0MT,	"SPI0MT" },
	{SLEEP_WK_SRC_SPI1MT,	IRQ_ID_SPI1MT,	"SPI1MT" },
	{SLEEP_WK_SRC_IIC0MT,	IRQ_ID_IIC0MT,	"IIC0MT" },
	{SLEEP_WK_SRC_IIC1MT,	IRQ_ID_IIC1MT,	"IIC1MT" },
};
#define SLEEP_WKSRC_NUM ARRAY_SIZE(wk_msg)

static struct sleep_wk_fun_data __act_s2_sleep_data *g_wk_fun[SLEEP_WKSRC_NUM] ;
static volatile uint16_t __act_s2_sleep_data g_sleep_wksrc_en;
static volatile uint32_t __act_s2_sleep_data g_sleep_cycle, g_wk_cnt;

static enum S_WK_SRC_TYPE g_sleep_wksrc_src;


int sleep_register_wk_callback(enum S_WK_SRC_TYPE wk_src, struct sleep_wk_fun_data *fn_data)

{
	if(fn_data == NULL)
		return -1;
	fn_data->next = g_wk_fun[wk_src];
	g_wk_fun[wk_src] = fn_data;
	return 0;
}


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
static uint32_t s2_gpio_reg_backups[ARRAY_SIZE(backup_regs_gpio)];


static void sys_pm_backup_registers(void)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(backup_regs_gpio); i++){ // set gpio highz
		s2_gpio_reg_backups[i] = sys_read32(backup_regs_gpio[i]);
		sys_write32(0x1000, backup_regs_gpio[i]);
	}


	for (i = 0; i < ARRAY_SIZE(backup_regs_addr); i++)
		s2_reg_backups[i] = sys_read32(backup_regs_addr[i]);
}

 static void sys_pm_restore_registers(void)
{

	int i;
	for (i = ARRAY_SIZE(backup_regs_gpio) - 1; i >= 0; i--)
		sys_write32(s2_gpio_reg_backups[i], backup_regs_gpio[i]);

	for (i = ARRAY_SIZE(backup_regs_addr) - 1; i >= 0; i--)
		sys_write32(s2_reg_backups[i], backup_regs_addr[i]);
}


void dump_reg(const char *promt)
{

#if 0

	int i;
	for (i = 0; i < 77 ; i++)// nor pin
		printk("gpio%d=0x%x\n", i, sys_read32(GPION_CTL(i)));

	printk("RMU_MRCR0=0x%x\n", sys_read32(RMU_MRCR0));
	printk("RMU_MRCR1=0x%x\n", sys_read32(RMU_MRCR1));
	printk("CMU_DEVCLKEN0=0x%x\n", sys_read32(CMU_DEVCLKEN0));
	printk("CMU_DEVCLKEN1=0x%x\n", sys_read32(CMU_DEVCLKEN1));
	printk("PMU_DET=0x%x\n", sys_read32(PMU_DET));
	printk("NVIC_ISPR0=0x%x\n", sys_read32(NVIC_ISPR0));
	printk("NVIC_ISPR1=0x%x\n", sys_read32(NVIC_ISPR1));
	printk("CMU_MEMCLKEN0=0x%x\n", sys_read32(CMU_MEMCLKEN0));
	printk("CMU_MEMCLKEN1=0x%x\n", sys_read32(CMU_MEMCLKEN1));
	printk("CMU_MEMCLKSRC0=0x%x\n", sys_read32(CMU_MEMCLKSRC0));
	printk("CMU_MEMCLKSRC1=0x%x\n", sys_read32(CMU_MEMCLKSRC1));
	printk("PWRGATE_DIG=0x%x\n", sys_read32(PWRGATE_DIG));
	printk("VOUT_CTL1_S2=0x%x\n", sys_read32(VOUT_CTL1_S2));
	printk("VOUT_CTL1_S3=0x%x\n", sys_read32(VOUT_CTL1_S3));
	printk("PWRGATE_RAM=0x%x\n", sys_read32(PWRGATE_RAM));
	printk("WIO0_CTL=0x%x\n", sys_read32(WIO0_CTL));
	printk("WIO1_CTL=0x%x\n", sys_read32(WIO1_CTL));
	printk("WIO2_CTL=0x%x\n", sys_read32(WIO2_CTL));
	printk("WIO3_CTL=0x%x\n", sys_read32(WIO3_CTL));
#endif

}

void powergate_prepare_sleep(int isdeep)
{
	/*
	RMU_MRCR0=0x1b0d5fb1
	RMU_MRCR1=0xc10c000c
	CMU_DEVCLKEN0=0x5b0413b1
	CMU_DEVCLKEN1=0x1f0c000c bit24-28 is bt
	*/
	sys_write32(0x0, PMUADC_CTL);

	//PWRGATE_DIG  bit20-aduio  bit21-de/se/usb bit25-adncdsp bit26-audiodsp bit27-bt-f bit28-btp
	if(isdeep){
		sys_write32(0x90000000, PWRGATE_DIG); // m4f/bt on   else off
	}else{
		sys_write32(0x88000000, PWRGATE_DIG); //m4f on, bt off  else off
	}
}

void powergate_prepare_wakeup(void)
{

}



static void soc_cmu_sleep_prepare(int isdeep)
{
	sys_pm_backup_registers();
	powergate_prepare_sleep(isdeep);
	SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk ;  // deepsleep
	/*spi0 clk switch to hosc*/
	sys_write32(0x0, CMU_SPI0CLK);
	sys_write32(0x0, CMU_GPIOCLKCTL); //select gpio clk RC32K
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

__sleepfunc uint32_t sleep_get_cycle(void)
{
	return sys_read32(T2_CNT)+g_sleep_cycle;
}

__sleepfunc static void __cpu_enter_sleep(int isdeep)
{
#if 1
	volatile int loop;
	uint32_t corepll_backup, st, end, sysclk_bak;
	uint32_t  gpio_nor[6];/*gpio0-3, gpio6-7*/

	//jtag_enable();
	/*bak nor gpio */
	for(st = 0; st < 4; st++) {
		gpio_nor[st] = sys_read32(GPION_CTL(st));
		sys_write32(0x1000, GPION_CTL(st));
	}
	gpio_nor[st++] = sys_read32(GPION_CTL(6));
	gpio_nor[st++] = sys_read32(GPION_CTL(7));
	sys_write32(0x1000, GPION_CTL(6));
	sys_write32(0x1000, GPION_CTL(7));


	sys_write32(0x4, CMU_TIMER2CLK); //select rc32k before enter S3
	st = sys_read32(T2_CNT);

	sysclk_bak =  sys_read32(CMU_SYSCLK);
	corepll_backup = sys_read32(COREPLL_CTL);
	sys_write32(0x0, CMU_SYSCLK); /*cpu clk select rc4M*/
	sys_write32(sys_read32(COREPLL_CTL) & ~(1 << 7), COREPLL_CTL);
	sys_write32(0, COREPLL_CTL);

	/*spi0  cache disable*/
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

	/*spi0  cache enable*/
	sys_set_bit(SPICACHE_CTL, 0); //enable spi 0 cache

	sys_write32(corepll_backup, COREPLL_CTL);
	sys_write32(0x0, CMU_SPI0CLK);// hosc

	end =  sys_read32(T2_CNT);
	st = end-st-1;
	sys_write32(0x0, CMU_TIMER2CLK); //select hosc for k_cycle_get
	g_sleep_cycle += st*(CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC/32768);
	sys_write32(sysclk_bak, CMU_SYSCLK);

	/*recovery nor gpio */
	for(st = 0; st < 4; st++) {
		sys_write32(gpio_nor[st], GPION_CTL(st));
	}
	sys_write32(gpio_nor[st++], GPION_CTL(6));
	sys_write32(gpio_nor[st++] , GPION_CTL(7));

#else
	k_busy_wait(2000000); //wati 2s

#endif

}

static inline  void set_PSP(unsigned int p_sp)
{
	__asm__ volatile(
		"msr psp, %0;"
		:  : "r"(p_sp) : "memory");
}

__sleepfunc static enum WK_CB_RC check_wk_run_sram_nor(uint16_t *wk_en_bit,
				struct sleep_wk_cb *cb, int *cb_num)
{
	int i;
	enum WK_CB_RC rc = WK_CB_SLEEP_AGAIN;
	enum WK_RUN_TYPE runt;
	uint32_t wk_pd0, wk_pd1,wkbit;
	bool b_nor_wk = false;
	struct sleep_wk_fun_data *p;
	wk_pd0 = sys_read32(NVIC_ISPR0);
	wk_pd1 = sys_read32(NVIC_ISPR1);
	*cb_num = 0;

	for(i = 0; i < SLEEP_WKSRC_NUM; i++){
		if(!((1 << i) & g_sleep_wksrc_en))
			continue;

		wkbit = wk_en_bit[i];
		if(wkbit >= 32){
			wkbit -= 32;
			if(!(wk_pd1 & (1<<wkbit)))
				continue;

		}else{
			if(!(wk_pd0 & (1<<wkbit)))
				continue;
		}
		if(g_wk_fun[i]){
			p = g_wk_fun[i];
			do{
				if(p->wk_prep){
					runt = p->wk_prep(i);//
					if(runt == WK_RUN_IN_SRAM){
						if(WK_CB_RUN_SYSTEM == p->wk_cb(i)) //要求唤醒到系统继续运行
							rc = WK_CB_RUN_SYSTEM;
					}else if (runt == WK_RUN_IN_NOR) {// 唤醒nor 跑
						if(!b_nor_wk){//nor 退出idle
							b_nor_wk = true;
							sys_norflash_power_ctrl(0);
						}
						if(WK_CB_RUN_SYSTEM == p->wk_cb(i)) //要求唤醒到系统继续运行
							rc = WK_CB_RUN_SYSTEM;

					}else{
						rc = WK_CB_RUN_SYSTEM; /*系统跑*/
						if(*cb_num < SLEEP_WKSRC_NUM){
							cb[*cb_num].wk_cb = p->wk_cb;
							cb[*cb_num].src = i;
							(*cb_num)++;
						}
					}

				}else{
					rc = WK_CB_RUN_SYSTEM; /*系统跑*/
					if(*cb_num < SLEEP_WKSRC_NUM){
						cb[*cb_num].wk_cb = p->wk_cb;
						cb[*cb_num].src = i;
						(*cb_num)++;
					}
				}
				p = p->next;
			}while(p);
		}else{
			rc = WK_CB_RUN_SYSTEM; /*not wake up callback , sytem handle*/
		}

	}

	if(rc == WK_CB_SLEEP_AGAIN){
		sys_write32(0xffffffff, NVIC_ICPR0);
		sys_write32(0xffffffff, NVIC_ICPR1);
		if(b_nor_wk){ // 如果继续休眠，但是nor已经退出idle，重新进idle
			sys_norflash_power_ctrl(1);
		}
	}else{
		if(!b_nor_wk){// 如果系统跑，还没有退出idle，退出idle
			sys_norflash_power_ctrl(0);
		}
	}
	return rc;
}

static struct sleep_wk_cb g_syste_cb[SLEEP_WKSRC_NUM];
static int g_num_cb;

__sleepfunc static void cpu_enter_sleep_tmp(int isdeep)
{
	unsigned int i, num_cb;
	uint16_t wk_en_bit[SLEEP_WKSRC_NUM];
	struct sleep_wk_cb cb[SLEEP_WKSRC_NUM];
	uint32_t bk_clksrc0;
	uint32_t  gpio_psram[13];/*gpio36-48*/

	mem_sram_save_to_psram();

	for(i = 0; i < SLEEP_WKSRC_NUM; i++) // copy nor to sram, nor is not use for wake first
		wk_en_bit[i] = wk_msg[i].wk_en_bit;

	sys_norflash_power_ctrl(1);/*nor enter deep power down */
	g_wk_cnt = 0;
	while(1) {
		//RAM4 shareRAM select  RC4MHZ in s2)

		bk_clksrc0 = sys_read32(CMU_MEMCLKSRC0);
		sys_write32((bk_clksrc0 & (~0x3ff)) | (0x0<<5), CMU_MEMCLKSRC0);

		for(i = 0; i < 13; i++){ //bak psram pin and set highz
			gpio_psram[i] = sys_read32(GPION_CTL(i+36));
			sys_write32(0x1000, GPION_CTL(i+36));
		}
		sys_clear_bit(SPI1_CACHE_CTL, 0); //bit0 disable spi 1 cache

		__cpu_enter_sleep(isdeep);

		for(i = 0; i < 13; i++){ // recovery psram pin
			sys_write32(gpio_psram[i], GPION_CTL(i+36));
		}
		sys_set_bit(SPI1_CACHE_CTL, 0); //bit0 enable spi 1 cache

		sys_write32(bk_clksrc0, CMU_MEMCLKSRC0); // recovery
		g_wk_cnt++;
		if(WK_CB_RUN_SYSTEM == check_wk_run_sram_nor(wk_en_bit, cb, &num_cb))
			break;

	}
	g_num_cb = num_cb;
	memcpy(g_syste_cb, cb, num_cb*sizeof(struct sleep_wk_cb));
	mem_psram_recovery_tosram();

}
static void cpu_enter_sleep(int isdeep)
{
	static unsigned int sp;
	sp = __get_PSP();
	set_PSP(SP_IN_SLEEP); //change sp to sram top
	cpu_enter_sleep_tmp(isdeep);
	set_PSP(sp);  // recovery sp
}



static void wakeup_system_callback(void)
{
	int i;
	for(i = 0; i < g_num_cb; i++){
		g_syste_cb[i].wk_cb(g_syste_cb[i].src);
	}
	g_num_cb = 0;
}


static void soc_cmu_sleep_exit(void)
{
	SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk ;
	powergate_prepare_wakeup();
	sys_pm_restore_registers();
	sys_write32(0x1, CMU_GPIOCLKCTL); //select gpio clk RC4M
}


static void soc_pmu_onoff_wk_set(void)
{
	//sys_write32(0x301, WIO0_CTL); // onoff
	sys_write32(0x3, WIO0_CTL);
	sys_write32(sys_read32(PMU_INTMASK) | (1<<1), PMU_INTMASK); //ONOFF SHORT WAKEUP
	k_busy_wait(300);
	printk("PMUINTMASK=0X%X\n", sys_read32(PMU_INTMASK));
	sys_s3_wksrc_set(SLEEP_WK_SRC_PMU);
}


static int check_exit_deep_sleep(void)
{
	uint32_t wksrc;
	wksrc = sys_sleep_check_wksrc();
	//if(wksrc == 0){
		//return check_exit_by_wio();
	//}

	return 1;
}

#ifdef CONFIG_BOARD_LARK_DVB_EARPHONE
#define soc_enter_sleep_switch_uart(x)		NULL
#else
static const struct acts_pin_config bt_pin_cfg_uart[] = {
	{28, 23 | GPIO_CTL_PADDRV_LEVEL(1) | GPIO_CTL_PULLUP},
	{29, 23 | GPIO_CTL_PADDRV_LEVEL(1) | GPIO_CTL_PULLUP},
};

static const struct acts_pin_config lark_pin_cfg_uart[] = {
	{28, 5 | GPIO_CTL_PADDRV_LEVEL(1) | GPIO_CTL_PULLUP},
	{29, 5 | GPIO_CTL_PADDRV_LEVEL(1) | GPIO_CTL_PULLUP},
};

static void soc_enter_sleep_switch_uart(bool bt_uart)
{
	if (bt_uart) {
		acts_pinmux_setup_pins(bt_pin_cfg_uart, ARRAY_SIZE(bt_pin_cfg_uart));
	} else {
		acts_pinmux_setup_pins(lark_pin_cfg_uart, ARRAY_SIZE(lark_pin_cfg_uart));
	}
}
#endif


void soc_enter_deep_sleep(void)
{
	unsigned int cyc ,end;
	sys_clock_set_timeout(CONFIG_SYS_CLOCK_TICKS_PER_SEC, false);
	sys_s3_wksrc_set(SLEEP_WK_SRC_BT);
	dump_reg("before");
	soc_pmu_onoff_wk_set();
	soc_cmu_sleep_prepare(1);
	cyc = sleep_get_cycle();
	soc_enter_sleep_switch_uart(true);
	cpu_enter_sleep(1);//wfi,enter to sleep
	soc_enter_sleep_switch_uart(false);
	end = sleep_get_cycle();
	printk("\n\n---sleep %d ms--cnt=%d--\n", k_cyc_to_ms_ceil32(end-cyc), g_wk_cnt);
	soc_cmu_sleep_exit();
	check_exit_deep_sleep();
	dump_reg("BT after");
	wakeup_system_callback();

}

void soc_enter_light_sleep(void)
{
	sys_clock_set_timeout(CONFIG_SYS_CLOCK_TICKS_PER_SEC*5, false);
	dump_reg("before");
	soc_cmu_sleep_prepare(0);
	cpu_enter_sleep(0);//wfi,enter to sleep
	soc_cmu_sleep_exit();
	dump_reg("after");
}


static int soc_sleep_init(const struct device *arg)
{
	int i;
	g_sleep_cycle = 0;
	g_sleep_wksrc_en = 0;
	printk("g_sleep_wksrc_en =0x%x\n", g_sleep_wksrc_en);
	for(i = 0; i < SLEEP_WKSRC_NUM; i++)
		g_wk_fun[i] = NULL;
	return 0;
}

SYS_INIT(soc_sleep_init, PRE_KERNEL_1, 20);

#else
void soc_enter_deep_sleep(void)
{

}
#endif


