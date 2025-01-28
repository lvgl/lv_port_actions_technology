/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file soc_sleep.c  sleep  for Actions SoC
 */


#include <zephyr.h>
#include <board.h>
#include <arch/arm/aarch32/cortex_m/cmsis.h>
#include <string.h>
#include <drivers/timer/system_timer.h>
#include <linker/linker-defs.h>
#include <partition/partition.h>
#include "spicache.h"
#include "sys_wakelock.h"
#include <act_arm_mpu.h>

#define HOSC_CTL_READY_SHIFT        28
#define HOSC_CTL_READY_MASK         (0x1 << HOSC_CTL_READY_SHIFT)

//#define CONFIG_SLEEP_PRINTK_DEBUG

//#define CONFIG_SLEEP_STAGE_DEBUG

//#define CONFIG_SLEEP_DISABLE_BT

//#define CONFIG_JTAG_DEBUG

//#define CONFIG_SLEEP_WD_DIS_DEBUG

//#define CONFIG_SLEEP_DUMP_INFO

/*sleep mode*/
#define SLEEP_MODE_NORMAL  		0	 /*normal sleep*/
#define SLEEP_MODE_BT_OFF 		1    /*sleep bt power off*/
#define SLEEP_MODE_LOWPOWER 	2	 /*sleep bt power off, only onoff wakeup*/
#define SLEEP_MODE_FORCE_SLEEP 	3	 /*force sleep for power test*/

#define FORCE_SLEEP_WAKE_CNT 	20	 /*auto wakeup after CNT*200ms */

#define PSRAM_START_ADDR                (0x38000000)
#define MEMORY_CHECK_INTEGRITY_SIZE     (8 * 1024 * 1024)

struct sleep_wk_data {
	uint16_t wksrc;
	uint16_t irq_en_bit;   /* wksrc settings only need irq_en for version-A */
	uint16_t wksrc_en_bit; /* wksrc additional settings for version-B */
	const char *wksrc_msg;
};

static const struct sleep_wk_data wk_msg[] = {
	{SLEEP_WK_SRC_PMU, 		IRQ_ID_PMU,  	WK_ID_PMU,    "PMU" },
	{SLEEP_WK_SRC_RTC,      IRQ_ID_RTC,     WK_ID_RTC,    "RTC" },
	{SLEEP_WK_SRC_BT, 		IRQ_ID_BT,  	WK_ID_BT,     "BT" },
	{SLEEP_WK_SRC_GPIO, 	IRQ_ID_GPIO,    WK_ID_GPIO,   "GPIO" },
	{SLEEP_WK_SRC_T0,		IRQ_ID_TIMER0, 	WK_ID_TIMER0, "T0" },
	{SLEEP_WK_SRC_T1,		IRQ_ID_TIMER1,	WK_ID_TIMER1, "T1" },
	{SLEEP_WK_SRC_T2,		IRQ_ID_TIMER2,	WK_ID_TIMER2, "T2" },
	{SLEEP_WK_SRC_T3,		IRQ_ID_TIMER3,	WK_ID_TIMER3, "T3" },
	{SLEEP_WK_SRC_T4,		IRQ_ID_TIMER4,	WK_ID_TIMER4, "T4" },
	{SLEEP_WK_SRC_T5,		IRQ_ID_TIMER5,	WK_ID_TIMER5, "T5" },
	{SLEEP_WK_SRC_TWS,		IRQ_ID_TWS, 	WK_ID_TWS0,    "TWS" },
	{SLEEP_WK_SRC_SPI0MT,	IRQ_ID_SPI0MT,	WK_ID_SPI0MT, "SPI0MT" },
	{SLEEP_WK_SRC_SPI1MT,	IRQ_ID_SPI1MT,	WK_ID_SPI1MT, "SPI1MT" },
	{SLEEP_WK_SRC_IIC0MT,	IRQ_ID_IIC0MT,	WK_ID_IIC0MT, "IIC0MT" },
	{SLEEP_WK_SRC_IIC1MT,	IRQ_ID_IIC1MT,	WK_ID_IIC1MT, "IIC1MT" },
};

#define SLEEP_WKSRC_NUM ARRAY_SIZE(wk_msg)

struct sleep_wk_cb {
	sleep_wk_callback_t wk_cb;
	enum S_WK_SRC_TYPE src;
};

/* sleep context structure */
struct sleep_context_t {
	uint32_t sleep_mode; /*control sleep mode*/
	uint32_t g_sleep_cycle;
	enum S_WK_SRC_TYPE g_sleep_wksrc_src;

	struct sleep_wk_cb g_syste_cb[SLEEP_WKSRC_NUM];
	uint32_t g_num_cb;

#ifdef CONFIG_SLEEP_MEMORY_CHECK_INTEGRITY
	uint32_t check_start_addr;
	uint32_t check_len;
	uint32_t check_sum;
	uint32_t resume_sum;
#endif
};

static struct sleep_context_t sleep_context_obj;
#define current_sleep (&sleep_context_obj)

static struct sleep_wk_fun_data __act_s2_sleep_data *g_wk_fun[SLEEP_WKSRC_NUM] ;

static volatile uint16_t __act_s2_sleep_data g_sleep_wksrc_en, g_sleep_wksrc_en_bak;
static int64_t __act_s2_sleep_data g_sleep_update_time, g_sleep_ms;
static uint32_t __act_s2_sleep_data g_sleep_st, check_cnt, g_sleep_t2cnt;

#define SOC_DEEP_SLEEP_BACKUP_NUM (16)
/* backup context: R0 - R14 except R12(ip) and R15(pc) ; MSP + PSP */
uint32_t __act_s2_sleep_data soc_sleep_backup[SOC_DEEP_SLEEP_BACKUP_NUM];
static uint32_t __act_s2_sleep_data pwrgat_irq_en0, pwrgat_irq_en1;
static uint32_t __act_s2_sleep_data pwrgat_wksrc_en;
static uint32_t __act_s2_sleep_data wk_pd0, wk_pd1;

void __cpu_suspend();
void __cpu_resume();
void arm_floating_point_init(void);

/* Save and restore the registers */
static const uint32_t backup_regs_addr[] = {
	CMU_S1CLKCTL,
	PMUADC_CTL,
	CMU_MEMCLKEN0,
	CMU_MEMCLKEN1,
	NVIC_ISER0,
	NVIC_ISER1,
	PWRGATE_DIG,
	CMU_SPI0CLK,
};

static uint32_t __act_s2_sleep_data s2_reg_backups[ARRAY_SIZE(backup_regs_addr)];

/*gpio36-48, GPIO0-3 ,6,7 handle in sleep fun*/
/*gpio not use int sleep*/
static const uint32_t backup_regs_gpio[] = {


#if CONFIG_SPINAND_3 || CONFIG_SPI_FLASH_2
	/*spinand */
	GPION_CTL(8),
	GPION_CTL(9),
	GPION_CTL(10),
	GPION_CTL(11),
	GPION_CTL(12),
	GPION_CTL(13),
#endif

#ifndef CONFIG_SENSOR_MANAGER
	/*g_sensor*/
	GPION_CTL(54), /*int*/
	GPION_CTL(55), /*i2cmt1 clk*/
	GPION_CTL(56), /*i2cmt1 dat*/
	/*heart_rate meter*/
	GPION_CTL(18), /*i2cmt0 clk*/
	GPION_CTL(19), /*i2cmt0 dat*/
	GPION_CTL(74), /*int*/
#endif

#if 0
	/*sensor*/
	//GPION_CTL(20), /*not use defaut highz*/
	GPION_CTL(21), /*sensor irq ,use in sleep*/
	//GPION_CTL(24), /* HR_PWR_EN ,use in sleep*/
	GPION_CTL(25), /*VDD1.8 eanble ,use in sleep*/
	GPION_CTL(33), /*GPS wake up Host ,use in sleep*/
#endif

#ifdef SLEEP_GPIO_REG_SET_HIGHZ
	SLEEP_GPIO_REG_SET_HIGHZ
#endif

#if IS_ENABLED(CONFIG_KNOB_ENCODER)
	SLEEP_KNOB_REG_SET_HIGHZ
#endif
};

#ifdef SLEEP_AOD_GPIO_REG_UNSET_HIGHZ
static const uint32_t backup_regs_aod_gpio[] = {
	SLEEP_AOD_GPIO_REG_UNSET_HIGHZ
};
#endif

__sleepfunc bool sleep_policy_is_pwrgat(void)
{
#ifdef CONFIG_CPU_PWRGAT
	if (soc_dvfs_opt())
		return true;
	else
		return false;
#else
	return false;
#endif
}

static uint32_t __act_s2_sleep_data s2_gpio_reg_backups[ARRAY_SIZE(backup_regs_gpio)];

#if defined(CONFIG_BOARD_NANDBOOT) || !defined(CONFIG_SPI_FLASH_ACTS)
void  sys_norflash_power_ctrl(uint32_t is_powerdown)
{
}
#else
extern void sys_norflash_power_ctrl(uint32_t is_powerdown);
#endif

/*g_cyc2ms_mul = (1000<<16) / soc_rc32K_freq() / */
static uint32_t __act_s2_sleep_data g_cyc2ms_mul;  //

static __sleepfunc uint32_t rc32k_cyc_to_ms(uint32_t cycle)
{
	//return (uint32_t)((uint64_t)cycle * 1000 / soc_rc32K_freq());
	uint64_t tmp = g_cyc2ms_mul;
	tmp = tmp * cycle;
	return (tmp >> 16);
}

__sleepfunc int uart_out_ch(int c, void *ctx)
{
	ARG_UNUSED(ctx);

	/* send a character */
	if(c == '\n'){
		/* Wait for transmitter to be ready */
		while (sys_read32(UART0_REG_BASE + 12) &  BIT(6));
		sys_write32('\r', UART0_REG_BASE + 8);
	}

	/* Wait for transmitter to be ready */
	while (sys_read32(UART0_REG_BASE + 12) &  BIT(6));
	sys_write32(c, UART0_REG_BASE+8);

	return 0;
}

__sleepfunc void uart_out_flush(void)
{
	/* Wait for transmitter complete */
	while (sys_read32(UART0_REG_BASE + 12) &  BIT(21));
}

__sleepfunc int sl_printk(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	sl_vprintk(fmt, ap);
	va_end(ap);

	return 0;
}

void uart_resume(void);
static void pwrgat_gpio_uart_restore_registers(void);
#ifndef CONFIG_SLEEP_PRINTK_DEBUG
static void soc_enter_sleep_switch_uart(bool bt_uart);
#endif
extern int check_panic_exe(void);

__sleepfunc int sl_vprintk(const char *fmt, va_list ap)
{
#ifdef CONFIG_SLEEP_PRINTK_DEBUG
	uint32_t print_enable = 1;
#else
	uint32_t print_enable = 0;

#ifdef CONFIG_ACTIONS_PRINTK_DMA
	if (check_panic_exe()) {
		if (!sys_test_bit(CMU_S1CLKCTL, 1)) {
			/* hosc enable for uart */
			sys_set_bit(CMU_S1CLKCTL, 1);
			sys_set_bit(CMU_S3CLKCTL, 1);
			soc_udelay(1000);
			
			/* restore uart reg and pin */
			pwrgat_gpio_uart_restore_registers();
			uart_resume();
			soc_enter_sleep_switch_uart(false);
		}
		print_enable = 1;
	}
#endif
#endif
	if (print_enable) {
		pbrom_libc_api->p_cbvprintf(uart_out_ch, NULL, fmt, ap);
		uart_out_flush();
	}

	return 0;
}

#ifdef CONFIG_SLEEP_STAGE_DEBUG
__sleepfunc void sleep_stage(unsigned int index)
{
	 unsigned int val,stage;
	 stage = (index&0x7f)+0x80;
	 val = sys_read32(RTC_REMAIN2)& (~(0xff<<8));
	 sys_write32( val | (stage << 8), RTC_REMAIN2);
}

#else
void sleep_stage(int index)
{

}
#endif

void dump_stage_timer(uint32_t st, uint32_t end)
{
#ifdef CONFIG_SLEEP_DUMP_INFO
	printk("sleep use %d ms, check_cnt=%d\n", rc32k_cyc_to_ms(end - st), check_cnt);
#endif
}

#ifdef CONFIG_SLEEP_MEMORY_CHECK_INTEGRITY
/* check_sum should be in SRAM when checking 8M psram */
static uint32_t __act_s2_sleep_data suspend_sum;

static uint32_t check_sum(uint32_t *buf, int len)
{
	int i;
	uint32_t chk = 0;

	for (i = 0; i < (len / 4); i++)
		chk += buf[i];

	return chk;
}

static void suspend_check(void)
{
	uint32_t chk;
	chk = check_sum((uint32_t *)current_sleep->check_start_addr,
					current_sleep->check_len);
	suspend_sum = chk;

	//printk("suspend checksum:0x%x\n", chk);
}

static void resume_check(void)
{
	uint32_t chk;
	chk = check_sum((uint32_t *)current_sleep->check_start_addr,
					current_sleep->check_len);
	current_sleep->resume_sum = chk;
}

static void wakeup_check_dump(void)
{
	//uint32_t chk;

	//chk = check_sum((uint32_t *)current_sleep->check_start_addr,
					//current_sleep->check_len);
	printk("checksum resume:0x%x suspend:0x%x,len=0x%x\n", current_sleep->resume_sum, suspend_sum, current_sleep->check_len);

	if (suspend_sum != current_sleep->resume_sum){
		printk("\n----- error memory integrity check -----------\n");
		//while(1);
	}
}
#endif

int sleep_register_wk_callback(enum S_WK_SRC_TYPE wk_src, struct sleep_wk_fun_data *fn_data)
{
	if (fn_data == NULL)
		return -1;

	fn_data->next = g_wk_fun[wk_src];
	g_wk_fun[wk_src] = fn_data;

	return 0;
}

void sys_s3_wksrc_set(enum S_WK_SRC_TYPE src)
{
	g_sleep_wksrc_en |= (1 << src);
}

void sys_s3_wksrc_clr(enum S_WK_SRC_TYPE src)
{
	g_sleep_wksrc_en &= ~(1 << src);
}

enum S_WK_SRC_TYPE sys_s3_wksrc_get(void)
{
	return current_sleep->g_sleep_wksrc_src;
}

void sys_s3_wksrc_init(void)
{
	current_sleep->g_sleep_wksrc_src = SLEEP_WK_SRC_T1;
}

static enum S_WK_SRC_TYPE sys_sleep_check_wksrc(void)
{
	int i;
	uint32_t wkbit;

	current_sleep->g_sleep_wksrc_src = 0;

	//printk("WK NVIC_ISPR0=0x%x\n", wk_pd0);
	//printk("WK NVIC_ISPR1=0x%x\n", wk_pd1);

	for (i = 0; i < SLEEP_WKSRC_NUM; i++) {
		if ((1 << wk_msg[i].wksrc) & g_sleep_wksrc_en) {
			wkbit = wk_msg[i].irq_en_bit;
			if (wkbit >= 32) {
				wkbit -= 32;
				if (wk_pd1 & (1 << wkbit))
					break;
			} else {
				if (wk_pd0 & (1 << wkbit))
					break;
			}
		}
	}

	if (i != SLEEP_WKSRC_NUM) {
		current_sleep->g_sleep_wksrc_src = wk_msg[i].wksrc;
		sl_dbg("wksrc=%s\n", wk_msg[i].wksrc_msg);
	} else {
		sl_dbg("no wksrc\n");
		current_sleep->g_sleep_wksrc_src = 0;
	}

	return current_sleep->g_sleep_wksrc_src;
}

static void sys_set_wksrc_by_sleep_mode(void)
{
	if (current_sleep->sleep_mode != SLEEP_MODE_NORMAL) {
		g_sleep_wksrc_en_bak = g_sleep_wksrc_en;
	}

	switch (current_sleep->sleep_mode) {
		case SLEEP_MODE_BT_OFF:
			g_sleep_wksrc_en = (1 << SLEEP_WK_SRC_PMU) | (1 << SLEEP_WK_SRC_IIC1MT);
			break;
		case SLEEP_MODE_LOWPOWER:
			g_sleep_wksrc_en = (1 << SLEEP_WK_SRC_PMU);
			break;
		case SLEEP_MODE_FORCE_SLEEP:
			g_sleep_wksrc_en = (1 << SLEEP_WK_SRC_IIC1MT);
			break;
		default:
			break;
	}
}

static int sys_get_bt_pwr_by_sleep_mode(void)
{
	int bt_pwr = 1;

	switch (current_sleep->sleep_mode) {
		case SLEEP_MODE_BT_OFF:
		case SLEEP_MODE_LOWPOWER:
			bt_pwr = 0;
			break;
		default:
			break;
	}

	return bt_pwr;
}

static enum WK_CB_RC sys_update_wk_by_sleep_mode(enum WK_CB_RC rc)
{
	static uint32_t s_sleep_cnt = 0;

#if FORCE_SLEEP_WAKE_CNT > 0
	if (current_sleep->sleep_mode == SLEEP_MODE_FORCE_SLEEP) {
		s_sleep_cnt ++;
		rc = WK_CB_SLEEP_AGAIN;
		if (s_sleep_cnt >= FORCE_SLEEP_WAKE_CNT) {
			s_sleep_cnt = 0;
			rc = WK_CB_RUN_SYSTEM;
		}
	}
#endif

	return rc;
}

static void sys_exit_wk_by_sleep_mode(void)
{
	if (current_sleep->sleep_mode != SLEEP_MODE_NORMAL) {
		if (current_sleep->sleep_mode == SLEEP_MODE_FORCE_SLEEP) {
			current_sleep->sleep_mode = SLEEP_MODE_NORMAL;
		}
		g_sleep_wksrc_en = g_sleep_wksrc_en_bak;
#ifdef CONFIG_SYS_WAKELOCK
		sys_wakelocks_enable(1);
#endif
	}
}

static void sys_set_wksrc_before_sleep(void)
{
	int i;
	uint32_t irq_en0, irq_en1;
	uint32_t wksrc_en;

	sys_set_wksrc_by_sleep_mode();

	sys_write32(sys_read32(NVIC_ISER0), NVIC_ICER0);
	sys_write32(sys_read32(NVIC_ISER1), NVIC_ICER1);
	//sys_write32(0xffffffff, NVIC_ICPR0);
	//sys_write32(0xffffffff, NVIC_ICPR1);

	if (g_sleep_wksrc_en) {

		irq_en0 = irq_en1 = wksrc_en = 0;

		for (i = 0; i < SLEEP_WKSRC_NUM; i++) {
			if ((1 << wk_msg[i].wksrc) & g_sleep_wksrc_en){

				sl_dbg("%d wksrc=%s \n",i, wk_msg[i].wksrc_msg);

				if (wk_msg[i].irq_en_bit >= 32) {
					irq_en1 |=  1 << (wk_msg[i].irq_en_bit - 32);
				} else {
					irq_en0 |=  1 << (wk_msg[i].irq_en_bit);
				}

				wksrc_en |= 1 << (wk_msg[i].wksrc_en_bit);
			}
		}

		if(irq_en0)
			sys_write32(irq_en0, NVIC_ISER0);

		if(irq_en1)
			sys_write32(irq_en1, NVIC_ISER1);

		/* wksrc additional settings for version-B */
		if (soc_dvfs_opt()) {
			sys_write32((sys_read32(MEMORYCTL2) & ~(0x1ff<<15)) | wksrc_en, MEMORYCTL2);
			pwrgat_wksrc_en = wksrc_en;
		}

		pwrgat_irq_en0 = irq_en0;
		pwrgat_irq_en1 = irq_en1;
	}

#if 0
	printk("NVIC_ISPR0-1=0x%x,0x%x\n", sys_read32(NVIC_ISPR0), sys_read32(NVIC_ISPR1));
	printk("NVIC_ISER0-1=0x%x,0x%x\n", sys_read32(NVIC_ISER0), sys_read32(NVIC_ISER1));
	printk("NVIC_IABR0-1=0x%x,0x%x\n", sys_read32(NVIC_IABR0), sys_read32(NVIC_IABR1));
#endif
}

__sleepfunc static void sys_restore_wksrc(void)
{
	if(pwrgat_irq_en0)
		sys_write32(pwrgat_irq_en0, NVIC_ISER0);

	if(pwrgat_irq_en1)
		sys_write32(pwrgat_irq_en1, NVIC_ISER1);

	if (soc_dvfs_opt() && pwrgat_wksrc_en)
		sys_write32((sys_read32(MEMORYCTL2) & ~(0x1ff<<15)) | pwrgat_wksrc_en, MEMORYCTL2);
}

#ifdef SLEEP_AOD_GPIO_REG_UNSET_HIGHZ
static uint32_t __act_s2_sleep_data s2_aod_gpio_reg_backups[ARRAY_SIZE(backup_regs_aod_gpio)];
#endif

static void sys_pm_backup_registers(void)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(backup_regs_gpio); i++){ /* set gpio highz */
		s2_gpio_reg_backups[i] = sys_read32(backup_regs_gpio[i]);
		sys_write32(0x1000, backup_regs_gpio[i]);
	}

#ifdef SLEEP_AOD_GPIO_REG_UNSET_HIGHZ
	if (soc_get_aod_mode() == 0) {
		for (i = 0; i < ARRAY_SIZE(backup_regs_aod_gpio); i++) { // set gpio highz
			s2_aod_gpio_reg_backups[i] = sys_read32(backup_regs_aod_gpio[i]);
			sys_write32(0x1000, backup_regs_aod_gpio[i]);
		}
	}
#endif

	for (i = 0; i < ARRAY_SIZE(backup_regs_addr); i++)
		s2_reg_backups[i] = sys_read32(backup_regs_addr[i]);
}

static void sys_pm_restore_registers(void)
{

	int i;
	for (i = ARRAY_SIZE(backup_regs_gpio) - 1; i >= 0; i--)
		sys_write32(s2_gpio_reg_backups[i], backup_regs_gpio[i]);

#ifdef SLEEP_AOD_GPIO_REG_UNSET_HIGHZ
	if (soc_get_aod_mode() == 0) {
		for (i = ARRAY_SIZE(backup_regs_aod_gpio) - 1; i >= 0; i--) {
			sys_write32(s2_aod_gpio_reg_backups[i], backup_regs_aod_gpio[i]);
		}
	}
#endif

	for (i = ARRAY_SIZE(backup_regs_addr) - 1; i >= 0; i--) {
		sys_write32(s2_reg_backups[i], backup_regs_addr[i]);
	}
}

/* Save and restore the icache registers */
static uint32_t __act_s2_sleep_data icache_backup_regs_addr[] = {
	SPI0_CTL,
	SPI0_DELAYCHAIN,
	SPICACHE_CTL,
	MEMORYCTL,
	MEMORYCTL2,
	MEMORYCTL3,
};

/* Save and restore the dcache registers */
static uint32_t __act_s2_sleep_data dcache_backup_regs_addr[] = {
	SPI1_CTL,
	SPI1_DDR_MODE_CTL,
	SPI1_DELAYCHAIN,
	SPI1_DQS1_DELAYCHAIN,
	SPI1_CACHE_CTL,
	CACHE_OPERATE_ADDR_START,
	CACHE_OPERATE_ADDR_END,
	SPI1_PSRAM_MAPPING_MISS_ADDR,
	SPI1_GPU_CTL,
};

enum pwrgat_gpio_pin {
	NOR_CS_PIN        = GPIO_0,
	NOR_PWR_PIN       = GPIO_64,
	PSRAM_CS_PIN      = GPIO_40,
	DEBUG_UART_TX_PIN = GPIO_28,
	DEBUG_UART_RX_PIN = GPIO_29,
};

enum ram_index {
	RAM0_IDX = 0,
	RAM1_IDX,
	RAM2_IDX,
	RAM3_IDX,
	RAM4_IDX,
	RAM5_IDX,
	RAM6_IDX,
	RAM7_IDX,
	RAM8_IDX,
	RAM9_IDX,
	RAM10_IDX,
	RAM11_IDX,
	RAM12_IDX,
	RAM_REGION_NUM,
};

#define  RAM_START_ADDR                       0x2FF60000
#define  RAM_STEP                             0x00010000
#define  DMAIE                                (DMA_REG_BASE  + 0x04)
#define  DMADEBUG                             (DMA_REG_BASE  + 0x80)
#define  SDMAIE                               (SDMA_REG_BASE + 0x04)
#define  SDMA_PRIORITY                        (SDMA_REG_BASE + 0x70)
#define  SDMADEBUG                            (SDMA_REG_BASE + 0x80)
#define  SDMA_COUPLE_REG_BASE                 (SDMA_REG_BASE + 0xa0)

#define  SPI_CACHE_MAPPING_BACKUP_REG_NUM     16
#define  NVIC_IPR_REG_NUM                     16
#define  DMA_NUM                              10
#define  SDMA_NUM                             5
#define  REG_NUM_PER_DMA                      7
#define  REG_NUM_SDMA_COUPLE                  6
#define  DMAX_REG_NUM                         (DMA_NUM * REG_NUM_PER_DMA)
#define  SDMAX_REG_NUM                        (SDMA_NUM * REG_NUM_PER_DMA)

/* RAM LS-DS */
#define RAM0_DS                               (0x1 << 0)
#define RAM1_12_DS                            (0xfff << 1)
#define RAM13_14_DS                           (0x3 << 13)
#define RAM15_16_DS                           (0x3 << 15)

/* CMU_MEMCLKEN0 */
#define ROM_CLK_EN                            (0x1 << 0)
#define RAM0_CLK_EN                           (0x1 << 1)
#define RAM1_12_CLK_EN                        (0x3fff << 2)
#define RAM15_16_CLK_EN                       (0xf << 20)
#define SRAM_CLK_EN                           (0xf << 24)
#define SPICACHE0_1_RAM_CLK_EN                (0xf << 28)

/* CMU_MEMCLKEN1 */
#define BTROM_RAM_CLK_EN                      (0x1 << 16)

static uint32_t __act_s2_sleep_data uart_ctl, uart_br;
static uint32_t __act_s2_sleep_data sleep_mode;
static uint32_t __act_s2_sleep_data spi_cache_mapping_backup[SPI_CACHE_MAPPING_BACKUP_REG_NUM];
static uint32_t __act_s2_sleep_data icache_backups[ARRAY_SIZE(icache_backup_regs_addr)];
static uint32_t __act_s2_sleep_data dcache_backups[ARRAY_SIZE(dcache_backup_regs_addr)];
static uint32_t __act_s2_sleep_data vector_tbloff_backup, nvic_ipr_backup[NVIC_IPR_REG_NUM];
/* dma backup */
static uint32_t __act_s2_sleep_data dmax_backup[DMAX_REG_NUM];
static uint32_t __act_s2_sleep_data dmaie_backup[3];
/* sdma backup */
static uint32_t __act_s2_sleep_data sdmaie_backup;
static uint32_t __act_s2_sleep_data sdmaprio_backup;
static uint32_t __act_s2_sleep_data sdmadebug_backup;
static uint32_t __act_s2_sleep_data sdma_couple_backup[REG_NUM_SDMA_COUPLE];
static uint32_t __act_s2_sleep_data sdmax_backup[SDMAX_REG_NUM];
/* psram/nor/uart pwrgat gpio backup */
static uint32_t __act_s2_sleep_data pwrgat_gpio_backups[5];

void uart_suspend(void)
{
	sleep_mode = current_sleep->sleep_mode;
	uart_ctl = sys_read32(UART0_REG_BASE);
	uart_br = sys_read32(UART0_REG_BASE + 0x10);
}

__sleepfunc void uart_resume(void)
{
	/* resume uart */
	sys_write32(sys_read32(RMU_MRCR0) | (1 << RESET_ID_UART0), RMU_MRCR0);
	sys_write32(sys_read32(CMU_DEVCLKEN0) | (1 << CLOCK_ID_UART0), CMU_DEVCLKEN0);
	sys_write32(uart_ctl, UART0_REG_BASE);
	sys_write32(uart_br, UART0_REG_BASE + 0x10);
}

static void icache_backup_registers(void)
{
	unsigned int i;

	for (i = 0; i < SPI_CACHE_MAPPING_BACKUP_REG_NUM; i++)
		spi_cache_mapping_backup[i] = sys_read32(SPI_CACHE_MAPPING_ADDR0 + 4 * i);

	for (i = 0; i < ARRAY_SIZE(icache_backup_regs_addr); i++)
		icache_backups[i] = sys_read32(icache_backup_regs_addr[i]);
}

static void dcache_backup_registers(void)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(dcache_backup_regs_addr); i++)
		dcache_backups[i] = sys_read32(dcache_backup_regs_addr[i]);
}


__sleepfunc static void icache_restore_registers(void)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(icache_backup_regs_addr); i++)
		sys_write32(icache_backups[i], icache_backup_regs_addr[i]);

	for (i = 0; i < SPI_CACHE_MAPPING_BACKUP_REG_NUM; i++)
		sys_write32(spi_cache_mapping_backup[i], SPI_CACHE_MAPPING_ADDR0 + 4 * i);
}

__sleepfunc static void dcache_restore_registers(void)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(dcache_backup_regs_addr); i++)
		sys_write32(dcache_backups[i], dcache_backup_regs_addr[i]);
}

static void nvic_backup_registers(void)
{
	unsigned int i;

	for (i = 0; i < NVIC_IPR_REG_NUM; i++)
		nvic_ipr_backup[i] = sys_read32(NVIC_IPR0 + 4 * i);

	vector_tbloff_backup = SCB->VTOR;
}

__sleepfunc static void nvic_restore_registers(void)
{
	unsigned int i;

	SCB->VTOR = vector_tbloff_backup;

	for (i = 0; i < NVIC_IPR_REG_NUM; i++)
		sys_write32(nvic_ipr_backup[i], NVIC_IPR0 + 4 * i);
}

__sleepfunc static void nvic_save_pending_register(void)
{
	wk_pd0 = sys_read32(NVIC_ISPR0);
	wk_pd1 = sys_read32(NVIC_ISPR1);
}

static void dma_backup_registers(void)
{
	unsigned int i, j;

	for (i = 0; i < 2; i++) {
		/* DMAIP DMAIE */
		dmaie_backup[i] = sys_read32(DMA_REG_BASE + 4 * i);
	}

	/* DMADEBUG */
	dmaie_backup[2] = sys_read32(DMADEBUG);

	for (i = 0; i < DMA_NUM; i++) {
		for (j = 0; j < REG_NUM_PER_DMA; j++) {
			/* DMAxCTL~DMAxBC */
			dmax_backup[i*REG_NUM_PER_DMA + j] = sys_read32(DMA_REG_BASE + 0x100*(i+1) + 4*j);
		}
	}
}

__sleepfunc static void dma_restore_registers(void)
{
	unsigned int i, j;

	for (i = 0; i < DMA_NUM; i++) {
		for (j = 0; j < REG_NUM_PER_DMA; j++) {
			/* DMAxCTL~DMAxBC */
			if(j != 1)  // DMA_START
				sys_write32(dmax_backup[i*REG_NUM_PER_DMA + j], DMA_REG_BASE + 0x100*(i+1) + 4*j);
		}
	}

	for (i = 0; i < DMA_NUM; i++) //DMA_START last recovery
		sys_write32(dmax_backup[i*REG_NUM_PER_DMA + 1], DMA_REG_BASE + 0x100*(i+1) + 4);

	sys_write32(dmaie_backup[2], DMADEBUG);
	sys_write32(dmaie_backup[1], DMAIE);
}

static void sdma_backup_registers(void)
{
	unsigned int i, j;

	sdmaie_backup    = sys_read32(SDMAIE);
	sdmaprio_backup  = sys_read32(SDMA_PRIORITY);
	sdmadebug_backup = sys_read32(SDMADEBUG);

	for (i = 0; i < REG_NUM_SDMA_COUPLE; i++)
		sdma_couple_backup[i] = sys_read32(SDMA_COUPLE_REG_BASE + 4*i);

	for (i = 0; i < SDMA_NUM; i++) {
		for (j = 0; j < REG_NUM_PER_DMA; j++) {
			/* SDMAxCTL~SDMAxBC */
			sdmax_backup[i*REG_NUM_PER_DMA + j] = sys_read32(SDMA_REG_BASE + 0x100*(i+1) + 4*j);
		}
	}
}

__sleepfunc static void sdma_restore_registers(void)
{
	unsigned int i, j;

	for (i = 0; i < SDMA_NUM; i++) {
		for (j = 0; j < REG_NUM_PER_DMA; j++) {
			/* SDMAxCTL~SDMAxBC */
			sys_write32(sdmax_backup[i*REG_NUM_PER_DMA + j], SDMA_REG_BASE + 0x100*(i+1) + 4*j);
		}
	}

	for (i = 0; i < REG_NUM_SDMA_COUPLE; i++)
		sys_write32(sdma_couple_backup[i], SDMA_COUPLE_REG_BASE + 4*i);

	sys_write32(sdmadebug_backup, SDMADEBUG);
	sys_write32(sdmaprio_backup,  SDMA_PRIORITY);
	sys_write32(sdmaie_backup,    SDMAIE);
}

__sleepfunc static void pwrgat_gpio_psram_backup_registers(void)
{
	/* make psram cs high */
	pwrgat_gpio_backups[0] = sys_read32(GPION_CTL(PSRAM_CS_PIN));
	sys_write32(1 << (PSRAM_CS_PIN % 32), GPION_BSR(PSRAM_CS_PIN));
	sys_write32(0x1040, GPION_CTL(PSRAM_CS_PIN));
	sys_write32(1 << (PSRAM_CS_PIN % 32), GPION_BSR(PSRAM_CS_PIN));
}

__sleepfunc static void pwrgat_gpio_nor_backup_registers(void)
{
	/* make nor cs high */
	pwrgat_gpio_backups[1] = sys_read32(GPION_CTL(NOR_CS_PIN));
	sys_write32(1 << (NOR_CS_PIN % 32), GPION_BSR(NOR_CS_PIN));
	sys_write32(0x1040, GPION_CTL(NOR_CS_PIN));
	sys_write32(1 << (NOR_CS_PIN % 32), GPION_BSR(NOR_CS_PIN));
}

__sleepfunc static void pwrgat_set_unused_uart_highz(void)
{
	/* if debug uart is not used, we need HighZ it.
	 * in pwrgat, need do it, otherwize consume more 180uA.
	 */
#ifdef CONFIG_SLEEP_DISABLE_BT
	sys_write32(0x1000, GPION_CTL(DEBUG_UART_TX_PIN));
	sys_write32(0x1000, GPION_CTL(DEBUG_UART_RX_PIN));
#endif
	if (sleep_mode != SLEEP_MODE_NORMAL) {
		sys_write32(0x1000, GPION_CTL(DEBUG_UART_TX_PIN));
		sys_write32(0x1000, GPION_CTL(DEBUG_UART_RX_PIN));
	}
}

__sleepfunc static void pwrgat_gpio_nor_restore_registers(void)
{
	sys_write32(pwrgat_gpio_backups[1], GPION_CTL(NOR_CS_PIN));
}

__sleepfunc static void pwrgat_gpio_psram_restore_registers(void)
{
	sys_write32(pwrgat_gpio_backups[0], GPION_CTL(PSRAM_CS_PIN));
}

__sleepfunc static void pwrgat_gpio_uart_restore_registers(void)
{
	sys_write32(pwrgat_gpio_backups[3], GPION_CTL(DEBUG_UART_TX_PIN));
	sys_write32(pwrgat_gpio_backups[4], GPION_CTL(DEBUG_UART_RX_PIN));
}

#ifdef CONFIG_SPI0_NOR_DTR_MODE
struct delaychain_tbl {
	uint16_t volt_mv;
	uint16_t delaychain;
};

static struct delaychain_tbl __act_s2_sleep_data delay_array[] = {
	{900, 19},  {950, 20},
	{1000, 22}, {1050, 23},
	{1100, 25}, {1150, 26},
	{1200, 28}
};

__sleepfunc static void nor_dtr_set_matched_delaychain(uint16_t vdd_mv)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(delay_array); i++) {
		if (vdd_mv == delay_array[i].volt_mv) {
			sys_write32((sys_read32(SPI0_DELAYCHAIN) & ~(0x3F)) \
				| delay_array[i].delaychain, SPI0_DELAYCHAIN);
		}
	}
}

__sleepfunc static void nor_dtr_backup_delaychain(void)
{
	icache_backups[1] = sys_read32(SPI0_DELAYCHAIN);
}
#endif

static void sys_pwrctrl_assigned_sram(bool is_pwrdown)
{
	uint32_t start_addr    = (uint32_t)_sleep_shutdown_ram_start;
	uint32_t end_addr      = (uint32_t)_sleep_shutdown_ram_end;
	uint16_t ram_start_idx = (start_addr - RAM_START_ADDR) / RAM_STEP;
	uint16_t ram_end_idx   = (end_addr   - RAM_START_ADDR) / RAM_STEP;
	uint32_t ram_sleep_reg = 0;
	uint32_t ram_clken_reg = 0;
	uint16_t i;

	if (start_addr < RAM_START_ADDR)
		return ;

	if (end_addr >= (RAM_START_ADDR + RAM_REGION_NUM * RAM_STEP))
		return ;

	/* ram region between ram_start_idx and ram_end_idx need be pwrdown or resume */
	for (i = ram_start_idx + 1; i < ram_end_idx; i++) {
		/* ram pwrdown available bit */
		ram_sleep_reg |= (0x1 << i);

		/* ram clk available bit */
		if (i <= RAM11_IDX)
			ram_clken_reg |= (0x1 << (i + 1));
		else if (i < RAM_REGION_NUM)
			ram_clken_reg |= (0x1 << (RAM11_IDX + 1 + 2*(i - RAM11_IDX)));
	}

	if (is_pwrdown) {
		/* shut down ram */
		sys_write32(sys_read32(RAM_LIGHTSLEEP) | ram_sleep_reg, RAM_LIGHTSLEEP);
		sys_write32(sys_read32(RAM_DEEPSLEEP)  | ram_sleep_reg, RAM_DEEPSLEEP);
		sys_write32(sys_read32(PWRGATE_RAM)    | ram_sleep_reg, PWRGATE_RAM);

		/* close ram clk  */
		sys_write32(sys_read32(CMU_MEMCLKEN0)  & ~ram_clken_reg, CMU_MEMCLKEN0);
	}
	else {
		/* open ram clk  */
		sys_write32(sys_read32(CMU_MEMCLKEN0)  | ram_clken_reg, CMU_MEMCLKEN0);

		/* restore ram */
		sys_write32(sys_read32(PWRGATE_RAM)    & ~ram_sleep_reg, PWRGATE_RAM);
		sys_write32(sys_read32(RAM_DEEPSLEEP)  & ~ram_sleep_reg, RAM_DEEPSLEEP);
		sys_write32(sys_read32(RAM_LIGHTSLEEP) & ~ram_sleep_reg, RAM_LIGHTSLEEP);
	}
}

static enum WK_CB_RC wakeup_system_callback(void)
{
	int i;
	enum WK_CB_RC rc;

	if (current_sleep->g_num_cb) {
		rc = WK_CB_SLEEP_AGAIN;

		printk("wake call fun=%d\n", current_sleep->g_num_cb);

		for (i = 0; i < current_sleep->g_num_cb; i++){

			printk("call wksrc=%d fun\n", current_sleep->g_syste_cb[i].src);

			if (current_sleep->g_syste_cb[i].wk_cb(current_sleep->g_syste_cb[i].src)
				== WK_CB_RUN_SYSTEM)  /* need run system */
				rc = WK_CB_RUN_SYSTEM;
		}

		current_sleep->g_num_cb = 0;
	} else {
		rc = WK_CB_RUN_SYSTEM;
	}

	return rc;
}

static void soc_pmu_onoff_wk_set(void)
{
	//sys_write32(0x3, WIO0_CTL);
	sys_write32(sys_read32(PMU_INTMASK) | (1 << 1), PMU_INTMASK); /* ONOFF SHORT WAKEUP */
	sys_s3_wksrc_set(SLEEP_WK_SRC_PMU);
	//printk("PMUINTMASK=0X%X\n", sys_read32(PMU_INTMASK));
}

//#define CONFIG_GPIO_WAKEUP_TEST
#ifdef CONFIG_GPIO_WAKEUP_TEST
#define GPIO_N_WK  GPIO_21
//#define WIO_N_WK	WIO_1
static void soc_gpio_wakeup_test(void)
{
#ifdef GPIO_N_WK
	printk("gpio=%d wakeup test \n", GPIO_N_WK);
	sys_write32(GPIO_CTL_GPIO_INEN|GPIO_CTL_SMIT|GPIO_CTL_PULLUP| GPIO_CTL_INTC_EN | GPIO_CTL_INTC_MASK |
					GPIO_CTL_INC_TRIGGER_RISING_EDGE|GPIO_CTL_PADDRV_LEVEL(3), GPION_CTL(GPIO_N_WK));

#else
	printk("wio =%d wakeup test \n", WIO_N_WK);
	sys_write32(GPIO_CTL_GPIO_INEN|GPIO_CTL_SMIT|GPIO_CTL_PULLUP| GPIO_CTL_INTC_EN | GPIO_CTL_INTC_MASK |
					GPIO_CTL_INC_TRIGGER_RISING_EDGE|GPIO_CTL_PADDRV_LEVEL(3), WIO_REG_CTL(WIO_N_WK));
#endif
	sys_s3_wksrc_set(SLEEP_WK_SRC_GPIO);
}
static void soc_gpio_check(void)
{
#ifdef GPIO_N_WK
	printk("*******gpio=%d check----\n", GPIO_N_WK);
	if(sys_read32(GPIO_REG_IRQ_PD(GPIO_REG_BASE, GPIO_N_WK)) & GPIO_BIT(GPIO_N_WK)){
		printk("*******gpio=%d wakeup----\n", GPIO_N_WK);
		sys_write32(GPIO_BIT(GPIO_N_WK), GPIO_REG_IRQ_PD(GPIO_REG_BASE, GPIO_N_WK));
	}
#else
	printk("*******wigo=%d check----\n", WIO_N_WK);
	if(sys_read32(WIO_REG_CTL(WIO_N_WK)) & WIO_CTL_INT_PD_MASK){
		printk("*******wio=%d wakeup----\n", WIO_N_WK);
		sys_write32(WIO_REG_CTL(WIO_N_WK), sys_read32(WIO_REG_CTL(WIO_N_WK)));
	}
#endif
}
#endif

static enum WK_CB_RC soc_cmu_sleep_exit(void)
{
	SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk;
#ifdef CONFIG_GPIO_WAKEUP_TEST
	soc_gpio_check();
#endif
	sys_pm_restore_registers();
	sys_sleep_check_wksrc();
#ifdef CONFIG_SLEEP_MEMORY_CHECK_INTEGRITY
	wakeup_check_dump();
#endif
	sys_exit_wk_by_sleep_mode();
	return wakeup_system_callback();
}

static void soc_cmu_sleep_prepare(int force_bt_pg)
{
	sys_pm_backup_registers();  /* backup reg */
	sys_s3_wksrc_set(SLEEP_WK_SRC_BT); /* set bt wakeup src */
	soc_pmu_onoff_wk_set(); /* set onoff wakeup src */
#ifdef CONFIG_GPIO_WAKEUP_TEST
	soc_gpio_wakeup_test();
#endif
	/**
	 * RMU_MRCR0=0x1b0d5fb1
	 * RMU_MRCR1=0xc10c000c
	 * CMU_DEVCLKEN0=0x5b0413b1
	 * CMU_DEVCLKEN1=0x1f0c000c bit24-28 is bt
	 */
	sys_write32(0x0, PMUADC_CTL);// close ADC

	/* pwrgat peripheral: DISPLAY_PG/GPU_PG/DSP_AU_PG */
	sys_write32(sys_read32(PWRGATE_DIG)
				& ~(1 << PWRGATE_DIG_DISPLAY_PG)
				& ~(1 << PWRGATE_DIG_GPU_PG)
				& ~(1 << PWRGATE_DIG_DSP_AU_PG), PWRGATE_DIG);

	/* force bt pwrgate in debug mode(CONFIG_SLEEP_DISABLE_BT=y or sleepmode1/2) */
	if (force_bt_pg == 0) {
		sys_write32(sys_read32(RMU_MRCR1) & ~(1 << (RESET_ID_BT-32)), RMU_MRCR1); /* disable bluetooth hub */ 

		/* pwrgat peripheral: BT_PG(BT_FORCE) */
		sys_write32((sys_read32(PWRGATE_DIG) | (1 << PWRGATE_DIG_BT_FORCE))
					& ~(1 << PWRGATE_DIG_BT_PG), PWRGATE_DIG);
	}

	/*spi0 clk switch to hosc*/
	sys_write32(sys_read32(CMU_SPI0CLK) & ~(0x3 << 8) & ~(0xf << 0), CMU_SPI0CLK);

	sys_set_wksrc_before_sleep(); /* init wakeup src */

	SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk ;  /* deepsleep */


}

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

__sleepfunc static void __cpu_enter_sleep(void)
{
	uint32_t devclk[2];
	uint32_t val;
	uint32_t wd_ctl;
	uint32_t reg_trace_bak;
#ifdef CONFIG_SPI0_NOR_DTR_MODE
	uint32_t spi0_delaychain_bak;
#endif

	//jtag_enable();
	/*spi0  cache disable*/
	//sys_clear_bit(SPICACHE_CTL, 0);

	sys_write32((1<<CLOCK_ID_EXINT), CMU_DEVCLKEN0);

	sys_write32(0x9f000000 |  (1 << (CLOCK_ID_I2CMT1 - 32)), CMU_DEVCLKEN1);
	sys_write32(sys_read32(CMU_DEVCLKEN1) |
				(1 << (CLOCK_ID_TIMER0 - 32)) |
				(1 << (CLOCK_ID_TIMER1 - 32)) |
				(1 << (CLOCK_ID_TIMER2 - 32)) |
				(1 << (CLOCK_ID_TIMER3 - 32)) |
				(1 << (CLOCK_ID_TIMER4 - 32)) |
				(1 << (CLOCK_ID_TIMER5 - 32)), CMU_DEVCLKEN1);
	sys_write32(0x0, CMU_GPIOCLKCTL); /* select gpio clk RC32K */

	devclk[0] = sys_read32(CMU_DEVCLKEN0);
	devclk[1] = sys_read32(CMU_DEVCLKEN1);

	/* RC64M enable for spi0 clk switch to rc64m*/
	sys_set_bit(CMU_S1CLKCTL,2);
	/*cpu clk select rc4M*/
	if (soc_dvfs_opt())
		sys_write32(0x0, CMU_SYSCLK);
	else
		sys_write32(0x02000230, CMU_SYSCLK);

	/* NOR use RC64M */
	sys_write32((sys_read32(CMU_SPI0CLK) & ~(0x3 << 8) & ~(0xf << 0)) | 0x301, CMU_SPI0CLK);

#ifdef CONFIG_SPI0_NOR_DTR_MODE
	spi0_delaychain_bak = sys_read32(SPI0_DELAYCHAIN);
	nor_dtr_set_matched_delaychain(950);
#endif
	val = sys_read32(VOUT_CTL1_S1M);
	sys_write32((val & ~(0xfF)) | 0x88, VOUT_CTL1_S1M); //0.95

	soc_udelay(3);/*delay for cpuclk&spi0 clk switch ok*/

	/* RC64M + RC4M enable, hosc enable. hosc need be enabled to
	 * prevent vc18 no-output happening in hardware S1<=>S3.
	 */
	sys_write32(0x7, CMU_S1CLKCTL);

#ifdef CONFIG_SLEEP_PRINTK_DEBUG
	/* hosc enable for uart */
	sys_set_bit(CMU_S1CLKCTL, 1);
	sys_set_bit(CMU_S3CLKCTL, 1);
#endif

	if (sleep_policy_is_pwrgat()) {
		sys_write32((uint32_t)__cpu_resume, RTC_REMAIN4);
		sys_write32(0xb5, RTC_REMAIN5);
	}

	wd_ctl = sys_read32(WD_CTL);
	reg_trace_bak = sys_read32(0xe0043004); //trace reg bakup

	/* enable wdt in deep sleep if wdt enable */
	if(sys_test_bit(WD_CTL, 4))
		sys_set_bit(CMU_WDCLK, 1);

	/* enter sleep */
	__asm__ volatile("cpsid	i");
#ifdef CONFIG_DISABLE_IRQ_STAT
	n_irq_unlock(0);
#else
	irq_unlock(0);
#endif

	if (sleep_policy_is_pwrgat()) {
		__cpu_suspend();
	}
	else {
		__asm__ volatile("dsb");
		__asm__ volatile("wfi");
	}

#ifdef CONFIG_DISABLE_IRQ_STAT
	n_irq_lock();
#else
    irq_lock();
#endif
	__asm__ volatile("cpsie	i");

	/* save wakeup pending info for judging and printing wakeup src afterwards */
	nvic_save_pending_register();

	/* hosc disable for save power */
	sys_write32(0x5, CMU_S1CLKCTL);

	sys_write32(wd_ctl, WD_CTL);
	sys_write32(reg_trace_bak, 0xe0043004);// trace reg restore

#ifdef CONFIG_SLEEP_PRINTK_DEBUG
	/* hosc enable for uart */
	sys_set_bit(CMU_S1CLKCTL, 1);
	sys_set_bit(CMU_S3CLKCTL, 1);
#endif

#ifdef CONFIG_SPI0_NOR_DTR_MODE
	sys_write32(spi0_delaychain_bak, SPI0_DELAYCHAIN);
#endif
	sys_write32(val, VOUT_CTL1_S1M);
	soc_udelay(30);/*delay for vdd*/

	/* CPU USE RC64M */
	if (soc_dvfs_opt())
		sys_write32(0x03, CMU_SYSCLK);//AHB /2
	else
		sys_write32(0x203, CMU_SYSCLK);//AHB /1

	sys_write32(0x1, CMU_GPIOCLKCTL); /* select gpio clk RC4M */

	/* resume nor devclk for execute sensor algo, i2cmt0 for sampling heart rate data */
	sys_write32(devclk[0] | (1<<CLOCK_ID_SPI0CACHE) | (1<<CLOCK_ID_SPI0), CMU_DEVCLKEN0);
	sys_write32(devclk[1] | (1<<(CLOCK_ID_I2CMT0 - 32)), CMU_DEVCLKEN1);

	if(sys_test_bit(WD_CTL, 4))/*if wdt enable, feed wdt*/
		sys_set_bit(WD_CTL, 0);

#ifdef CONFIG_SLEEP_WD_DIS_DEBUG
	sys_clear_bit(WD_CTL, 4);
#endif

	g_sleep_t2cnt = sys_read32(T2_CNT);/*T2 counter before clock sync*/

}

__sleepfunc static enum WK_CB_RC check_wk_run_sram_nor(uint16_t *irq_en_bit,
				struct sleep_wk_cb *cb, int *cb_num)
{
	int i;
	enum WK_CB_RC rc = WK_CB_SLEEP_AGAIN;
	enum WK_CB_RC rc_t ;
	enum WK_RUN_TYPE runt;
	uint32_t wkbit;
	bool b_nor_wk = false;
	struct sleep_wk_fun_data *p;
	*cb_num = 0;

	for(i = 0; i < SLEEP_WKSRC_NUM; i++){
		if(!((1 << i) & g_sleep_wksrc_en))
			continue;

		wkbit = irq_en_bit[i];
		if(wkbit >= 32){
			if(!(wk_pd1 & (1<<(wkbit - 32))))
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
						rc_t = p->wk_cb(i);
						//if(WK_CB_RUN_SYSTEM == p->wk_cb(i))
							//rc = WK_CB_RUN_SYSTEM;
					}else if (runt == WK_RUN_IN_NOR) {
						if(!b_nor_wk){
							b_nor_wk = true;
						}
						rc_t = p->wk_cb(i);
						//if(WK_CB_RUN_SYSTEM == p->wk_cb(i))
							//rc = WK_CB_RUN_SYSTEM;

					}else{
						rc_t = WK_CB_RUN_SYSTEM;
						if(*cb_num < SLEEP_WKSRC_NUM){
							cb[*cb_num].wk_cb = p->wk_cb;
							cb[*cb_num].src = i;
							(*cb_num)++;
						}
					}
					if(rc_t == WK_CB_SLEEP_AGAIN){
						if(wkbit < 32)
							sys_write32(1<<wkbit, NVIC_ICPR0);
						else
							sys_write32(1<<(wkbit-32), NVIC_ICPR1);
					}

				}else{
					rc_t = WK_CB_RUN_SYSTEM;
					if(*cb_num < SLEEP_WKSRC_NUM){
						cb[*cb_num].wk_cb = p->wk_cb;
						cb[*cb_num].src = i;
						(*cb_num)++;
					}
				}
				p = p->next;
				if(rc_t == WK_CB_RUN_SYSTEM)// if wksrc need wake up system ,wakeup sysytem
					rc = WK_CB_RUN_SYSTEM;
			}while(p);

			if(rc_t == WK_CB_CARELESS) /*if  all callback not care of this wksrc, wakeup system handle*/
					rc = WK_CB_RUN_SYSTEM;

		}else{
			rc = WK_CB_RUN_SYSTEM; /*not wake up callback , system handle*/
		}

		if (wk_msg[i].wksrc == SLEEP_WK_SRC_IIC1MT) {
			rc = sys_update_wk_by_sleep_mode(rc);
		}
	}

	if(rc == WK_CB_SLEEP_AGAIN){
		//sys_write32(0xffffffff, NVIC_ICPR0);
		//sys_write32(0xffffffff, NVIC_ICPR1);
		if(b_nor_wk){
			sys_norflash_power_ctrl(1);
		}
	}
	return rc;
}

#ifndef CONFIG_SOC_NO_PSRAM
static void soc_psram_flush(void)
{
	spi1_cache_ops(SPI_WRITEBUF_FLUSH, (void *)SPI1_BASE_ADDR, 0x1000);
	spi1_cache_ops(SPI_CACHE_FLUSH_ALL, (void *)SPI1_BASE_ADDR, 0x1000);
	spi1_cache_ops_wait_finshed();
}
#endif

__sleepfunc static void cpu_enter_sleep(void)
{
	uint32_t corepll_backup, sysclk_bak;
	unsigned int i, num_cb;
	uint16_t irq_en_bit[SLEEP_WKSRC_NUM];
	struct sleep_wk_cb cb[SLEEP_WKSRC_NUM];
	uint32_t bk_clksrc0, bk_clksrc1, bk_vout_s1;
	uint32_t devclk[2], memclk[2];
#ifdef CONFIG_SPI0_NOR_DTR_MODE
	uint32_t bk_spi0_delaychain;
#endif

	devclk[0] = sys_read32(CMU_DEVCLKEN0);
	devclk[1] = sys_read32(CMU_DEVCLKEN1);
	memclk[0] = sys_read32(CMU_MEMCLKEN0);
	memclk[1] = sys_read32(CMU_MEMCLKEN1);

	uart_suspend();
	icache_backup_registers();
	dcache_backup_registers();
	nvic_backup_registers();
	dma_backup_registers();
	sdma_backup_registers();
	sys_pwrctrl_assigned_sram(true);

	/* only enable btrom_ram clk in cmu_memclken1 */
	sys_write32(memclk[1] & BTROM_RAM_CLK_EN, CMU_MEMCLKEN1);

#ifndef CONFIG_SOC_NO_PSRAM
	/* flush spi1 cache */
	soc_psram_flush();
#ifdef CONFIG_SLEEP_MEMORY_CHECK_INTEGRITY
	suspend_check();
#endif
	/* disable SPI1 cache for PSRAM */
	sys_clear_bit(SPI1_CACHE_CTL, 0);
#endif

	for(i = 0; i < SLEEP_WKSRC_NUM; i++) // copy nor to sram, nor is not use in sleep func
		irq_en_bit[i] = wk_msg[i].irq_en_bit;

#if	defined(CONFIG_BOARD_EMMCBOOT) || defined(CONFIG_BOARD_NANDBOOT)
	uint32_t spi1_clk, ddr_mode;
	sys_set_bit(CMU_S1CLKCTL,2);	// enable rc64m for psram
	spi1_clk =	sys_read32(CMU_SPI1CLK); // bak spi1 clk
	ddr_mode =	sys_read32(SPI1_DDR_MODE_CTL);
	if(!soc_boot_is_mini())
		sys_write32((ddr_mode & ~(0xf << 16)) | (4<<16), SPI1_DDR_MODE_CTL);  // spi1 clk 64MHZ,must adjust RLC to 4 ( default is 5)
	sys_write32(0x300, CMU_SPI1CLK); // psram now use rc64m
#endif

	/*nor enter deep power down */
	sys_norflash_power_ctrl(1);

	sleep_stage(0);

	/* psram enter low self refresh mode */
	psram_self_refresh_control(true);

	/* psram enter low power mode */
	psram_power_control(true);

	/* make psram cs pin in fixed state when cpu enters deepsleep */
	pwrgat_gpio_psram_backup_registers();

	sysclk_bak =  sys_read32(CMU_SYSCLK);

	/*first switch cpu clk source (hosc)*/
	sys_write32((sysclk_bak&(~0x7)) | 0x1, CMU_SYSCLK);
	soc_udelay(1);
	/*cpu clk select HOSC*/
	if (soc_dvfs_opt())
		sys_write32(0x1, CMU_SYSCLK); /* AHB /2 */
	else
		sys_write32(0x201, CMU_SYSCLK); /* AHB /1 */

	corepll_backup = sys_read32(COREPLL_CTL);
	bk_vout_s1 = sys_read32(VOUT_CTL1_S1M);

	/* disable COREPLL */
	sys_write32(sys_read32(COREPLL_CTL) & ~(1 << 7), COREPLL_CTL);
	sys_write32(0, COREPLL_CTL);

	/*disable avdd, corepll use must enable*/
	sys_clear_bit(AVDDLDO_CTL, 0);


	/* S1 VDD 1.05V
	 * TODO use 0.95V
	 */
#ifdef CONFIG_SPI0_NOR_DTR_MODE
	/* backup S1 delaychain */
	bk_spi0_delaychain = sys_read32(SPI0_DELAYCHAIN);
	/* set delaychain used in ds sensor algo cycle */
	nor_dtr_set_matched_delaychain(950);
	/* backup delaychain in ds sensor algo cycle */
	nor_dtr_backup_delaychain();
#endif
	//sys_write32((sys_read32(VOUT_CTL1_S1M) & ~(0xff)) | 0xaa, VOUT_CTL1_S1M);
	sys_write32((sys_read32(VOUT_CTL1_S1M) & ~(0xfff)) | 0x988, VOUT_CTL1_S1M);//VDD_S1M=0.95V, VD12_S1M=0.95+0.1V
	soc_udelay(30);/*delay for vdd*/

	pwrgat_gpio_backups[3] = sys_read32(GPION_CTL(DEBUG_UART_TX_PIN));
	pwrgat_gpio_backups[4] = sys_read32(GPION_CTL(DEBUG_UART_RX_PIN));

	bk_clksrc0 = sys_read32(CMU_MEMCLKSRC0);
	bk_clksrc1 = sys_read32(CMU_MEMCLKSRC1);

	while(1) {
		/* RAM4 shareRAM select  RC4MHZ in s2) */
		sys_write32((bk_clksrc0 & ~(7 << 25)) | (0 << 25), CMU_MEMCLKSRC0); /*shareRAM select  RC4M */

		pwrgat_gpio_nor_backup_registers();
		pwrgat_set_unused_uart_highz();

		sleep_stage(1);
		__cpu_enter_sleep();
		sleep_stage(2);

		sys_write32((bk_clksrc0 & ~(7 << 25)) | (4 << 25), CMU_MEMCLKSRC0); /* shareRAM select RC64M */
		//sys_write32((bk_clksrc1 & (~0xe)) | (0x4 << 1), CMU_MEMCLKSRC1); /* RC64M and ANX ram */

		sleep_stage(3);

		pwrgat_gpio_nor_restore_registers();
		icache_restore_registers();
		sys_norflash_power_ctrl(0);
		if (sleep_policy_is_pwrgat()) {
			sys_write32(1, SPICACHE_INVALIDATE);
			while((sys_read32(SPICACHE_INVALIDATE) & 0x1) == 1);
		}
	#if	defined(CONFIG_BOARD_EMMCBOOT) || defined(CONFIG_BOARD_NANDBOOT)
		sys_write32(sys_read32(CMU_DEVCLKEN0) | (1<<CLOCK_ID_SPI1CACHE) | (1<<CLOCK_ID_SPI1), CMU_DEVCLKEN0);
		pwrgat_gpio_psram_restore_registers();
		dcache_restore_registers();
		sys_set_bit(CMU_S1CLKCTL,2);	// enable rc64m for psram
		if(!soc_boot_is_mini())
			sys_write32((ddr_mode & ~(0xf << 16)) | (4<<16), SPI1_DDR_MODE_CTL);  // spi1 clk 64MHZ,must adjust RLC to 4 ( default is 5)
		sys_write32(0x300, CMU_SPI1CLK); // psram now use rc64m

		psram_power_control(false); // psram resume for sensor run
	#endif
		arm_floating_point_init();

		/* Resume vector offset before sensor algo for algo fault debug.
		 * Avoid all sections of MTB brush off if running into BROM fault.
		 */
		nvic_restore_registers();

	#ifdef CONFIG_SLEEP_PRINTK_DEBUG
		pwrgat_gpio_uart_restore_registers();
		uart_resume();
	#endif

		if (WK_CB_RUN_SYSTEM == check_wk_run_sram_nor(irq_en_bit, cb, &num_cb)) {
			sleep_stage(4);
			break;
		}
		else {
			sys_restore_wksrc();
			SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;
		}
	#if	defined(CONFIG_BOARD_EMMCBOOT) || defined(CONFIG_BOARD_NANDBOOT)
		psram_power_control(true); // now psram suspend
		/* make psram cs pin in fixed state when cpu enters deepsleep */
		pwrgat_gpio_psram_backup_registers();
	#endif
	}

	sleep_stage(5);

	/* restore memory clock sources */
	sys_write32(bk_clksrc0, CMU_MEMCLKSRC0);
	sys_write32(bk_clksrc1, CMU_MEMCLKSRC1);

	/* hosc+rc4m+64M */
	sys_write32(0x7, CMU_S1CLKCTL);

	/* cmu/memclk/ram_ds go to normal */
	sys_write32(devclk[0], CMU_DEVCLKEN0);
	sys_write32(devclk[1], CMU_DEVCLKEN1);
	sys_write32(memclk[0], CMU_MEMCLKEN0);
	sys_write32(memclk[1], CMU_MEMCLKEN1);

	pwrgat_gpio_psram_restore_registers();
	pwrgat_gpio_uart_restore_registers();
	dcache_restore_registers();
	uart_resume();
	dma_restore_registers();
	sdma_restore_registers();

	__DSB();
	__ISB();

#ifndef CONFIG_SOC_NO_PSRAM
	/* enable spi1 cache */
	sys_set_bit(SPI1_CACHE_CTL, 0);
#endif

#ifdef CONFIG_SPI0_NOR_DTR_MODE
	/* restore S1 delaychain */
	sys_write32(bk_spi0_delaychain, SPI0_DELAYCHAIN);
#endif
	/* restore VDD */
	sys_write32(bk_vout_s1, VOUT_CTL1_S1M);
	soc_udelay(30);/*delay for vdd*/

	/*enable avdd, for pll*/
	sys_write32(((sys_read32(AVDDLDO_CTL)
				& (~(1<<AVDDLDO_CTL_AVDD_PD_EN)))
				| (1<<AVDDLDO_CTL_AVDD_EN)), AVDDLDO_CTL);
	soc_udelay(1);

	/* resume psram */
	psram_power_control(false);

	for(i = 0; i < 300; i++){
		if(sys_read32(HOSC_CTL) & HOSC_CTL_READY_MASK)
			break;
		soc_udelay(5);
	}
	if (!soc_dvfs_opt())
		soc_udelay(200);

	sys_write32(corepll_backup, COREPLL_CTL);
	/*spi0 clk switch to hosc*/
	sys_write32(sys_read32(CMU_SPI0CLK) & ~(0x3 << 8) & ~(0xf << 0), CMU_SPI0CLK);
	for(i = 0; i < 300; i++){
		if(sys_read32(COREPLL_CTL) & (1<<8))
			break;
		soc_udelay(5);
	}

	check_cnt = i;

	/*first switch clk ahb div*/
	sys_write32((sys_read32(CMU_SYSCLK)&0x7) | (sysclk_bak & (~0x7)) , CMU_SYSCLK);
	soc_udelay(1);
	sys_write32(sysclk_bak, CMU_SYSCLK);

	psram_self_refresh_control(false);

#if	defined(CONFIG_BOARD_EMMCBOOT) || defined(CONFIG_BOARD_NANDBOOT)
	sys_write32(spi1_clk, CMU_SPI1CLK); // restore spi1 clk for spsram
	sys_write32(ddr_mode, SPI1_DDR_MODE_CTL);
#endif

#ifndef CONFIG_SOC_NO_PSRAM
	/*spi1 rx fifo reset*/
	sys_clear_bit(SPI1_CTL, 4);
	sys_clear_bit(SPI1_CTL, 5);
	soc_udelay(1);
	sys_set_bit(SPI1_CTL, 4);
	sys_set_bit(SPI1_CTL, 5);

	/* invalid spi1 cache */
	spi1_cache_ops(SPI_CACHE_INVALID_ALL, (void *)SPI1_BASE_ADDR, 0x1000);
#ifdef CONFIG_SLEEP_MEMORY_CHECK_INTEGRITY
	resume_check();
#endif
#endif

	sys_pwrctrl_assigned_sram(false);

	current_sleep->g_num_cb = num_cb;
	memcpy(current_sleep->g_syste_cb, cb,
			num_cb * sizeof(struct sleep_wk_cb));

}


#ifdef CONFIG_SLEEP_PRINTK_DEBUG
#define soc_enter_sleep_switch_uart(x)		NULL
#else
#ifdef CONFIG_SLEEP_UART_SILENT
#define soc_enter_sleep_switch_uart(x)		NULL
#else
/* bt pwrgat, tx isolated to 0, should not set PULLUP to save pwr */
static const struct acts_pin_config bt_pin_cfg_uart[] = {
	{28, 23 | GPIO_CTL_PADDRV_LEVEL(1)},
	{29, 23 | GPIO_CTL_PADDRV_LEVEL(1) | GPIO_CTL_PULLUP},
};

static const struct acts_pin_config leopard_pin_cfg_uart[] = {
	{28, 5 | GPIO_CTL_PADDRV_LEVEL(1) | GPIO_CTL_PULLUP},
	{29, 5 | GPIO_CTL_PADDRV_LEVEL(1) | GPIO_CTL_PULLUP},
};

static void soc_enter_sleep_switch_uart(bool bt_uart)
{
	//if(current_sleep->sleep_mode)
	//	return;

	if (bt_uart) {
		acts_pinmux_setup_pins(bt_pin_cfg_uart, ARRAY_SIZE(bt_pin_cfg_uart));
	} else {
		acts_pinmux_setup_pins(leopard_pin_cfg_uart, ARRAY_SIZE(leopard_pin_cfg_uart));
	}
}
#endif /* CONFIG_SLEEP_UART_SILENT */
#endif /* CONFIG_SLEEP_TIMER_DEBUG */

void dump_reg(const char *promt)
{
#ifdef CONFIG_SLEEP_DUMP_INFO
	printk("%s LOSC_CTL=0x%x,sleep_mode=%d\n", promt, sys_read32(LOSC_CTL), current_sleep->sleep_mode);

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

unsigned int soc_sleep_cycle(void)
{
	return current_sleep->g_sleep_cycle;
}

 __sleepfunc int64_t soc_sys_uptime_get(void)
{
	if(g_sleep_st){
		unsigned int ms;
		while(g_sleep_t2cnt == sys_read32(T2_CNT)); // wait for clock sync
		ms =  rc32k_cyc_to_ms(sys_read32(T2_CNT)-g_sleep_st);
		return (g_sleep_update_time+g_sleep_ms) + ms;
	}else{
		return (k_uptime_get()+g_sleep_ms);
	}
}
static void soc_timer_sleep_prepare(void)
{
	g_sleep_update_time = k_uptime_get();
	acts_clock_peripheral_disable(CLOCK_ID_TIMER2); // for switch clk source
	sys_write32(0x4, CMU_TIMER2CLK); /* select rc32k before enter S3 */
	g_sleep_st = sys_read32(T2_CNT);
	acts_clock_peripheral_enable(CLOCK_ID_TIMER2);
}

static void soc_timer_exit_sleep(void)
{
	unsigned int end;
	acts_clock_peripheral_disable(CLOCK_ID_TIMER2); // for switch clk source
	end =  sys_read32(T2_CNT);
	sys_write32(0x0, CMU_TIMER2CLK); /* select hosc for k_cycle_get */
	acts_clock_peripheral_enable(CLOCK_ID_TIMER2);
	dump_stage_timer(g_sleep_st, end);
	end = end - g_sleep_st;
	current_sleep->g_sleep_cycle += soc_rc32K_mutiple_hosc()*end;
	g_sleep_ms += rc32k_cyc_to_ms(end);
	g_sleep_st = 0;
}

static uint8_t g_lcd_aod_mode;

void soc_set_aod_mode(int is_aod)
{
	g_lcd_aod_mode = is_aod ? 1 : 0;
	if (is_aod) {
		sys_s3_wksrc_set(SLEEP_WK_SRC_RTC);
	} else {
		sys_s3_wksrc_clr(SLEEP_WK_SRC_RTC);
	}
}

int soc_get_aod_mode(void)
{
	return g_lcd_aod_mode;
}

void soc_set_sleep_mode(uint8_t mode)
{
	if(mode > SLEEP_MODE_FORCE_SLEEP)
		mode = SLEEP_MODE_FORCE_SLEEP;
	current_sleep->sleep_mode = mode;
	printk("sleep mode=%d\n", current_sleep->sleep_mode);
	
	if (mode != SLEEP_MODE_NORMAL) {
#ifdef CONFIG_SYS_WAKELOCK
		sys_wakelocks_enable(0);
#endif
	}
}

int soc_in_sleep_mode(void)
{
	return (g_sleep_st > 0);
}

/*AIRCR SCR CCR SHPR SHCSR*/

static unsigned int  scb_aircr_bak, scb_scr_bak, scb_ccr_bak, scb_shcsr_bak;
static unsigned char scb_shpr_bak[12];

static void  soc_arm_reg_backup(void)
{

	memcpy(scb_shpr_bak, (void*)SCB->SHPR, 12);
	scb_aircr_bak = SCB->AIRCR;
	scb_scr_bak = SCB->SCR;
	scb_ccr_bak = SCB->CCR;
	scb_shcsr_bak = SCB->SHCSR;
}

static void  soc_arm_reg_restore(void)
{
#ifdef CONFIG_ACTIONS_ARM_MPU
	arm_mpu_protect_init();
#endif
	memcpy((void*) SCB->SHPR, scb_shpr_bak, 12);
	SCB->AIRCR = scb_aircr_bak;
	SCB->SCR = scb_scr_bak;
	SCB->CCR = scb_ccr_bak;
	SCB->SHCSR = scb_shcsr_bak;
}

void soc_enter_deep_sleep(void)
{
	dump_reg("before");

	soc_pmu_check_onoff_reset_func();
#ifdef CONFIG_SLEEP_DISABLE_BT
	soc_cmu_sleep_prepare(0);
#else
	soc_cmu_sleep_prepare(sys_get_bt_pwr_by_sleep_mode());
#endif
	soc_timer_sleep_prepare();
	if (sleep_policy_is_pwrgat()) {
		soc_arm_reg_backup();
	}
	soc_enter_sleep_switch_uart(true);
	cpu_enter_sleep();/* wfi,enter to sleep */
	soc_enter_sleep_switch_uart(false);
	if (sleep_policy_is_pwrgat()) {
		soc_arm_reg_restore();
	}
	soc_timer_exit_sleep();
	soc_cmu_sleep_exit();
	dump_reg("BT after");

#ifdef CONFIG_SLEEP_STAGE_DEBUG
	soc_pstore_set(SOC_PSTORE_TAG_SLEEP_DBG_STAGE, 0);
#endif
}

void soc_enter_light_sleep(void)
{
	dump_reg("before");

	soc_cmu_sleep_prepare(0);
	cpu_enter_sleep();/* wfi,enter to sleep */
	soc_cmu_sleep_exit();

	dump_reg("after");
}

static int soc_sleep_init(const struct device *dev)
{
	int i;

	ARG_UNUSED(dev);
#ifdef CONFIG_SLEEP_STAGE_DEBUG
	uint32_t val;
	soc_pstore_get(SOC_PSTORE_TAG_SLEEP_DBG_STAGE, &val);
	printk("SLEEP_DBG_STAGE=0x%x\n", val);
#endif
	g_lcd_aod_mode = 0;
	current_sleep->g_sleep_cycle = 0;
	current_sleep->sleep_mode = 0;
	g_sleep_ms = 0;
	g_sleep_st = 0;
	g_sleep_wksrc_en = 0;
	g_sleep_wksrc_en_bak = 0;
	g_cyc2ms_mul = (1000<<16) / soc_rc32K_freq();

	for(i = 0; i < SLEEP_WKSRC_NUM; i++)
		g_wk_fun[i] = NULL;

#ifdef CONFIG_JTAG_DEBUG
	extern void jtag_set(void);
	jtag_set();
#endif

#ifdef CONFIG_SLEEP_MEMORY_CHECK_INTEGRITY
	current_sleep->check_start_addr = PSRAM_START_ADDR;
	current_sleep->check_len = MEMORY_CHECK_INTEGRITY_SIZE;
#endif

	return 0;
}

SYS_INIT(soc_sleep_init, PRE_KERNEL_1, 80);


