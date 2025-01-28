/*******************************************************************************
 * @file    sensor_acc_qma6100.c
 * @author  MEMS Application Team
 * @version V1.0
 * @date    2022-08-03
 * @brief   sensor devices module
*******************************************************************************/

/******************************************************************************/
//includes
/******************************************************************************/
#include <sensor_acc_qma6100.h>

/******************************************************************************/
//constants
/******************************************************************************/

/******************************************************************************/
//sensor task
/******************************************************************************/
const i2c_task_t qma6100_task = {
	.irq_type = I2C_TASK_IRQ_CMPLT | I2C_TASK_IRQ_HALF_CMPLT,
#if defined(CONFIG_SOC_SERIES_LEOPARD)
	.ctl = {
		.sdataddr_sel = 0,
		.rdlen_wsdat = 6,
		.swen = 0,
	},
	.slavedev = {
		.rwsel = 1,
		.sdevaddr = QMA6100_ADR,
		.sdataddr = 0x3F,
	},
#else
	.ctl = {
		.rwsel = 1,
		.sdevaddr = QMA6100_ADR,
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
const sensor_cfg_t qma6100_init_cfg[] = {
	{ 0x36, 1,  {0xb6} }, // soft reset
	{ REG_DELAY, 5, { 0 } },    // delay 5ms
	{ 0x36, 1,  {0x00} }, // soft reset



	{ 0x11, 1,  {0x80}  }, // set device into active mode
	{ 0x11, 1,  {0x84}  }, // Freq of MCLK = 50K


	{ 0x4a, 1,  {0x20}  },
	{ 0x56, 1,  {0x01}  },
	{ 0x5f, 1,  {0x80}  },
	{ REG_DELAY, 2, { 0 } },    // delay 10ms
	{ 0x5f, 1,  {0x00}  },
	{ REG_DELAY, 10, { 0 } },    // delay 10ms


	{ 0x0f, 1,  {0x02}  }, // set_range = +-4G

	{ 0x10, 1,  {0x05}  }, // NO LPF, ODR = MCLK/512
	//{ 0x11, 1,  {0x84}  }, // set device into active mode,Freq of MCLK = 51.2K

	{ 0x17, 1,  {0x40}  }, //Enable FIFO watermark interrupt.

	{ 0x1a, 1,  {0x40}  }, //map FIFO watermark interrupt to INT1 pin

	{ 0x21, 1,  {0x80}  }, //fifo use latch int

	{ 0x31, 1,  {0x1}  }, //FIFO watermark = 1 frame 

	{ 0x3e, 1,  {0x87}  }, //select stream mode

	{ REG_NULL }, // end
};

const sensor_cfg_t qma6100_on_cfg[] = {
	{ 0x11, 1,  { 0x84 }  }, // suspend mode enable
	{ 0x1A, 1,  {0x40}  }, //FIFO watermark interrupt to INT1 
	{ 0x17, 1,  {0x40}  }, //Enable FIFO watermark interrupt.
	{ 0x31, 1,  {0x1}  }, //FIFO watermark = 1 frame 
	{ REG_NULL }, // end
};

const sensor_cfg_t qma6100_off_cfg[] = {
	{ 0x11, 1,  { 0x4 }  }, // suspend mode enable
	{ REG_NULL }, // end
};

/******************************************************************************/
// sensor function
/******************************************************************************/
int qma6100_cvt(float *val, uint8_t *buf, uint16_t len, int16_t *raw)
{
	int idx;
	short acc[3];

	acc[0] = ((buf[1]<<8))|(buf[0]);
	acc[1] = ((buf[3]<<8))|(buf[2]);
	acc[2] = ((buf[5]<<8))|(buf[4]);
	acc[0] = acc[0]>>2;
	acc[1] = acc[1]>>2;
	acc[2] = acc[2]>>2;

	/* Gain is in 1.95mg/LSB */
	/* Convert to m/s^2 */
	for (idx = 0; idx < 3; idx++) {
		if (raw != NULL) {
			raw[idx] = acc[idx];
		}
		if (val != NULL) {
			val[idx] = (acc[idx] * 9.80665f) / 2048;
		}
	}

	return 3;
}
