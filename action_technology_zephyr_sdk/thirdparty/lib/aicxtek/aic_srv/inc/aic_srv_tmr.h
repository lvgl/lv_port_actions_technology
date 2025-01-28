/******************************************************************************/
/*                                                                            */
/*    Copyright 2023 by AICXTEK TECHNOLOGIES CO.,LTD. All rights reserved.    */
/*                                                                            */
/******************************************************************************/

/**
 * @file aic_srv_tmr.h
 *      "tmr" is an abbreviation for "timer"
 *
 */

#ifndef __AIC_SRV_TMR_H__
#define __AIC_SRV_TMR_H__

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "aic_srv_bus.h"

/*********************
 *      DEFINES
 *********************/


/**********************
 *      TYPEDEFS
 **********************/
typedef void *srv_tmr_handle;

typedef struct {
    uint32_t    time_out;       /* timer expiration */
    bool        is_repeat;      /* is repeat */
    char        *p_name;        /* timer name */
    void        *p_usr_data;    /* user data */
}srv_tmr_param_t;

typedef struct {
    srv_tmr_handle  tmr_handle;     /* the handler of timer which is return value when create */
    void            *p_usr_data;    /* the user data which is input value when create */
}srv_tmr_cb_param_t;

/************************
 * GLOBAL FUNCTIONS
 **********************/

/**
* aic_srv_tmr_create.
* @arg1: p_tmr_param:timer params.
* @arg2: _tmr_cb:timer callback
* Return: timer handle.
*/
srv_tmr_handle aic_srv_tmr_create(srv_tmr_param_t *p_tmr_param, srv_func_callback _tmr_cb);

/**
* aic_srv_tmr_destroy
* @arg1: tmr_handle:timer handle.
* Return: -1,fail;0,success.
*/
int32_t aic_srv_tmr_destroy(srv_tmr_handle tmr_handle);

/**
* aic_srv_tmr_pause
* @arg1: tmr_handle:timer handle.
* Return: -1,fail;0,success.
*/
int32_t aic_srv_tmr_pause(srv_tmr_handle tmr_handle);

/**
* aic_srv_tmr_resume
* @arg1: tmr_handle:timer handle.
* Return: -1,fail;0,success.
*/
int32_t aic_srv_tmr_resume(srv_tmr_handle tmr_handle);

/**
* aic_srv_tmr_change
* @arg1: tmr_handle:timer handle.
* @arg2: p_tmr_param:to change timer param
* Return: -1,fail;0,success.
*/
int32_t aic_srv_tmr_change(srv_tmr_handle tmr_handle, srv_tmr_param_t *p_tmr_param);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /* __AIC_SRV_TMR_H__ */

