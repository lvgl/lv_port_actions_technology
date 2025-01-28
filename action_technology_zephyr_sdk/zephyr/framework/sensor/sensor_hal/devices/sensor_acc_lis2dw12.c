/*******************************************************************************
 * @file    sensor_acc_lis2dw12.c
 * @author  MEMS Application Team
 * @version V1.0
 * @date    2022-08-03
 * @brief   sensor devices module
*******************************************************************************/

/******************************************************************************/
//includes
/******************************************************************************/
#include <sensor_acc_lis2dw12.h>

/******************************************************************************/
//constants
/******************************************************************************/

/******************************************************************************/
//sensor task
/******************************************************************************/
const i2c_task_t lisdw12_task = {
	.irq_type = I2C_TASK_IRQ_CMPLT | I2C_TASK_IRQ_HALF_CMPLT,
#if defined(CONFIG_SOC_SERIES_LEOPARD)
	.ctl = {
		.sdataddr_sel = 0,
		.rdlen_wsdat = 6,
		.swen = 0,
	},
	.slavedev = {
		.rwsel = 1,
		.sdevaddr = LIS2DW12_ADR,
		.sdataddr = 0x28,
	},
#else
	.ctl = {
		.rwsel = 1,
		.sdevaddr = LIS2DW12_ADR,
		.sdataddr = 0x28,
		.rdlen_wsdat = 6,
	},
#endif
	.dma = {
		.reload = 1,
		.len = (2 * 6 * 10),
	},
	.trig = {
		.en = 1,
		.chan = PPI_CH3,
		.task = I2CMT1_TASK0,
		.trig = IO0_IRQ + ACC_TRIG,
	},
};

/******************************************************************************/
//sensor config
/******************************************************************************/
const sensor_cfg_t lisdw12_init_cfg[] = {
	{ 0x21, 1,  { (1 << 6) }  }, // soft reset
	{ REG_DELAY, 10, { 0 } },    // delay 10ms
	{ 0x25, 1,  { (3 << 4) }  }, // set fs: +-16G
	{ REG_NULL }, // end
};

const sensor_cfg_t lisdw12_on_cfg[] = {
	{ 0x23, 1,  { (1 << 0) }  }, // enable int
	{ 0x20, 1,  { (4 << 4) | (0 << 2) | 1 }  }, // set mode and odr
	{ REG_NULL }, // end
};

const sensor_cfg_t lisdw12_off_cfg[] = {
	{ 0x23, 1,  { 0 }  }, // disable int
	{ 0x20, 1,  { 0 }  }, // power down
	{ REG_NULL }, // end
};

/******************************************************************************/
// sensor function
/******************************************************************************/
int lisdw12_cvt(float *val, uint8_t *buf, uint16_t len, int16_t *raw)
{
	int idx;
	short *acc = (short *)buf;

	/* Mapping sensor(x/y/z) to display(-x/-y/z) */
	acc[0] = -acc[0];
	acc[1] = -acc[1];

	/* Gain is in 244ug/LSB */
	/* Convert to m/s^2 */
	for (idx = 0; idx < 3; idx++) {
		if (raw != NULL) {
			raw[idx] = (acc[idx] >> 2);
		}
		if (val != NULL) {
			val[idx] = (acc[idx] >> 2) * (244 << 3) * 9.80665f / 1000000LL;
		}
	}

	return 3;
}
