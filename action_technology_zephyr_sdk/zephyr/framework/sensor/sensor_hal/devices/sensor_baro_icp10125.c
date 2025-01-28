/*******************************************************************************
 * @file    sensor_baro_icp10125.c
 * @author  MEMS Application Team
 * @version V1.0
 * @date    2022-08-03
 * @brief   sensor devices module
*******************************************************************************/

/******************************************************************************/
//includes
/******************************************************************************/
#include <sensor_hal.h>
#include <sensor_baro_icp10125.h>

/******************************************************************************/
//constants
/******************************************************************************/

/******************************************************************************/
//sensor task
/******************************************************************************/
const i2c_task_t icp10125_task = {
	.irq_type = I2C_TASK_IRQ_CMPLT | I2C_TASK_IRQ_HALF_CMPLT,
#if defined(CONFIG_SOC_SERIES_LEOPARD)
	.ctl = {
		.sdataddr_sel = 1,
		.rdlen_wsdat = 9,
	},
	.slavedev = {
		.rwsel = 1,
		.sdevaddr = ICP10125_ADR,
		.sdataddr = 0,
	},
#else
	.ctl = {
		.rwsel = 1,
		.sdevaddr = ICP10125_ADR,
		.sdataddr = 0,
		.rdlen_wsdat = 9,
	},
#endif
	.dma = {
		.reload = 1,
		.len = (2 * 9),
	},
	.trig = {
		.en = 0,
		.chan = 0,
		.task = 0,
		.trig = TIMER4,
		.peri = 100,
	},
};

/******************************************************************************/
//sensor config
/******************************************************************************/
const sensor_cfg_t icp10125_init_cfg[] = {
	{ 0x805d,    0,  { 0 } }, // soft reset
	{ REG_DELAY, 20, { 0 } }, // delay 20ms
	{ REG_NULL }, // end
};

/******************************************************************************/
// sensor function
/******************************************************************************/
/* data structure to hold pressure sensor related parameters */
typedef struct inv_invpres {
	float sensor_constants[4]; // OTP values
	float p_Pa_calib[3];
	float LUT_lower;
	float LUT_upper;
	float quadr_factor;
	float offst_factor;
} inv_invpres_t;

static inv_invpres_t invpres;

static int read_otp_from_i2c(short *out)
{
	uint8_t buf[4];
	int status;
	int i;
	
	// OTP Read mode
	buf[0] = 0x00;
	buf[1] = 0x66;
	buf[2] = 0x9C;
	status = sensor_hal_write(ID_BARO, 0xC595, buf, 3);
	if (status)
		return status;
	
	// Read OTP values
	for (i = 0; i < 4; i++) {
		status = sensor_hal_read(ID_BARO, 0xC7F7, buf, 3);
		if (status)
			return status;
		out[i] = (buf[0] << 8) | buf[1];
	}
	
	return 0;
}

// p_Pa -- List of 3 values corresponding to applied pressure in Pa
// p_LUT -- List of 3 values corresponding to the measured p_LUT values at the applied pressures.
static void calculate_conversion_constants(float *p_Pa, float *p_LUT, float *out)
{
	out[2] = (p_LUT[0] * p_LUT[1] * (p_Pa[0] - p_Pa[1]) + p_LUT[1] * p_LUT[2] * (p_Pa[1] - p_Pa[2])
				+	p_LUT[2] * p_LUT[0] * (p_Pa[2] - p_Pa[0])) / (p_LUT[2] * (p_Pa[0] - p_Pa[1])
				+	p_LUT[0] * (p_Pa[1] - p_Pa[2]) + p_LUT[1] * (p_Pa[2] - p_Pa[0]));
	out[0] = (p_Pa[0] * p_LUT[0] - p_Pa[1] * p_LUT[1] - (p_Pa[1] - p_Pa[0]) * out[2]) / (p_LUT[0] - p_LUT[1]);
	out[1] = (p_Pa[0] - out[0]) * (p_LUT[0] + out[2]);
}

int icp10125_init(void)
{
	inv_invpres_t *s = &invpres;
	short otp[4];
	int i;
	
	// read otp
	read_otp_from_i2c(otp);
	
	// init param
	for(i = 0; i < 4; i++) {
		s->sensor_constants[i] = (float)otp[i];
	}
	s->p_Pa_calib[0] = 45000.0;
	s->p_Pa_calib[1] = 80000.0;
	s->p_Pa_calib[2] = 105000.0;
	s->LUT_lower = 3.5 * (1<<20);
	s->LUT_upper = 11.5 * (1<<20);
	s->quadr_factor = 1 / 16777216.0;
	s->offst_factor = 2048.0;
	
	return 0;
}

int icp10125_cvt(float *val, uint8_t *buf, uint16_t len, int16_t *raw)
{
	inv_invpres_t *s = &invpres;
	int press;
	int temp;
	float t;
	float in[3];
	float out[3];
	
	if (val != NULL) {
		/* get raw data */
		press = (buf[0] << 16) | (buf[1] << 8) | buf[3];
		temp = (buf[6] << 8) | buf[7];
		
		/* get conversion */
		t = (float)(temp - 32768);
		in[0] = s->LUT_lower + (float)(s->sensor_constants[0] * t * t) * s->quadr_factor;
		in[1] = s->offst_factor * s->sensor_constants[3] + (float)(s->sensor_constants[1] * t * t) * s->quadr_factor;
		in[2] = s->LUT_upper + (float)(s->sensor_constants[2] * t * t) * s->quadr_factor;
		calculate_conversion_constants(s->p_Pa_calib, in, out);
		
		/* pressure: hPa */
		val[0] = (out[0] + out[1] / (out[2] + press)) / 100.0f;
		
		/* temperature */
		val[1] = temp * 175.0f / 65536.0f - 45.0f;
	}
	
	return 2;
}
