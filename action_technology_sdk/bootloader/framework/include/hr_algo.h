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
	/* Printf */
  int (*dbgOutput)(const char *fmt, ...);
	
	/* I2C write func */
	int (*i2c_write)(uint8_t addr, uint16_t reg, uint8_t *buf, uint16_t len);
	
	/* I2C read func */
	int (*i2c_read)(uint8_t addr, uint16_t reg, uint8_t *buf, uint16_t len);
	
	/* User handler */
	wear_handler_t wear_handler;
	hb_handler_t hb_handler;
	spo2_handler_t spo2_handler;
	hrv_handler_t hrv_handler;
	
} hr_os_api_t;


/* Hr algo api */
typedef struct {
	/* Init hr algo */
	int (*init)(const hr_os_api_t *api);
	
	/* Get os api */
	hr_os_api_t* (*get_os_api)(void);
	
	/* Set os api */
	int (*set_os_api)(const hr_os_api_t *api);
	
	/* Start hr sensor */
	int (*start)(int mode);
	
	/* Stop hr sensor */
	int (*stop)(void);
	
	/* Process data through call-back handler */
	int (*process)(void);
	
} hr_algo_api_t;


/******************************************************************************/
//function
/******************************************************************************/

/* Init sensor algo */
extern int hr_algo_init(const hr_os_api_t *api);

/* Get os api */
extern hr_os_api_t* hr_algo_get_os_api(void);

/* Set os api */
extern int hr_algo_set_os_api(const hr_os_api_t *api);

/* Start hr sensor */
extern int hr_algo_start(int mode);

/* Stop hr sensor */
extern int hr_algo_stop(void);

/* Process data through call-back handler */
extern int hr_algo_process(void);

/******************************************************************************/
//api
/******************************************************************************/

/* sensor algo api */
extern char __hr_algo_start[];
extern char __hr_algo_end[];
#define __hr_algo_size	(__hr_algo_end - __hr_algo_start)
#define p_hr_algo_api	((const hr_algo_api_t*)__hr_algo_start)

#endif  /* _HR_ALGO_H */

