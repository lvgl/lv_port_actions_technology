/*******************************************************************************
 * @file    sensor_acc_sc7a20.c
 * @author  MEMS Application Team
 * @version V1.0
 * @date    2022-08-03
 * @brief   sensor devices module
*******************************************************************************/

/******************************************************************************/
//includes
/******************************************************************************/
#include <sensor_acc_sc7a20.h>

/******************************************************************************/
//constants
/******************************************************************************/

/******************************************************************************/
//sensor task
/******************************************************************************/
const i2c_task_t sc7a20_task = {
	.irq_type = I2C_TASK_IRQ_CMPLT | I2C_TASK_IRQ_HALF_CMPLT,
#if defined(CONFIG_SOC_SERIES_LEOPARD)
	.ctl = {
		.sdataddr_sel = 0,
		.rdlen_wsdat = 6,
		.swen = 0,
	},
	.slavedev = {
		.rwsel = 1,
		.sdevaddr = SC7A20_ADR,
		.sdataddr = 0x3F,
	},
#else
	.ctl = {
		.rwsel = 1,
		.sdevaddr = SC7A20_ADR,
		.sdataddr = 0x3F,
		.rdlen_wsdat = 6,
	},
#endif
	.dma = {
		.reload = 1,
		.len = (2 * 6 * 10),
	},
	.trig = {
		.en = 0,
		.chan = 0,
		.task = 0,
		.trig = TIMER3,
		.peri = 500,
	},
};

