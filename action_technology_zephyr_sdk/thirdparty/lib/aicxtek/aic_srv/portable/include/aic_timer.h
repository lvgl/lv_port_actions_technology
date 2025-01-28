/******************************************************************************/
/*                                                                            */
/*    Copyright 2021 by AICXTEK TECHNOLOGIES CO.,LTD. All rights reserved.    */
/*                                                                            */
/******************************************************************************/

/**
 *  DESCRIPTION
 *
 *    This file defines the system assert Interface.
 */

#ifndef __AIC_TIMER_H__
#define __AIC_TIMER_H__

#include <sys/time.h>

/*
*********************************************************************
* Description: aic_udelay
* Arguments  : usec:  延时多少us
* Return     : None
* Note(s)    : None.
*********************************************************************
*/
void aic_udelay(int usec);

/*
*********************************************************************
* Description: aic_msleep
* Arguments  : time_ms:  延时多少ms
* Return     : None
* Note(s)    : None.
*********************************************************************
*/
void aic_msleep(int time_ms);

#endif/* __AIC_TIMER_H__ */

