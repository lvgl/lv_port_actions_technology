/******************************************************************************/
/*                                                                            */
/*    Copyright 2023 by AICXTEK TECHNOLOGIES CO.,LTD. All rights reserved.    */
/*                                                                            */
/******************************************************************************/

/**
 * @file aic_srv_tmr.c
 *
 */


/*********************
 *      INCLUDES
 *********************/
#include <kernel.h>
#include "aic_portable.h"
#include "aic_srv_tmr.h"

/*********************
 *      DEFINES
 *********************/
#define dbg_print   alog_info

/**********************
 *      TYPEDEFS
 **********************/
typedef struct {
    srv_tmr_handle      tmr_handle;         /* timer handle */
    struct k_timer      sys_tmr;            /* timer id */
    struct k_work       task_work;
    srv_func_callback   client_timer_cb;    /* user timer callback */
    bool                is_period;          /* user timer is period */
    void                *p_user_data;       /* user data */
    uint32_t            time_out;
} timer_record_t;

/**********************
 *  STATIC PROTOTYPES
 **********************/


/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *   STATIC FUNCTIONS
 **********************/
static void task_work_handle(struct k_work *work)
{
    timer_record_t *p_timer_record;
    srv_tmr_cb_param_t tmr_cb_param = {0};
    uint32_t data_size;

    p_timer_record = CONTAINER_OF(work, timer_record_t, task_work);
    if(NULL != p_timer_record->client_timer_cb) {
        tmr_cb_param.tmr_handle = p_timer_record->tmr_handle;
        tmr_cb_param.p_usr_data = p_timer_record->p_user_data;
        data_size = sizeof(srv_tmr_cb_param_t);
        p_timer_record->client_timer_cb(&tmr_cb_param, data_size);
    } else {
        dbg_print("[%s],client_timer_cb is null\n", __func__);
    }
}

static void timer_timeout_func(struct k_timer *timer)
{
    timer_record_t *p_timer_record =
                                CONTAINER_OF(timer, timer_record_t, sys_tmr);

    if(NULL != p_timer_record->client_timer_cb) {
        k_work_submit(&p_timer_record->task_work);
    } else {
        dbg_print("[%s],client_timer_cb is null\n", __func__);
    }
}

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
srv_tmr_handle aic_srv_tmr_create(srv_tmr_param_t *p_tmr_param, srv_func_callback _tmr_cb)
{
    timer_record_t *p_timer_record = NULL;
    struct k_timer *tmr = NULL;

    /* 1. Judge validity of Input param */
    if(NULL == p_tmr_param) {
        dbg_print("[%s]:p_tmr_param is null\n", __func__);
        return NULL;
    }

    /* 2. malloc space for record */
    p_timer_record = (timer_record_t *)aos_malloc(sizeof(timer_record_t));

    if(NULL == p_timer_record) {
        dbg_print("[%s],p_timer_record malloc fail\n", __func__);
        return NULL;
    }

    p_timer_record->tmr_handle      = p_timer_record;
    p_timer_record->p_user_data     = p_tmr_param->p_usr_data;
    p_timer_record->client_timer_cb = _tmr_cb;
    p_timer_record->time_out     = p_tmr_param->time_out;
    p_timer_record->is_period     = p_tmr_param->is_repeat;
    k_work_init(&p_timer_record->task_work, task_work_handle);

    /* 3. Create timer*/
    tmr = &p_timer_record->sys_tmr;
    k_timer_init(tmr, timer_timeout_func, NULL);

    /* 4. Save timer handle & start timer  */
    if(p_timer_record->is_period)
        k_timer_start(tmr,  K_MSEC(p_timer_record->time_out), K_MSEC(p_timer_record->time_out));
    else
        k_timer_start(tmr,  K_MSEC(p_timer_record->time_out ), K_NO_WAIT);

    return (srv_tmr_handle)p_timer_record;
}

int32_t aic_srv_tmr_destroy(srv_tmr_handle tmr_handle)
{
    int ret = 0;
    timer_record_t *p_timer_record = NULL;
    struct k_timer *tmr = NULL;

    if(NULL == tmr_handle) {
        dbg_print("[%s],srv_tmr_ptr timer handle is NULL,error\n", __func__);
        ret = -1;
        return ret;
    }
    p_timer_record = (timer_record_t *)tmr_handle;
    tmr = &p_timer_record->sys_tmr;
    dbg_print("[%s],timer handle = %p\n", __func__, p_timer_record);

    /* 1. delete srv timer id */
    k_timer_stop(tmr);
    /* 2. free srv timer id */
    aos_free(p_timer_record);
    p_timer_record = NULL;
    return ret;
}

int32_t aic_srv_tmr_pause(srv_tmr_handle tmr_handle)
{
    int ret = 0;
    timer_record_t *p_timer_record = NULL;
    struct k_timer *tmr = NULL;
    if(NULL == tmr_handle) {
        dbg_print("[%s],srv_tmr_ptr timer handle is NULL,error\n", __func__);
        ret = -1;
        return ret;
    }
    p_timer_record = (timer_record_t *)tmr_handle;
    tmr = &p_timer_record->sys_tmr;
    k_timer_stop(tmr);
    dbg_print("[%s],timer pause success.\n", __func__);
    return ret;
}

int32_t aic_srv_tmr_resume(srv_tmr_handle tmr_handle)
{
    int  ret = 0;
    timer_record_t *p_timer_record = NULL;
    struct k_timer *tmr = NULL;
    if(NULL == tmr_handle) {
        dbg_print("[%s],timer handle is NULL,error\n", __func__);
        ret = -1;
        return ret;
    }

    p_timer_record = (timer_record_t *)tmr_handle;
    dbg_print("[%s],timer handle = %p\n", __func__, p_timer_record);
    tmr = &p_timer_record->sys_tmr;
    if(p_timer_record->is_period)
        k_timer_start(tmr,  K_MSEC(p_timer_record->time_out), K_MSEC(p_timer_record->time_out));
    else
        k_timer_start(tmr,  K_MSEC(p_timer_record->time_out ), K_NO_WAIT);
    dbg_print("[%s],timer resume success.\n", __func__);
    return ret;
}

int32_t aic_srv_tmr_change(srv_tmr_handle tmr_handle, srv_tmr_param_t *p_tmr_param)
{
    int ret = 0;
    timer_record_t *p_timer_record = NULL;
    struct k_timer *tmr  = NULL;

    if(NULL == tmr_handle) {
        dbg_print("[%s],timer handle is NULL,error\n", __func__);
        ret = -1;
        return ret;
    }

    if(NULL == p_tmr_param) {
        dbg_print("[%s],p_tmr_param NULL,error\n", __func__);
        ret = -1;
        return ret;
    }

    p_timer_record = (timer_record_t *)tmr_handle;
    tmr = &p_timer_record->sys_tmr;
    p_timer_record->p_user_data  = p_tmr_param->p_usr_data;
    p_timer_record->time_out     = p_tmr_param->time_out;
    p_timer_record->is_period    = p_tmr_param->is_repeat;

    /* 1. change this timer */
    k_timer_stop(tmr);

    dbg_print("[%s],timer change,timer handle = %p, is_repeat = %d, timeout = %d\n", __func__, p_timer_record, p_tmr_param->is_repeat, p_tmr_param->time_out);

    if(p_timer_record->is_period)
        k_timer_start(tmr,  K_MSEC(p_timer_record->time_out), K_MSEC(p_timer_record->time_out));
    else
        k_timer_start(tmr,  K_MSEC(p_timer_record->time_out ), K_NO_WAIT);
    dbg_print("[%s],timer change success.\n", __func__);
    return ret;
}
