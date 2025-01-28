/*******************************************************************************
 * @file    sensor_hr_vc9202.h
 * @author  MEMS Application Team
 * @version V1.0
 * @date    2022-08-03
 * @brief   sensor devices module
*******************************************************************************/

#ifndef _SENSOR_HR_VC9202_H
#define _SENSOR_HR_VC9202_H

/******************************************************************************/
//includes
/******************************************************************************/
#include <sensor_devices.h>

/******************************************************************************/
//sensor address
/******************************************************************************/
#define VC9202_ADR				(0x33)  // i2c address

/******************************************************************************/
//sensor device
//[type] sensor_dev_t: sensor_hw_t + sensor_io_t + cfg[] + func[] + task
/******************************************************************************/
#define SENSOR_DEV_HR_VC9202                                                      \
	/*[hw] name  adr,len(adr,reg)  chip(reg,id)    sta(reg,rdy)   dat(reg,len,cmd,to)*/   \
	{ "VC9202",  VC9202_ADR,1,1,   0x00,0x21,      REG_NULL,0x0,  REG_NULL,16,0x0,1000 }, \
	/*[io] bus(type,id,cs)  pwr(io,val)  rst(io,val,lt,ht)  int(io,mode,mfp)*/     \
	{ BUS_I2C,0,0,          HR_POWER,1,  HR_RESET,0,10,10,  HR_ISR, 0, 0 },        \
	/*[cfg] init                enable                   disable*/                 \
	{ NULL,                 NULL,                    NULL, },                  \
	/*[func] init               self test                converter*/               \
	{ NULL,                     NULL,                    NULL, },     \
	/*[task] config*/                                                              \
	(void*)&vc9202_task

/******************************************************************************/
//sensor task
/******************************************************************************/
extern const i2c_task_t vc9202_task;



#endif  /* _SENSOR_HR_GH3011_H */
