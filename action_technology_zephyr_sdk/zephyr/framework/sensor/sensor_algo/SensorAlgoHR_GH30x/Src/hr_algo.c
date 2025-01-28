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
#include "gh3011_example.h"

/******************************************************************************/
//constants
/******************************************************************************/
#define DBG(...)			printk(__VA_ARGS__)
//#define DBG(...)			do {} while (0)

/******************************************************************************/
//variables
/******************************************************************************/
/* hr os api */
hr_os_api_t hr_os_api = {0};

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
	EMGh30xRunMode gh3011_mode = (EMGh30xRunMode)HB;
	
	switch(mode) {
		case HB:
			gh3011_mode = GH30X_RUN_MODE_HB;
			break;
		case SPO2:
			gh3011_mode = GH30X_RUN_MODE_SPO2;
			break;
		case HRV:
			gh3011_mode = GH30X_RUN_MODE_HRV;
			break;
	}
	
	gh30x_module_init();
	GH30X_HBA_SCENARIO_CONFIG(0);
	gh30x_module_start(gh3011_mode);
	
	return 0;
}

/* Stop hr sensor */
int hr_algo_stop(void)
{
	gh30x_module_stop();
	return 0;
}

/* Process data through call-back handler */
int hr_algo_process(void)
{
	gh30x_int_msg_handler();
	return 0;
}
