/******************************************************************************/
/*                                                                            */
/*    Copyright 2023 by AICXTEK TECHNOLOGIES CO.,LTD. All rights reserved.    */
/*                                                                            */
/******************************************************************************/

/**
 * @file aic_srv_version.h
 *
 */

#ifndef __AIC_SRV_VERSION_H__
#define __AIC_SRV_VERSION_H__

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "aic_portable.h"

/*********************
 *      DEFINES
 *********************/
#define AIC_SRV_VERSION_MAX_LEN     (40)
#define AIC_SRV_BUILD_TIME_MAX_LEN  (40)
#define AIC_SRV_VERSION_TOTAL_LEN   (5*AIC_SRV_VERSION_MAX_LEN + AIC_SRV_BUILD_TIME_MAX_LEN)

#define AIC_SRV_SW_VERSION      "v24.005"
#define AIC_SRV_SW_SUBVERSION   "00037"

#define AIC_VERSION             "v24.008"
#define AIC_SUBVERSION          "00037"

#define AIC_SRV_BUILD_DATE      "2024.XX.XX"
#define AIC_SRV_BUILD_TIME      "XX:XX:XX"

#define AIC_SRV_VERSION_FMT     "Version:" \
                                "  %s.%s_aicsrv_%s\r\n" \
                                "Time:" \
                                "  %s %s\r\n" \
                                "Aic_Version:" \
                                "  %s.%s\r\n" \
                                "318_Version:" \
                                " %s\r\n"

/**********************
 *      TYPEDEFS
 **********************/
/*
 * The sw version like "v24.001.00001"
 * The whole vesion info like below:
 *
 * Version:
 *    v24.001.00001_aicsrv_XXXX
 * Time:
 *    Oct 26 2022 08:00:45
 */
void aic_get_version(char *buf, size_t size);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /* __AIC_SRV_VERSION_H__ */
