/*******************************************************************************
 * @file    algo_port.c
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
#include <soc.h>
#include <sys_manager.h>
#include <sensor_hal.h>
#include <sensor_algo.h>
#include <hr_algo.h>
#include <os_common_api.h>
#include <linker/linker-defs.h>
#ifdef CONFIG_SYS_WAKELOCK
#include <sys_wakelock.h>
#endif
#include "sensor_port.h"
#include "sensor_sleep.h"
#include "algo_port.h"

/******************************************************************************/
//declarations
/******************************************************************************/
extern int sensor_service_callback(int evt_id, sensor_res_t *res);

static void usr_evt_handler(sensor_evt_t *evt);
static void hr_wear_handler(uint8_t wearing_state);
static void hr_hb_handler(uint8_t hb_val, uint8_t hb_lvl_val, uint16_t rr_val);
static void hr_spo2_handler(uint8_t spo2_val, uint8_t spo2_lvl_val, uint8_t hb_val,
				uint8_t hb_lvl_val, uint16_t rr_val[4], uint8_t rr_lvl_val, uint8_t rr_cnt, uint16_t spo2_r_val);

/******************************************************************************/
//variables
/******************************************************************************/
const static sensor_os_api_t os_api =
{
	.user_handler = usr_evt_handler,
};

const static hr_os_api_t hr_os_api =
{
	.wear_handler = hr_wear_handler,
	.hb_handler = hr_hb_handler,
	.spo2_handler = hr_spo2_handler,
	.hrv_handler = NULL,
};

/******************************************************************************/
//functions
/******************************************************************************/
__sleepfunc static void usr_evt_handler(sensor_evt_t *evt)
{
	sensor_res_t *res = sensor_algo_get_result();
	
	// dump event
//	sensor_algo_dump_event(evt);

	// process in sleep mode
	if (soc_in_sleep_mode()) {
		switch(evt->id) {
			case ALGO_HANDUP:
				printk("handup=%d\r\n", res->handup);
				if (res->handup == 1) {
					sensor_sleep_wakeup(1);
				}
				break;

			case RAW_HEARTRATE:
				printk("hr=%d\r\n", (int)res->heart_rate);
				if (res->heart_rate > 0) {
					sensor_sleep_wakeup(0);
				}
				break;
		}
	} else {
		// process in system mode
		switch(evt->id) {
			case REQ_SENSOR:
				if ((int)evt->fData[2]) {
					sensor_hal_enable((int)evt->fData[1]);
					if ((int)evt->fData[1] == IN_HEARTRATE) {
						hr_algo_start(HB);
					}
				} else {
					sensor_hal_disable((int)evt->fData[1]);
					if ((int)evt->fData[1] == IN_HEARTRATE) {
						hr_algo_stop();
					}
				}
				break;

			case ALGO_ACTIVITY_OUTPUT:
				sensor_algo_dump_result(res);
				break;

			case ALGO_HANDUP:
				SYS_LOG_INF("handup=%d", res->handup);
#ifdef CONFIG_SYS_WAKELOCK
				if (res->handup == 1) {
					sys_wake_lock(FULL_WAKE_LOCK);
					sys_wake_unlock(FULL_WAKE_LOCK);
				} else if (res->handup == 2) {
					system_request_fast_standby();
				}
#endif
				break;
		}

		// callback
		sensor_service_callback(evt->id, res);
	}
}

static void hr_wear_handler(uint8_t wearing_state)
{
	sensor_raw_t sdata;
	
	// input wear data
	memset(&sdata, 0, sizeof(sdata));
	sdata.id = IN_OFFBODY;
	sdata.fData[0] = wearing_state;
	sensor_algo_input(&sdata);
	
	// call algo
	sensor_algo_process(sdata.id);
}

static void hr_hb_handler(uint8_t hb_val, uint8_t hb_lvl_val, uint16_t rr_val)
{
	sensor_raw_t sdata;
	
	// input wear data
	memset(&sdata, 0, sizeof(sdata));
	sdata.id = IN_HEARTRATE;
	sdata.fData[0] = hb_val;
	sdata.fData[1] = (hb_lvl_val == 0) ? 3 : 0;
	sdata.fData[2] = 200;
	sdata.fData[3] = 40;
	sensor_algo_input(&sdata);
	
	// call algo
	sensor_algo_process(sdata.id);
}

static void hr_spo2_handler(uint8_t spo2_val, uint8_t spo2_lvl_val, uint8_t hb_val,
				uint8_t hb_lvl_val, uint16_t rr_val[4], uint8_t rr_lvl_val, uint8_t rr_cnt, uint16_t spo2_r_val)
{
	// TODO
}

void algo_init(void)
{
	// init algo
	sensor_algo_init(&os_api);
	
	// init fifo
	sensor_algo_input_fifo_init(20);

	// init heart-rate algo
	hr_algo_init(&hr_os_api);
}

__sleepfunc void algo_handler(int id, sensor_dat_t *dat)
{
	int idx;
	uint64_t ts, delta;
	float val[3];
	sensor_raw_t sdata;
	int ret;
#ifdef CONFIG_SENSOR_ALGO_MOTION_CYWEE_DML
	sensor_cvt_ag_t ag_cov = {0};
#endif

	//printk("sensor algo_handler id=%d, evt=%d\n", id, dat->evt);

	// input data
	switch(id) {
		case ID_ACC:
		case ID_MAG:
			if (dat) {
				// start fifo
				if (dat->cnt > 1) {
					ts = dat->ts * 1000000ull;
#ifdef CONFIG_SENSOR_ALGO_MOTION_CYWEE_DML
					delta = sensor_algo_get_data_period(id) * 1000000ull;
#else
					delta = dat->pd * 1000000ull;
#endif
					sensor_algo_input_fifo_start(id, ts, delta);
				}
				// input data
				for (idx = 0; idx < dat->cnt; idx ++) {
					// get value
#ifdef CONFIG_SENSOR_ALGO_MOTION_CYWEE_DML
					sensor_hal_cvt_data(id, dat->buf + dat->sz * idx, dat->sz, NULL, ag_cov.acc_raw);
					sensor_algo_convert_data(&ag_cov);
					memcpy(val, ag_cov.acc_data, sizeof(float) *3);
#else
					sensor_hal_get_value(id, dat, idx, val);
#endif
					//printk("%s %.2f %.2f %.2f\n", sensor_hal_get_name(id), val[0], val[1], val[2]);

					// input acc data
					memset(&sdata, 0, sizeof(sensor_raw_t));
					sdata.id = id;
					memcpy(sdata.fData, val, sizeof(float) *3);
					sensor_algo_input(&sdata);
				}
				// end fifo
				if (dat->cnt > 1) {
					sensor_algo_input_fifo_end(id);
				}
			}
			// call algo
			sensor_algo_process(id);
			break;
			
		case ID_BARO:
			if (dat) {
				if (dat->cnt == 0) {
					ret = sensor_hal_poll_data(id, dat, NULL);
					if (ret) {
						printk("sensor %d poll failed!\n", id);
						break;
					}
				}
				// get value
				sensor_hal_get_value(id, dat, 0, val);
				//printk("%s %0.2f %0.2f\n", sensor_hal_get_name(id), val[0], val[1]);

				// input baro data
				memset(&sdata, 0, sizeof(sdata));
				sdata.id = IN_BARO;
				sdata.fData[0] = val[0];
				sdata.fData[1] = 1.0f;
				sdata.fData[2] = val[1];
				sensor_algo_input(&sdata);
			}
			// call algo
			sensor_algo_process(id);
			break;
		
		case ID_HR:
			// call algo
			hr_algo_process();
			break;
	}
}

