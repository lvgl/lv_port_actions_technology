/*******************************************************************************
 * @file    sensor_hr_vc9202.c
 * @author  MEMS Application Team
 * @version V1.0
 * @date    2022-08-03
 * @brief   sensor devices module
*******************************************************************************/

/******************************************************************************/
//includes
/******************************************************************************/
#include <sensor_hr_vc9202.h>

/******************************************************************************/
//constants
/******************************************************************************/

/******************************************************************************/
//sensor task
/******************************************************************************/
const i2c_task_t vc9202_task = {
	.irq_type = I2C_TASK_IRQ_CMPLT | I2C_TASK_IRQ_HALF_CMPLT,
#if defined(CONFIG_SOC_SERIES_LEOPARD)
	.ctl = {
		.sdataddr_sel = 0,
		.rdlen_wsdat = 16,
		.swen = 0,
	},
	.slavedev = {
		.rwsel = 1,
		.sdevaddr = VC9202_ADR,
		.sdataddr = 0xaa,
	},
#else
	.ctl = {
		.rwsel = 1,
		.sdevaddr = VC9202_ADR,
		.sdataddr = 0xaa,
		.rdlen_wsdat = 16,
	},
#endif
	.dma = {
		.reload = 1,
		.len = (2 * 16),
	},
	.trig = {
		.en = 0,
		.chan = 0,
		.task = 0, 
		.trig = TIMER3, // 30K counter for vcare hr algo
	},
};
