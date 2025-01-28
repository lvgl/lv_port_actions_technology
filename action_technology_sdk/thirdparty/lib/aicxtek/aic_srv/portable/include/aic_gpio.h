/******************************************************************************/
/*                                                                            */
/*    Copyright 2021 by AICXTEK TECHNOLOGIES CO.,LTD. All rights reserved.    */
/*                                                                            */
/******************************************************************************/

/**
 *  DESCRIPTION
 *
 *    This file defines AIC GPIO Interface.
 */

#ifndef __AIC_GPIO_H__
#define __AIC_GPIO_H__

#include "aic_type.h"

/*
*********************************************************************
* Description: aic_gpio_set_dir 设置gpio的方向
* Arguments  : gpio_id:  gpio索引号
*              gpio_dir:  0, 输出
*                         1, 输入
* Return     : None
* Note(s)    : None.
*********************************************************************
*/
void aic_gpio_set_dir(u32 gpio_id, u32 gpio_dir);

/*
*********************************************************************
* Description: aic_gpio_set_outvalue    输出高低电平
* Arguments  : gpio_id:  gpio索引号
*              gpio_value:  0, 输出低
*                           1, 输出高
* Return     : None
* Note(s)    : None.
*********************************************************************
*/
void aic_gpio_set_outvalue(u32 gpio_id, u32 gpio_value);

/*
*********************************************************************
* Description: aic_gpio_level_get
* Arguments  : gpio_id:  gpio index
* Return     : None
* Note(s)    : None.
*********************************************************************
*/
int aic_gpio_level_get(u32 gpio_id);

#endif/* __AIC_GPIO_H__ */

