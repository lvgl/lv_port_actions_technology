/*******************************************************************************
 * @file    sensor_acc_stk8321.h
 * @author  MEMS Application Team
 * @version V1.0
 * @date    2022-08-03
 * @brief   sensor devices module
*******************************************************************************/

#ifndef _SENSOR_ACC_STK8321_H
#define _SENSOR_ACC_STK8321_H

/******************************************************************************/
//includes
/******************************************************************************/
#include <sensor_devices.h>

/******************************************************************************/
//sensor address
/******************************************************************************/
#define STK8321_ADR					(0x0F)  // i2c address

/******************************************************************************/
//sensor device
//[type] sensor_dev_t: sensor_hw_t + sensor_io_t + cfg[] + func[] + task
/******************************************************************************/
#ifdef CONFIG_SENSOR_ALGO_MOTION_CYWEE_DML

#define SENSOR_DEV_ACC_STK8321                                                        \
	/*[hw] name   adr,len(adr,reg)  chip(reg,id)  sta(reg,rdy)  dat(reg,len,cmd,to)*/ \
	{ "STK8321",  STK8321_ADR,1,1,  0x0,0x23,     0x0C,1,       0x3F,6,0x0,32 },      \
	/*[io] bus(type,id,cs)  pwr(io,val)   rst(io,val,lt,ht)  int(io,mode,mfp)*/    \
	{ BUS_I2C,1,0,          ACC_POWER,0,  ACC_RESET,0,0,0,   ACC_ISR,0,11 },       \
	/*[cfg] init                enable                   disable*/                 \
	{ NULL,                     NULL,                   NULL, },                   \
	/*[func] init               self test               converter*/                \
	{ NULL,                     NULL,                   (void*)stk8321_cvt, },     \
	/*[task] config*/                                                              \
	(void*)&stk8321_task

#else

#define SENSOR_DEV_ACC_STK8321                                                        \
	/*[hw] name   adr,len(adr,reg)  chip(reg,id)  sta(reg,rdy)  dat(reg,len,cmd,to)*/ \
	{ "STK8321",  STK8321_ADR,1,1,  0x0,0x23,     0x0C,1,       0x3F,6,0x0,25 },      \
	/*[io] bus(type,id,cs)  pwr(io,val)   rst(io,val,lt,ht)  int(io,mode,mfp)*/    \
	{ BUS_I2C,1,0,          ACC_POWER,0,  ACC_RESET,0,0,0,   ACC_ISR,0,11 },       \
	/*[cfg] init                enable                   disable*/                 \
	{ (void*)stk8321_init_cfg,  (void*)stk8321_on_cfg,  (void*)stk8321_off_cfg, }, \
	/*[func] init               self test               converter*/                \
	{ NULL,                     NULL,                   (void*)stk8321_cvt, },     \
	/*[task] config*/                                                              \
	(void*)&stk8321_task

#endif

/******************************************************************************/
//sensor task
/******************************************************************************/
extern const i2c_task_t stk8321_task;

/******************************************************************************/
//sensor config
/******************************************************************************/
extern const sensor_cfg_t stk8321_init_cfg[];
extern const sensor_cfg_t stk8321_on_cfg[];
extern const sensor_cfg_t stk8321_off_cfg[];

/******************************************************************************/
//sensor function
/******************************************************************************/
int stk8321_cvt(float *val, uint8_t *buf, uint16_t len, int16_t *raw);


#endif  /* _SENSOR_ACC_STK8321_H */
