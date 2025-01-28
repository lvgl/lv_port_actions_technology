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
#include "vc9202_user.h"

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
    /* 30K counter for vcare hr algo */
    sensor_hal_config_tm(ID_HR, 0);

	DBG("vc9202 hr_algo_init\n");
	return 0;
}

/* Start hr sensor */
int hr_algo_start(int mode)
{
	DBG("vc9202 hr_algo_start\n");
    InitParamTypeDef config={800,WORK_MODE_HR};	//(修改fifo间隔时间（建议hr-800ms,spo2-400ms）, 设定工作模式)
    vc9202_usr_init(&config);
	return 0;
}

/* Stop hr sensor */
int hr_algo_stop(void)
{
	DBG("vc9202 hr_algo_stop\n");
	vc9202_usr_stop_sampling();
	return 0;
}

/* Process data through call-back handler */
int hr_algo_process(void)
{
    vc9202_usr_interrupt_handler(SPORT_TYPE_NORMAL, 1); 
	return 0;
}
