/*******************************************************************************
 * @file    sensor_devices.c
 * @author  MEMS Application Team
 * @version V1.0
 * @date    2022-08-03
 * @brief   sensor devices module
*******************************************************************************/

/******************************************************************************/
//includes
/******************************************************************************/
#include <sensor_devices.h>

/******************************************************************************/
//sensor device
/******************************************************************************/

const sensor_dev_t acc_sensor_dev[] = {
#if CONFIG_SENSOR_ACC_LIS2DW12
		{
		SENSOR_DEV_ACC_LIS2DW12,
		},
#endif
#if CONFIG_SENSOR_ACC_STK8321
		{
		SENSOR_DEV_ACC_STK8321,
		},
#endif
#if CONFIG_SENSOR_ACC_SC7A20
		{
		SENSOR_DEV_ACC_SC7A20,
		},
#endif
#if CONFIG_SENSOR_ACC_QMA6100
		{
		SENSOR_DEV_ACC_QMA6100,
		},
#endif
		{
			SENSOR_DEV_NULL,
		},
};

const sensor_dev_t gyro_sensor_dev[] = {
		/* GYRO */
		{
			SENSOR_DEV_NULL,
		},
};

const sensor_dev_t mag_sensor_dev[] = {
		/* MAG */
#if CONFIG_SENSOR_MAG_MMC5603NJ
		{
			SENSOR_DEV_MAG_MMC5603NJ,
		},
#endif
		{
			SENSOR_DEV_NULL,
		},
};


const sensor_dev_t baro_sensor_dev[] = {
		/* BARO */
#if CONFIG_SENSOR_BARO_ICP10125
		{
			SENSOR_DEV_BARO_ICP10125,
		},
#endif
		{
			SENSOR_DEV_NULL,
		},
};

const sensor_dev_t temp_sensor_dev[] = {
		/* TEMP */
		{
			SENSOR_DEV_NULL,
		},
};


const sensor_dev_t hr_sensor_dev[] = {
		/* HR */
#if CONFIG_SENSOR_HR_GH3011
		{
			SENSOR_DEV_HR_GH3011,
		},
#endif
#if CONFIG_SENSOR_HR_VC9201
		{
			SENSOR_DEV_HR_VC9201,
		},
#endif
#if CONFIG_SENSOR_HR_VC9202
		{
			SENSOR_DEV_HR_VC9202,
		},
#endif
#if CONFIG_SENSOR_HR_HX3605
		{
			SENSOR_DEV_HR_HX3605,
		},
#endif
#if CONFIG_SENSOR_HR_HX3690
		{
			SENSOR_DEV_HR_HX3690,
		},
#endif
		{
			SENSOR_DEV_NULL,
		},

};

const sensor_dev_t gnss_sensor_dev[] = {
		/* GNSS */
		{
			SENSOR_DEV_NULL,
		},
};


const sensor_dev_t offbody_sensor_dev[] = {
		/* OFFBODY */
		{
			SENSOR_DEV_NULL,
		},
};

const sensor_dev_t *sensor_dev_list[] = {
	acc_sensor_dev,
	gyro_sensor_dev,
	mag_sensor_dev,
	baro_sensor_dev,
	temp_sensor_dev,
	hr_sensor_dev,
	gnss_sensor_dev,
	offbody_sensor_dev,
};

//const sensor_dev_t sensor_dev[] = 
#if 0
__act_s2_sleep_data sensor_dev_t sensor_dev[] = 
{
	{ /* ACC */
		SENSOR_DEV_NULL,
	},

	{ /* GYRO */
		SENSOR_DEV_NULL,
	},

	{ /* MAG */
		SENSOR_DEV_NULL,
	},

	{ /* BARO */
		SENSOR_DEV_NULL,
	},

	{ /* TEMP */
		SENSOR_DEV_NULL,
	},

	{ /* HR */
		SENSOR_DEV_NULL,
	},

	{ /* GNSS */
		SENSOR_DEV_NULL,
	},

	{ /* OFFBODY */
		SENSOR_DEV_NULL,
	},
};
#endif
