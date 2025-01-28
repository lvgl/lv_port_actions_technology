/*******************************************************************************
 * @file    sensor_hr_hx3690.h
 * @author  MEMS Application Team
 * @version V1.0
 * @date    2022-08-03
 * @brief   sensor devices module
*******************************************************************************/

#ifndef _SENSOR_HR_HX3690_H
#define _SENSOR_HR_HX3690_H

/******************************************************************************/
//includes
/******************************************************************************/
#include <sensor_devices.h>

/******************************************************************************/
//sensor address
/******************************************************************************/
#define HX3690_ADR				(0x44)  // i2c address

/******************************************************************************/
//sensor device
//[type] sensor_dev_t: sensor_hw_t + sensor_io_t + cfg[] + func[] + task
/******************************************************************************/
#define SENSOR_DEV_HR_HX3690                                                     \
	/*[hw] name  adr,len(adr,reg)  chip(reg,id)    sta(reg,rdy)   dat(reg,len,cmd,to)*/   \
	{ "HX3690",  HX3690_ADR,1,1,   0x00,0x69,      REG_NULL,0x0,  REG_NULL,16,0x0,1000 }, \
	/*[io] bus(type,id,cs)  pwr(io,val)  rst(io,val,lt,ht)  int(io,mode,mfp)*/     \
	{ BUS_I2C,0,0,          HR_POWER,1,  HR_RESET,0,10,10,  0, 0, 0 },        \
	/*[cfg] init                enable                   disable*/                 \
	{ NULL,                 NULL,                    NULL, },                  \
	/*[func] init               self test                converter*/               \
	{ NULL,                     NULL,                    NULL, },     \
	/*[task] config*/                                                              \
	(void*)&hx3690_task

/******************************************************************************/
//sensor task
/******************************************************************************/
extern const i2c_task_t hx3690_task;



#endif  /* _SENSOR_HR_GH3011_H */
