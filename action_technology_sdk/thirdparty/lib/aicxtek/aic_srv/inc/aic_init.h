/******************************************************************************/
/*                                                                            */
/*    Copyright 2023 by AICXTEK TECHNOLOGIES CO.,LTD. All rights reserved.    */
/*                                                                            */
/******************************************************************************/

/**
 *  DESCRIPTION
 *
 *    This file defines the aicxtek init Interface.
 */


#ifndef __AIC_INIT_H__
#define __AIC_INIT_H__

#include <stdint.h>
#include "aic_tele.h"
#include "aic_srv_at.h"
#include "aic_srv_cfg.h"
#include "aic_srv_wifi_scan.h"

/*
 * aic_cmux_init
 * Init cmux module.
 */
int aic_cmux_init(void);
/*
 * aic_ctrl_init
 * Init control module.
 */
int aic_ctrl_init(void);
/*
 * aic_net_mgr_init
 * Init net mangenet module.
 */
void aic_net_mgr_init(void);

/*
 * aic_common_poweroff_init
 * Init common poweroff module.
 */
int32_t aic_common_poweroff_init(void);

/*
 * aic_cmux_init_task
 * create task after init cmux.
 */
int aic_cmux_init_task(void);

/*
 * aic_net_init
 * Init netowrk module.
 */
void aic_net_init(void);

/*
*********************************************************************
* Description: aicxtek 初始化接口
* Arguments  : void
* Return     : No return
* Note(s)    : aicxtek初始化接口，必须在系统boot流程中调用，例如，在
*              平台module init时调用。
*********************************************************************
*/
static inline void aic_init(void)
{
    aic_cmux_init();
    aic_ctrl_init();
    aic_net_mgr_init();
    aic_common_poweroff_init();
    aic_tele_init();
    aic_cmux_init_task();
    aic_srv_at_init();
    aic_net_init();
#if AIC_CAT1_ENABLE_WIFI_SCAN
    aic_srv_wifi_scan_init();
#endif
}

#endif /* __AIC_INIT_H__ */

