/*******************************************************************************
 * @file    sensor_sleep.c
 * @author  MEMS Application Team
 * @version V1.0
 * @date    2020-08-12
 * @brief   sensor testing
*******************************************************************************/

/******************************************************************************/
//includes
/******************************************************************************/
#include <stdio.h>
#include <string.h>
#include <zephyr.h>
#include <soc.h>
#include <drivers/i2cmt.h>
#include <sensor_hal.h>
#include <sensor_bus.h>
#include <sensor_algo.h>
#include <sensor_devices.h>
#include <hr_algo.h>
#include <linker/linker-defs.h>
#include "sensor_port.h"

/******************************************************************************/
//declarations
/******************************************************************************/
extern void algo_handler(int id, sensor_dat_t *dat);
static enum WK_CB_RC sleep_wk_callback(enum S_WK_SRC_TYPE wk_src);
static enum WK_RUN_TYPE sleep_wk_prepare(enum S_WK_SRC_TYPE wk_src);

/******************************************************************************/
//variables
/******************************************************************************/
static __act_s2_sleep_data int wakeup_system = 0;

static __act_s2_sleep_data struct sleep_wk_fun_data sleep_fn = 
{
	.wk_cb = sleep_wk_callback,
	.wk_prep = sleep_wk_prepare,
};

__sleepfunc static enum WK_CB_RC sleep_wk_callback(enum S_WK_SRC_TYPE wk_src)
{
	int len;
	uint8_t *buf;
	uint32_t pd_reg, int_stat;
	sensor_dat_t dat;

	printk("callback wk_src=%d\r\n", wk_src);

	switch ((int)wk_src) {
		case SLEEP_WK_SRC_IIC1MT:
			// acc sensor
			buf = i2c_task_get_data(1, 0, ACC_TRIG, &len);
			if (buf != NULL) {
				printk("ACC buf=0x%p, len=%d, time=%d\r\n", buf, len, (uint32_t)soc_sys_uptime_get());

				/* init data */
				sensor_hal_init_data(ID_ACC, &dat, buf, len);

				/* process data */
				algo_handler(ID_ACC, &dat);
			}
			break;
	
		case SLEEP_WK_SRC_GPIO:
			// check pending
			pd_reg = GPION_IRQ_PD(HR_ISR);
			int_stat = sys_read32(pd_reg);
			if(int_stat & GPIO_BIT(HR_ISR)){
				// clear pending
				sys_write32(GPIO_BIT(HR_ISR), pd_reg);
				
				// hr sensor
				printk("hr proc start\r\n");
				hr_algo_process();
				printk("hr proc end\r\n");
			}else{
				// check pending
				pd_reg = GPION_IRQ_PD(CONFIG_TPKEY_GPIO_ISR_NUM);
				int_stat = sys_read32(pd_reg);
				if(int_stat & GPIO_BIT(CONFIG_TPKEY_GPIO_ISR_NUM)){
					// clear pending
					sys_write32(GPIO_BIT(CONFIG_TPKEY_GPIO_ISR_NUM), pd_reg);
					wakeup_system = 1;
				} else {
					return WK_CB_CARELESS;
				}
			}
			break;

		case SLEEP_WK_SRC_T3:
			sensor_hal_clear_tm_pending(TIMER3);
			/* process data */
			printk("time %d\r\n", (uint32_t)soc_sys_uptime_get());
#if CONFIG_SENSOR_ALGO_MOTION_SILAN
			algo_handler(ID_ACC, NULL);
#endif
#if (CONFIG_SENSOR_ALGO_HR_HX3605 || CONFIG_SENSOR_ALGO_HR_HX3690)
			hr_algo_process();
#endif
			break;
	}

	if (wakeup_system) {
		wakeup_system = 0;
		return WK_CB_RUN_SYSTEM;
	} else {
		return WK_CB_SLEEP_AGAIN;
	}
}

#ifdef __UVISION_VERSION
__attribute__((no_stack_protector))
#else
__attribute__((optimize("no-stack-protector")))
#endif
__sleepfunc static enum WK_RUN_TYPE sleep_wk_prepare(enum S_WK_SRC_TYPE wk_src)
{
//	printk("prepare wk_src=%d\r\n", wk_src);
	return WK_RUN_IN_NOR; // WK_RUN_IN_SYTEM for debug
}

/******************************************************************************/
//functions
/******************************************************************************/
int sensor_sleep_init(void)
{
	// acc wakeup
	sys_s3_wksrc_set(SLEEP_WK_SRC_IIC1MT);
	sys_s3_wksrc_set(SLEEP_WK_SRC_T3);

	sleep_register_wk_callback(SLEEP_WK_SRC_IIC1MT, &sleep_fn);
	sleep_register_wk_callback(SLEEP_WK_SRC_T3, &sleep_fn);
	
	// hr wakeup
	sys_s3_wksrc_set(SLEEP_WK_SRC_GPIO);
	sleep_register_wk_callback(SLEEP_WK_SRC_GPIO, &sleep_fn);

	return 0;
}

int sensor_sleep_suspend(void)
{
	return sensor_hal_suspend();
}

int sensor_sleep_resume(void)
{
	return sensor_hal_resume();
}

void sensor_sleep_wakeup(int wakeup)
{
	wakeup_system = wakeup;
}

