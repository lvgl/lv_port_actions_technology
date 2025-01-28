/*******************************************************************************
 * @file    sensor_algo.c
 * @author  MEMS Application Team
 * @version V1.0
 * @date    2021-5-25
 * @brief   sensor algorithm api
*******************************************************************************/

/******************************************************************************/
//includes
/******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sensor_algo.h>
#include <SL_Watch_Pedo_Kcal_Wrist_Sleep_Sway_L_Algorithm.h>
#include <SL_Watch_Pedo_Kcal_Wrist_Sleep_Sway_L_Application.h>

/******************************************************************************/
//constants
/******************************************************************************/
#define DBG(...)			printk(__VA_ARGS__)
//#define DBG(...)			do {} while (0)

#define PI						(3.14159265f)

/******************************************************************************/
//variables
/******************************************************************************/

/* Sensor algorithm result */
sensor_res_t sensor_algo_res = {0};

/* event handler */
sensor_handler_t user_handler = NULL;

/******************************************************************************/
//functions
/******************************************************************************/

/* Init sensor algo */
int sensor_algo_init(const sensor_os_api_t *api)
{
	// config os api
	user_handler = api->user_handler;
	
//	DBG("sensor_algo_init\r\n");
	SL_SC7A20_PEDO_KCAL_WRIST_SLEEP_SWAY_L_INIT();
	
	return 0;
}


/* Control sensor algo */
int sensor_algo_control(uint32_t ctl_id, sensor_ctl_t *ctl)
{
	return 0;
}

/* Enable sensor id */
int sensor_algo_enable(uint32_t id, uint32_t func)
{
	return 0;
}

/* Disable sensor id */
int sensor_algo_disable(uint32_t id, uint32_t func)
{
	return 0;
}

/* Input sensor raw data */
void sensor_algo_input(sensor_raw_t* dat)
{
	return;
}

/* Input sensor fifo init */
void sensor_algo_input_fifo_init(int num)
{
	return;
}

/* Input sensor fifo start */
int sensor_algo_input_fifo_start(uint32_t id, uint64_t timestamp, uint64_t delta)
{
	return 0;
}

/* Input sensor fifo end */
void sensor_algo_input_fifo_end(uint32_t id)
{
	return;
}

/* Get the duration time of the next event */
int64_t sensor_algo_get_next_duration(void)
{
	return 0;
}

/* Read sensor data and output through sensor call-back function */
int sensor_algo_process(uint32_t id)
{
	unsigned int tmp;
	// DBG("sensor_algo_process\r\n");
	tmp = SL_SC7A20_PEDO_KCAL_WRIST_SLEEP_SWAY_L_ALGO();	
//	DBG("sensor_algo_process %d\r\n",tmp);
	return 0;
}
/* Read sensor algorithm result */
sensor_res_t* sensor_algo_get_result(void)
{
//	DBG("sensor_algo_get_result %d\r\n",sensor_algo_res.handup);
	return &sensor_algo_res;
}

/* Dump sensor event */
void sensor_algo_dump_event(sensor_evt_t *evt)
{
	return;
}

/* Dump sensor result */
void sensor_algo_dump_result(sensor_res_t *res)
{
	return;
}

/* Convert sensor raw data */
void sensor_algo_convert_data(sensor_cvt_data_ag_t* dat)
{
	return;
}
