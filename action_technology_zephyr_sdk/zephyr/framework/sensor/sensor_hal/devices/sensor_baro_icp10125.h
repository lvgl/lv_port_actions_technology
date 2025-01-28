/*******************************************************************************
 * @file    sensor_baro_icp10125.h
 * @author  MEMS Application Team
 * @version V1.0
 * @date    2022-08-03
 * @brief   sensor devices module
*******************************************************************************/

#ifndef _SENSOR_BARO_ICP10125_H
#define _SENSOR_BARO_ICP10125_H

/******************************************************************************/
//includes
/******************************************************************************/
#include <sensor_devices.h>

/******************************************************************************/
//sensor address
/******************************************************************************/
#define ICP10125_ADR				(0x63)  // i2c address

/******************************************************************************/
//sensor device
//[type] sensor_dev_t: sensor_hw_t + sensor_io_t + cfg[] + func[] + task
/******************************************************************************/
#define SENSOR_DEV_BARO_ICP10125                                                   \
	/*[hw] name    adr,len(adr,reg)     chip(reg,id)    sta(reg,rdy)  dat(reg,len,cmd,to)*/   \
	{ "ICP10125",  ICP10125_ADR,  2,2,  0xefc8,0x4801,  REG_NULL,0x0,REG_NULL,9,0x5059,100 }, \
	/*[io] bus(type,id,cs)  pwr(io,val)   rst(io,val,lt,ht)  int(io,mode,mfp)*/    \
	{ BUS_I2C,1,0,          MAG_POWER,0,  MAG_RESET,0,0,0,   0,0,0 },              \
	/*[cfg] init                enable           disable*/                         \
	{ (void*)icp10125_init_cfg,   NULL,          NULL, },                          \
	/*[func] init               self test        converter*/                       \
	{ (void*)icp10125_init, 	  NULL,         (void*)icp10125_cvt, },            \
	/*[task] config*/                                                              \
	(void*)&icp10125_task

/******************************************************************************/
//sensor task
/******************************************************************************/
extern const i2c_task_t icp10125_task;

/******************************************************************************/
//sensor config
/******************************************************************************/
extern const sensor_cfg_t icp10125_init_cfg[];

/******************************************************************************/
//sensor function
/******************************************************************************/
int icp10125_init(void);
int icp10125_cvt(float *val, uint8_t *buf, uint16_t len, int16_t *raw);


#endif  /* _SENSOR_BARO_ICP10125_H */
