/*******************************************************************************
 * @file    sensor_port.c
 * @author  MEMS Application Team
 * @version V1.0
 * @date    2020-08-12
 * @brief   sensor testing
*******************************************************************************/

/******************************************************************************/
//includes
/******************************************************************************/
#include <stdio.h>
#include <string.h>
#include <zephyr.h>
#include <drivers/ipmsg.h>
#include <rbuf/rbuf_msg_sc.h>
#include "sensor_port.h"
#include "sensor_manager.h"
#include <os_common_api.h>
#include <sys/ring_buffer.h>

/******************************************************************************/
//constants
/******************************************************************************/
#define MAX_SENSOR_DATA_NUM			(4)

/******************************************************************************/
//variables
/******************************************************************************/
static void _sensor_work_handler(struct k_work *work);

static sensor_dat_t sensor_dat[MAX_SENSOR_DATA_NUM];
static struct ring_buf sensor_rbuf;
static os_delayed_work sensor_work;

/******************************************************************************/
//functions
/******************************************************************************/
static void _sensor_work_handler(struct k_work *work)
{
	sensor_send_msg(MSG_SENSOR_DATA, 0, NULL, 0);
}

static void sensor_task_callback(int id, sensor_dat_t *dat, void *ctx)
{
	int ret = ring_buf_put(&sensor_rbuf, (const uint8_t *)dat, sizeof(sensor_dat_t));
	if (!ret) {
		SYS_LOG_ERR("sensor rbuf full");
	}
#ifdef CONFIG_USER_WORK_Q
	os_work_q *work_queue = os_get_user_work_queue();
	os_delayed_work_submit_to_queue(work_queue, &sensor_work, 0);
#else
	os_delayed_work_submit(&sensor_work, 0);
#endif
}

int sensor_init(void)
{
	int ret;

	/* init sensor */
	ret = sensor_hal_init();
	if(ret != 0) {
		SYS_LOG_ERR("sensor init failed!");
	}
	
	/* add callback */
	sensor_hal_add_callback(ID_ACC, sensor_task_callback, NULL);
	sensor_hal_add_callback(ID_MAG, sensor_task_callback, NULL);
	sensor_hal_add_callback(ID_BARO, sensor_task_callback, NULL);
	sensor_hal_add_callback(ID_HR, sensor_task_callback, NULL);
	sensor_hal_add_callback(ID_GYRO, sensor_task_callback, NULL);

	ring_buf_init(&sensor_rbuf, sizeof(sensor_dat), sensor_dat);
	os_delayed_work_init(&sensor_work, _sensor_work_handler);

	return ret;
}

int sensor_poll(void)
{
	return 0;
}

int sensor_get_data(sensor_dat_t *pdat)
{
	return ring_buf_get(&sensor_rbuf, (uint8_t *)pdat, sizeof(sensor_dat_t));
}

int sensor_data_is_empty(void)
{
	return ring_buf_is_empty(&sensor_rbuf);
}

