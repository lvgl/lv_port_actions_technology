/*********************************************************************************************************
 *               Copyright(c) 2022, vcare Corporation. All rights reserved.
 **********************************************************************************************************
 * @file     vc9202_common.h
 * @brief    configuration file for the hardware driver layer
 * @details  
 * @author   
 * @date
 * @version  
 *********************************************************************************************************/
#ifndef __VC9202_COMMON_H__
#define __VC9202_COMMON_H__

#include "vc30x91x92x_common.h"
#include <string.h>
#include <hr_algo.h>
#include <sys/printk.h>

extern hr_os_api_t hr_os_api;


#define VC9202_VC9202_CFG_DRV_BY_TIMER  (FUNC_DISABLE)
/* VC_COMMON_VC9202_CFG_SAMPLE_RATE: default 25hz */
#define VC9202_CFG_SAMPLE_RATE    (25)
/*VC_COMMON_CFG_GPIO_TIME: 0-50us, 1-100us, 2-200us, 3-400us */
#define VC9202_CFG_INTERRUPT_TIME (3)
/* 0:rising edge 1:descending edge */
#define VC9202_CFG_INTERRUPT_MODE (0)

#define VC9202_CFG_CLK_CHECK_FREQUENCY (30000)      //counter 30k
#define VC9202_CFG_CLK_CHECK_MAX       (0xFFFFFF) //24bits counter
#define VC9202_CFG_CLK_CHECK_GAPTIME   (10)        //seconds 
#define VC9202_CFG_CLK_CHECK_DEVIATION (3)

/****************************************************************************
 * Configuration wear parameters 
 ***************************************************************************/
#define VC9202_CFG_WEAR_DETECTION       (FUNC_ENABLE) // FUNC_DISABLE  FUNC_ENABLE
#define VC9202_CFG_WEAR_DEFAULT_STATUS  (WEAR_IS_HOLD)  //1 is  default wear,0 is default not wear  WEAR_IS_HOLD  WEAR_IS_DROP
#define VC9202_CFG_WEAR_BIO_EN        (FUNC_ENABLE)
#define VC9202_CFG_WEAR_ALG_EN        (FUNC_DISABLE) 	//开启心率摘下功能，必须开启满足(ATC|BIO)为真
#define VC9202_CFG_WEAR_ACT_EN        (FUNC_DISABLE)
#define VC9202_CFG_WEAR_THRESHOLD_EN        (FUNC_ENABLE)    //绝对阈值宏开关
#define VC9202_CFG_WEAR_HOLD_CNT    (1)
#define VC9202_CFG_WEAR_DROP_CNT    (3)

/* sensor serial default wear param */
#define VC9202_CFG_WEAR_PARAM_PS     (128)
#define VC9202_CFG_WEAR_PARAM_PSIN   (100)
#define VC9202_CFG_WEAR_PARAM_ENVMAX (128) //8*16
#define VC9202_CFG_WEAR_PARAM_ENVMIN (64)  //4*16
#define VC9202_CFG_WEAR_PARAM_BIOIN  (40)

#define VC9202_CFG_PS_SLOT_CUR      (0x3A)	/* 0x5A */
#define VC9202_CFG_PS_SLOT_RES      (0x47)

/* board cap halve,电容减半，需要提高一倍采样率的配置  */
#define VC9202_CFG_IS_HALVE_CAP     0

/* VC9202 function */
extern unsigned char vc9202_drv_reg_init(InitParamTypeDef *pInit_Param);
extern unsigned char vc9202_drv_get_chip_id(void);
extern void vc9202_drv_start_sampling(void);
extern void vc9202_drv_stop_sampling(void);
extern void vc9202_drv_soft_reset(void);
extern unsigned char vc9202_drv_channel_switch(unsigned char num_channel, unsigned char enable_flag);
extern unsigned char vc9202_drv_get_int_reason(void);
extern void vc9202_drv_set_algo_wear_status(WearStatusTypeDef status);
extern void vc9202_drv_set_gsensor_actuating_quantity(unsigned int axis_diff);
extern WearStatusTypeDef vc9202_drv_get_wear_status(void);
extern unsigned char vc9202_drv_get_sampling_data(SamplingDataTypeDef *pSamplingData, unsigned char *psize);
/* electrostatic patch */
extern void vc9202_drv_electrostatic_death( void );

#endif
