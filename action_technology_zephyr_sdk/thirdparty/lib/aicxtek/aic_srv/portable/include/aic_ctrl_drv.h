/**************************************************************************/
/*                                                                        */
/* Copyright 2023 by AICXTEK TECHNOLOGIES CO.,LTD. All rights reserved.   */
/*                                                                        */
/**************************************************************************/

#ifndef __AIC_CTRL_DRV_H__
#define __AIC_CTRL_DRV_H__

#include <aic_type.h>

/*
*********************************************************************
* Description: aic_ctrl_active_vbat_pin
* active or deactive the pwr pin.
*
* Arguments  : active:
*              0: deactive
*              1: active
*
* Return     : None
* Note(s)    : Need to implemented it base on actual harfware design.
*********************************************************************
*/
void aic_ctrl_active_power_pin(bool active);

#endif /* __AIC_CTRL_DRV_H__ */
