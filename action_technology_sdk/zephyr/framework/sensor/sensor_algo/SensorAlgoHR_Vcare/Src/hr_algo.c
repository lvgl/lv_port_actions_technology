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
#include "vcHr02Hci.h"

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

/* Heart rate data struct */
extern vcHr02_t vcHr02;

/* Heart rate mode */
extern vcHr02Mode_t vcMode;

/* Sport Mode In Heart Rate Mode */
extern AlgoSportMode_t vcSportMode;

extern bool vcHr02IRQFlag;

extern int HeartRateValue;
extern int real_spo;

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
	
	DBG("Vcare hr_algo_init\n");
	vcHr02StopSample(&vcHr02);
	return 0;
}

/* Start hr sensor */
int hr_algo_start(int mode)
{
	vcHr02Init(&vcHr02,vcMode);
	return 0;
}

/* Stop hr sensor */
int hr_algo_stop(void)
{
	vcHr02StopSample(&vcHr02);
	return 0;
}

/* Process data through call-back handler */
int hr_algo_process(void)
{
	vcHr02IRQFlag = true;
	vcHr02_process(vcSportMode);
	hr_os_api.hb_handler(HeartRateValue, real_spo, 0);
	DBG("HeartRateValue:%d real_spo:%d\r\n", HeartRateValue,real_spo);
	return 0;
}
