/*******************************************************************************
 * @file    sensor_mag_mmc5603nj.c
 * @author  MEMS Application Team
 * @version V1.0
 * @date    2022-08-03
 * @brief   sensor devices module
*******************************************************************************/

/******************************************************************************/
//includes
/******************************************************************************/
#include <sensor_mag_mmc5603nj.h>

/******************************************************************************/
//constants
/******************************************************************************/

/******************************************************************************/
//sensor task
/******************************************************************************/
const i2c_task_t mmc5603nj_task = {
	.irq_type = I2C_TASK_IRQ_CMPLT | I2C_TASK_IRQ_HALF_CMPLT,
#if defined(CONFIG_SOC_SERIES_LEOPARD)
	.ctl = {
		.sdataddr_sel = 0,
		.rdlen_wsdat = 9,
		.swen = 0,
	},
	.slavedev = {
		.rwsel = 1,
		.sdevaddr = MMC5603NJ_ADR,
		.sdataddr = 0x0,
	},
#else
	.ctl = {
		.rwsel = 1,
		.sdevaddr = MMC5603NJ_ADR,
		.sdataddr = 0xaa,
		.rdlen_wsdat = 16,
	},
#endif
	.dma = {
		.reload = 1,
		.len = (2 * 9 * 10),
	},
	.trig = {
		.en = 1,
		.chan = PPI_CH1,
		.task = I2CMT1_TASK0,
		.trig = TIMER3,
		.peri = 20,
	},
};

/******************************************************************************/
//sensor config
/******************************************************************************/
const sensor_cfg_t mmc5603nj_init_cfg[] = {
	{ 0x1c,    1,  { (1 << 7) } }, // soft reset
	{ REG_DELAY, 20, { 0 } }, // delay 20ms
	{ 0x1a,    1,  { 50 } }, // set ODR
	{ REG_NULL }, // end
};

const sensor_cfg_t mmc5603nj_on_cfg[] = {
	{ 0x1b,    1,  { (1 << 5) | (1 << 7) } }, // set auto SET/RESET & start calculation of the measurement period
	{ 0x1d,    1,  { (1 << 4) } }, // enter continuous mode
	{ REG_NULL }, // end
};

const sensor_cfg_t mmc5603nj_off_cfg[] = {
	{ 0x1b,    1,  { 0 } }, // disable cmd en
	{ 0x1d,    1,  { 0 } }, // disable continuous mode
	{ REG_NULL }, // end
};

/******************************************************************************/
// sensor function
/******************************************************************************/
int mmc5603nj_cvt(float *val, uint8_t *buf, uint16_t len, int16_t *raw)
{
	int idx;
	int mag;

	/* Gain is in 0.0625mG/LSB = 0.00625uT/LSB */
	/* Convert to uT */
	for (idx = 0; idx < 3; idx++) {
		mag = (((buf[2*idx] << 16) | (buf[2*idx+1] << 8) | (buf[6+idx])) >> 4) - 524288;
		if (raw != NULL) {
			raw[idx] = mag;
		}
		if (val != NULL) {
			val[idx] = mag * 0.00625f;
		}
	}

	return 3;
}
