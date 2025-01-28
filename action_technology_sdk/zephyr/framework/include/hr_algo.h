/*******************************************************************************
 * @file    hr_algo.h
 * @author  MEMS Application Team
 * @version V1.0
 * @date    2021-5-25
 * @brief   hr algorithm api
*******************************************************************************/

#ifndef _HR_ALGO_H
#define _HR_ALGO_H

/******************************************************************************/
//includes
/******************************************************************************/
#include <stdint.h>
#include <stdarg.h>
#include <sys/printk.h>
#include <sensor_hal.h>
#include <sensor_algo_common.h>

/******************************************************************************/
//constants
/******************************************************************************/

/* run mode */
typedef enum {
	HB = 0,
	SPO2,
	HRV,
} hr_mode_e;

/* wear status */
typedef enum {
	WEAR = 0,
	UNWEAR,
} hr_wear_e;

/******************************************************************************/
//typedef
/******************************************************************************/
#ifdef __CC_ARM                         /* ARM Compiler */
#pragma anon_unions
#endif

/* hr result handler */
typedef void (*wear_handler_t)(uint8_t wearing_state);
typedef void (*hb_handler_t)(uint8_t hb_val, uint8_t hb_lvl_val, uint16_t rr_val);
typedef void (*spo2_handler_t)(uint8_t spo2_val, uint8_t spo2_lvl_val, uint8_t hb_val,
				uint8_t hb_lvl_val, uint16_t rr_val[4], uint8_t rr_lvl_val, uint8_t rr_cnt, uint16_t spo2_r_val);
typedef void (*hrv_handler_t)(uint16_t *rr_val_arr, uint8_t rr_val_cnt, uint8_t rr_lvl);

/* os api */
typedef struct {
	/* User handler */
	wear_handler_t wear_handler;
	hb_handler_t hb_handler;
	spo2_handler_t spo2_handler;
	hrv_handler_t hrv_handler;
	
} hr_os_api_t;

/******************************************************************************/
//function
/******************************************************************************/

/* Init sensor algo */
extern int hr_algo_init(const hr_os_api_t *api);

/* Start hr sensor */
extern int hr_algo_start(int mode);

/* Stop hr sensor */
extern int hr_algo_stop(void);

/* Process data through call-back handler */
extern int hr_algo_process(void);

#endif  /* _HR_ALGO_H */

