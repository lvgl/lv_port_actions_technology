/*****************************************************************************/
/*                                                                           */
/*    Copyright 2024 by AICXTEK TECHNOLOGIES CO.,LTD. All rights reserved.   */
/*                                                                           */
/*****************************************************************************/

/**
 * @file aic_srv_cfg.h
 *
 */

#ifndef __AIC_SRV_CFG_H_
#define __AIC_SRV_CFG_H_

#ifdef __cplusplus
extern "C" {
#endif
/*********************
 *      INCLUDES
 *********************/
#include <stdint.h>
#include <stdbool.h>

/******************************************************************************/
/*  Currently supports three LTE network solutions, they are mutually         */
/*  exclusive.                                                                */
/******************************************************************************/
/* LWIP network scheme */
#define AIC_CAT1_ENABLE_LWIP_NETWORK_SOLUTION   0

/* AT NETWORK scheme */
#define AIC_CAT1_ENABLE_AT_NETWORK_SOLUTION     0

/* AT SOCKET scheme */
#define AIC_CAT1_ENABLE_AT_SOCKET_SOLUTION      1

/* WIFI SCAN ENABLE */
#define AIC_CAT1_ENABLE_WIFI_SCAN               0

#ifdef __cplusplus
}
#endif

#endif /* __AIC_SRV_CFG_H__ */

