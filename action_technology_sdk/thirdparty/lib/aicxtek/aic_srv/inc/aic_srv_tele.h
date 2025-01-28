/******************************************************************************/
/*                                                                            */
/*    Copyright 2023 by AICXTEK TECHNOLOGIES CO.,LTD. All rights reserved.    */
/*                                                                            */
/******************************************************************************/

/**
 * @file aic_srv_tele.h
 *
 */

#ifndef __AIC_SRV_TELE_H__
#define __AIC_SRV_TELE_H__

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "aic_srv_tele_radio.h"
#include "aic_srv_tele_sim.h"

#ifdef CONFIG_TELE_CALL
#include "aic_srv_tele_call.h"
#endif

#ifdef CONFIG_TELE_DATA
#include "aic_srv_tele_data.h"
#endif

#ifdef CONFIG_TELE_SMS
#include "aic_srv_tele_sms.h"
#endif

#ifdef CONFIG_TELE_PB
#include "aic_srv_tele_pb.h"
#endif

/*********************
 *      DEFINES
 *********************/
/* Error code in Service layer */
#define TS_SUCCESSFULL           (0)
#define TS_ERR_PARAMETER_INVALID (-1001)
#define TS_ERR_CARD_INVALID      (-1002)
#define TS_ERR_IN_PROGRESS       (-1003)
#define TS_ERR_NO_MEMORY         (-1004)
#define TS_ERR_CARD_STATUS       (-1005)
#define TS_ERR_PARSE_FAIL        (-1006)
#define TS_ERR_NOT_OBTAINED      (-1007)

#ifndef min
#define min(a, b)  ((a) < (b) ? (a) : (b))
#endif

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/
/**
* aic_srv_tele_init
* Return: if success return 0, else return other value.
*/
int32_t aic_srv_tele_init(void);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /* __AIC_SRV_TELE_H__ */
