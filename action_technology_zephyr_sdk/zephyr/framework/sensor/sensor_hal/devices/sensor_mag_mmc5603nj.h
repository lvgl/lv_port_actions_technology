/*******************************************************************************
 * @file    sensor_mag_mmc5603nj.h
 * @author  MEMS Application Team
 * @version V1.0
 * @date    2022-08-03
 * @brief   sensor devices module
*******************************************************************************/

#ifndef _SENSOR_MAG_MMC5603NJ_H
#define _SENSOR_MAG_MMC5603NJ_H
/******************************************************************************/
//includes
/******************************************************************************/
#include <sensor_devices.h>

/******************************************************************************/
//sensor address
/******************************************************************************/
#define MMC5603NJ_ADR				(0x30)  // i2c address

/******************************************************************************/
//sensor device
//[type] sensor_dev_t: sensor_hw_t + sensor_io_t + cfg[] + func[] + task
/******************************************************************************/
#define SENSOR_DEV_MAG_MMC5603NJ                                                          \
	/*[hw] name     adr,len(adr,reg)    chip(reg,id)  sta(reg,rdy)  dat(reg,len,cmd,to)*/ \
	{ "MMC5603NJ",  MMC5603NJ_ADR,1,1,  0x39,0x10,    0x18,0x40,    0x0,9,0x0,20 },       \
	/*[io] bus(type,id,cs)  pwr(io,val)   rst(io,val,lt,ht)  int(io,mode,mfp)*/          \
	{ BUS_I2C,1,0,          MAG_POWER,0,  MAG_RESET,0,0,0,   0,0,0 },                    \
	/*[cfg] init                  enable                    disable*/                    \
	{ (void*)mmc5603nj_init_cfg,  (void*)mmc5603nj_on_cfg,  (void*)mmc5603nj_off_cfg, }, \
	/*[func] init                 self test                 converter*/                  \
	{ NULL,                       NULL,                     (void*)mmc5603nj_cvt, },     \
	/*[task] config*/                                                                    \
	(void*)&mmc5603nj_task

/******************************************************************************/
//sensor task
/******************************************************************************/
extern const i2c_task_t mmc5603nj_task;

/******************************************************************************/
//sensor config
/******************************************************************************/
extern const sensor_cfg_t mmc5603nj_init_cfg[];
extern const sensor_cfg_t mmc5603nj_on_cfg[];
extern const sensor_cfg_t mmc5603nj_off_cfg[];

/******************************************************************************/
//sensor function
/******************************************************************************/
int mmc5603nj_cvt(float *val, uint8_t *buf, uint16_t len, int16_t *raw);


#endif  /* _SENSOR_MAG_MMC5603NJ_H */
