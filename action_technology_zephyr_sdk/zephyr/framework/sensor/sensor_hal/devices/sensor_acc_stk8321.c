/*******************************************************************************
 * @file    sensor_acc_stk8321.c
 * @author  MEMS Application Team
 * @version V1.0
 * @date    2022-08-03
 * @brief   sensor devices module
*******************************************************************************/

/******************************************************************************/
//includes
/******************************************************************************/
#include <sensor_acc_stk8321.h>

/******************************************************************************/
//constants
/******************************************************************************/

/******************************************************************************/
//sensor task
/******************************************************************************/
const i2c_task_t stk8321_task = {
	.irq_type = I2C_TASK_IRQ_CMPLT | I2C_TASK_IRQ_HALF_CMPLT,
#if defined(CONFIG_SOC_SERIES_LEOPARD)
	.ctl = {
		.sdataddr_sel = 0,
		.rdlen_wsdat = 6,
		.swen = 0,
	},
	.slavedev = {
		.rwsel = 1,
		.sdevaddr = STK8321_ADR,
		.sdataddr = 0x3F,
	},
#else
	.ctl = {
		.rwsel = 1,
		.sdevaddr = STK8321_ADR,
		.sdataddr = 0x3F,
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
const sensor_cfg_t stk8321_init_cfg[] = {
	{ 0x14, 1,  {0xb6} }, // soft reset
	{ 0x11, 1,  { 0x80 }  }, // suspend mode enable
	{ REG_DELAY, 50, { 0 } },    // delay 10ms
	{ 0x0F, 1,  {0x05}  }, // range = +/-4g
	{ 0x5E, 1,  {0xC0}  }, // eng mode for power saving
	{ 0x34, 1,  {0x04}  }, // enable watch dogs
	//{ 0x10, 1,  {0x0B}  }, //set bandwidth 125hz
	{ 0x10, 1,  {0x0F}  }, //set bandwidth 1000hz
	//{ 0x11, 1,  {0x74}  }, // low-power mode Duration (10ms)
	{ 0x11, 1,  {0x76}  }, // low-power mode Duration (25ms)
	//{ 0x3E, 1,  {0xC4}  }, // stream mode and store interval 4 (ODR=100 and FIFO ODR=100/4=25Hz)
	{ 0x3E, 1,  {0xC0}  },
	{ REG_NULL }, // end
};

const sensor_cfg_t stk8321_on_cfg[] = {
	{ 0x1A, 1,  {0x02}  }, //FIFO watermark interrupt to INT1 
	{ 0x17, 1,  {0x40}  }, //Enable FIFO watermark interrupt.
	{ 0x3D, 1,  {1}  }, //FIFO watermark = 1 frame 
	{ 0x11, 1,  {0x76}  }, // low-power mode Duration (25ms)
	{ REG_NULL }, // end
};

const sensor_cfg_t stk8321_off_cfg[] = {
	{ 0x11, 1,  { 0x80 }  }, // suspend mode enable
	{ REG_NULL }, // end
};

/******************************************************************************/
// sensor function
/******************************************************************************/
int stk8321_cvt(float *val, uint8_t *buf, uint16_t len, int16_t *raw)
{
	int idx;
	short *acc = (short *)buf;

	/* Gain is in 1.95mg/LSB */
	/* Convert to m/s^2 */
	for (idx = 0; idx < (len/2); idx++) {
		if (raw != NULL) {
			raw[idx] = (acc[idx] >> 4);
		}
		if (val != NULL) {
			val[idx] = (acc[idx] >> 4) * (1950) * 9.80665f / 1000000LL;
		}
	}

	return 3;
}
