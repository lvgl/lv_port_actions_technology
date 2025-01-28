/*******************************************************************************
 * @file    sensor_acc_lis2dw12.h
 * @author  MEMS Application Team
 * @version V1.0
 * @date    2022-08-03
 * @brief   sensor devices module
*******************************************************************************/

#ifndef _SENSOR_ACC_LIS2DW12_H
#define _SENSOR_ACC_LIS2DW12_H

/******************************************************************************/
//includes
/******************************************************************************/
#include <sensor_devices.h>

/******************************************************************************/
//sensor address
/******************************************************************************/
#define LIS2DW12_ADR				(0x19)  // i2c address

/******************************************************************************/
//sensor device
//[type] sensor_dev_t: sensor_hw_t + sensor_io_t + cfg[] + func[] + task
/******************************************************************************/
#ifdef CONFIG_SENSOR_ALGO_MOTION_CYWEE_DML

#define SENSOR_DEV_ACC_LIS2DW12                                                         \
	/*[hw] name    adr,len(adr,reg)   chip(reg,id)  sta(reg,rdy)  dat(reg,len,cmd,to)*/ \
	{ "LIS2DW12",  LIS2DW12_ADR,1,1,  0xf,0x44,     0x27,0x1,     0x28,6,0x0,20 },      \
	/*[io] bus(type,id,cs)  pwr(io,val)   rst(io,val,lt,ht)  int(io,mode,mfp)*/         \
	{ BUS_I2C,1,0,          ACC_POWER,0,  ACC_RESET,0,0,0,   ACC_ISR,0,11 },            \
	/*[cfg] init                  enable                   disable*/                    \
	{ NULL,                       NULL,                    NULL, },                     \
	/*[func] init                 self_test                converter*/                  \
	{ NULL,                       NULL,                    (void*)lisdw12_cvt, },       \
	/*[task] config*/                                                                   \
	(void*)&lisdw12_task

#else

#define SENSOR_DEV_ACC_LIS2DW12                                                         \
	/*[hw] name    adr,len(adr,reg)   chip(reg,id)  sta(reg,rdy)  dat(reg,len,cmd,to)*/ \
	{ "LIS2DW12",  LIS2DW12_ADR,1,1,  0xf,0x44,     0x27,0x1,     0x28,6,0x0,20 },      \
	/*[io] bus(type,id,cs)  pwr(io,val)   rst(io,val,lt,ht)  int(io,mode,mfp)*/         \
	{ BUS_I2C,1,0,          ACC_POWER,0,  ACC_RESET,0,0,0,   ACC_ISR,0,11 },            \
	/*[cfg] init                  enable                   disable*/                    \
	{ (void*)lisdw12_init_cfg,    (void*)lisdw12_on_cfg,   (void*)lisdw12_off_cfg, },   \
	/*[func] init                 self_test                converter*/                  \
	{ NULL,                       NULL,                    (void*)lisdw12_cvt, },       \
	/*[task] config*/                                                                   \
	(void*)&lisdw12_task

#endif

/******************************************************************************/
//sensor task
/******************************************************************************/
extern const i2c_task_t lisdw12_task;

/******************************************************************************/
//sensor config
/******************************************************************************/
extern const sensor_cfg_t lisdw12_init_cfg[];
extern const sensor_cfg_t lisdw12_on_cfg[];
extern const sensor_cfg_t lisdw12_off_cfg[];

/******************************************************************************/
//sensor function
/******************************************************************************/
int lisdw12_cvt(float *val, uint8_t *buf, uint16_t len, int16_t *raw);


#endif  /* _SENSOR_ACC_LIS2DW12_H */
