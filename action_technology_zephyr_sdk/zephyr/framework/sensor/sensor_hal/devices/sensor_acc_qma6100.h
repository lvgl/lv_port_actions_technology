/*******************************************************************************
 * @file    sensor_acc_qma6100.h
 * @author  MEMS Application Team
 * @version V1.0
 * @date    2022-08-03
 * @brief   sensor devices module
*******************************************************************************/

#ifndef _SENSOR_ACC_QMA6100_H
#define _SENSOR_ACC_QMA6100_H

/******************************************************************************/
//includes
/******************************************************************************/
#include <sensor_devices.h>

/******************************************************************************/
//sensor address
/******************************************************************************/
#define QMA6100_ADR					(0x12)  // i2c address

/******************************************************************************/
//sensor device
//[type] sensor_dev_t: sensor_hw_t + sensor_io_t + cfg[] + func[] + task
/******************************************************************************/
#ifdef CONFIG_SENSOR_ALGO_MOTION_CYWEE_DML

#define SENSOR_DEV_ACC_QMA6100                                                        \
	/*[hw] name   adr,len(adr,reg)  chip(reg,id)  sta(reg,rdy)  dat(reg,len,cmd,to)*/ \
	{ "QMA6100",  QMA6100_ADR,1,1,  0x0,0x90,     0x0C,1,       0x3F,6,0x0,20 },      \
	/*[io] bus(type,id,cs)  pwr(io,val)   rst(io,val,lt,ht)  int(io,mode,mfp)*/    \
	{ BUS_I2C,1,0,          ACC_POWER,0,  ACC_RESET,0,0,0,   ACC_ISR,0,11 },       \
	/*[cfg] init                enable                   disable*/                 \
	{ NULL,                       NULL,                    NULL, },                \
	/*[func] init               self test               converter*/                \
	{ NULL,                     NULL,                   (void*)qma6100_cvt, },     \
	/*[task] config*/                                                              \
	(void*)&qma6100_task

#else

#define SENSOR_DEV_ACC_QMA6100                                                        \
	/*[hw] name   adr,len(adr,reg)  chip(reg,id)  sta(reg,rdy)  dat(reg,len,cmd,to)*/ \
	{ "QMA6100",  QMA6100_ADR,1,1,  0x0,0x90,     0x0C,1,       0x3F,6,0x0,20 },      \
	/*[io] bus(type,id,cs)  pwr(io,val)   rst(io,val,lt,ht)  int(io,mode,mfp)*/    \
	{ BUS_I2C,1,0,          ACC_POWER,0,  ACC_RESET,0,0,0,   ACC_ISR,0,11 },       \
	/*[cfg] init                enable                   disable*/                 \
	{ (void*)qma6100_init_cfg,  (void*)qma6100_on_cfg,  (void*)qma6100_off_cfg, }, \
	/*[func] init               self test               converter*/                \
	{ NULL,                     NULL,                   (void*)qma6100_cvt, },     \
	/*[task] config*/                                                              \
	(void*)&qma6100_task

#endif

/******************************************************************************/
//sensor task
/******************************************************************************/
extern const i2c_task_t qma6100_task;

/******************************************************************************/
//sensor config
/******************************************************************************/
extern const sensor_cfg_t qma6100_init_cfg[];
extern const sensor_cfg_t qma6100_on_cfg[];
extern const sensor_cfg_t qma6100_off_cfg[];

/******************************************************************************/
//sensor function
/******************************************************************************/
int qma6100_cvt(float *val, uint8_t *buf, uint16_t len, int16_t *raw);


#endif  /* _SENSOR_ACC_QMA6100_H */
