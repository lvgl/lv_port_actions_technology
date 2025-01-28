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
#include <string.h>
#include <sensor_algo.h>

/******************************************************************************/
//functions
/******************************************************************************/
/* Init sensor algo */
int sensor_algo_init(const sensor_os_api_t *api)
{
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
	return 0;
}

/* Read sensor algorithm result */
sensor_res_t* sensor_algo_get_result(void)
{
	return NULL;
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
