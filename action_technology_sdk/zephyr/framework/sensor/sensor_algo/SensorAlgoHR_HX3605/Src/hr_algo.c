/*******************************************************************************
 * @file    hr_algo.c
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
#include <hr_algo.h>
#include <hx3605.h>
#include <hx3605_hrs_agc.h>

/******************************************************************************/
//constants
/******************************************************************************/
#define DBG(...) printk(__VA_ARGS__)
//#define DBG(...)			do {} while (0)

/******************************************************************************/
//variables
/******************************************************************************/
/* hr os api */
hr_os_api_t hr_os_api = { 0 };

extern bool timer_40ms_en;
extern bool timer_320ms_en;
extern uint8_t timer_320ms_cnt;

/******************************************************************************/
//functions
/******************************************************************************/
/* Init sensor algo */
int hr_algo_init(const hr_os_api_t *api)
{
	// init os api
	if (api == NULL) {
		return -1;
	}
	hr_os_api = *api;

	return 0;
}

/* Start hr sensor */
int hr_algo_start(int mode)
{
	bool ret = false;
	switch (mode) {
	case HB:
		ret = hx3605_init(HRS_MODE);
		break;
	case SPO2:
		ret = hx3605_init(SPO2_MODE);
		break;
	case HRV:
		ret = hx3605_init(HRV_MODE);
		break;
	case HR_WEAR:
		ret = hx3605_init(WEAR_MODE);
		break;
	default:
		break;
	}
	DBG("hr_algo_start:%d, ret:%d\n", mode, ret);
	return ret;
}

/* Stop hr sensor */
int hr_algo_stop(void)
{
	hx3605_hrs_disable();
	return 0;
}

/* Process data through call-back handler */
int hr_algo_process(void)
{
	if (timer_40ms_en)
		hx3605_agc_Int_handle();

	if (timer_320ms_en) {
		timer_320ms_cnt++;
		if (timer_320ms_cnt == 8) {
			timer_320ms_cnt = 0;
			ppg_timeout_handler(0);
		}
	}
	return 0;
}
