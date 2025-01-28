/*******************************************************************************
 * @file    sensor_hr_gh3011.h
 * @author  MEMS Application Team
 * @version V1.0
 * @date    2022-08-03
 * @brief   sensor devices module
*******************************************************************************/

#ifndef _SENSOR_HR_GH3011_H
#define _SENSOR_HR_GH3011_H

/******************************************************************************/
//includes
/******************************************************************************/
#include <sensor_devices.h>

/******************************************************************************/
//sensor address
/******************************************************************************/
#define GH3011_ADR				(0x14)  // i2c address

/******************************************************************************/
//sensor device
//[type] sensor_dev_t: sensor_hw_t + sensor_io_t + cfg[] + func[] + task
/******************************************************************************/
#define SENSOR_DEV_HR_GH3011                                                       \
	/*[hw] name  adr,len(adr,reg)  chip(reg,id)    sta(reg,rdy)   dat(reg,len,cmd,to)*/   \
	{ "GH3011",  GH3011_ADR,2,2,   0xffff,0xffff,  REG_NULL,0x0,  REG_NULL,16,0x0,1000 }, \
	/*[io] bus(type,id,cs)  pwr(io,val)  rst(io,val,lt,ht)  int(io,mode,mfp)*/     \
	{ BUS_I2C,0,0,          HR_POWER,1,  HR_RESET,0,10,10,  HR_ISR, 0, 0 },        \
	/*[cfg] init                enable                   disable*/                 \
	{ (void*)gh3011_init_cfg,   NULL,                    NULL, },                  \
	/*[func] init               self test                converter*/               \
	{ NULL,                     NULL,                    (void*)gh3011_cvt, },     \
	/*[task] config*/                                                              \
	(void*)&gh3011_task

/******************************************************************************/
//sensor task
/******************************************************************************/
extern const i2c_task_t gh3011_task;

/******************************************************************************/
//sensor config
/******************************************************************************/
extern const sensor_cfg_t gh3011_init_cfg[];

/******************************************************************************/
//sensor function
/******************************************************************************/
int gh3011_cvt(float *val, uint8_t *buf, uint16_t len, int16_t *raw);


#endif  /* _SENSOR_HR_GH3011_H */
