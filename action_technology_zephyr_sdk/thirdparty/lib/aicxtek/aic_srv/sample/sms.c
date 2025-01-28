/******************************************************************************/
/*                                                                            */
/*    Copyright 2023 by AICXTEK TECHNOLOGIES CO.,LTD. All rights reserved.    */
/*                                                                            */
/******************************************************************************/

/**
 *  DESCRIPTION
 *
 *    This file is sample code of sms
 */

/*********************
 *      INCLUDES
 *********************/
#include <stdlib.h>
#include "aic_type.h"
#include "aic_srv_tele.h"

/**********************
 *   STATIC FUNCTIONS
 **********************/
static int32_t handleSmsCallback(void *p_param, uint32_t size)
{
    int32_t ret = -1;
    uint32_t event = TS_EVENT_SMS_MAX;

    if(NULL == p_param) {
        alog_error("[%s][Error]p_param is NULL.", __func__);
        return -1;
    }

    event = *(uint32_t *)p_param;
    alog_info("[%s][Info]event id is 0x%x.", __func__, event);
    switch(event) {
        case TS_EVENT_SMS_RECEIVE_NEW: /* recieve a new sms, and have NOT writen this new sms in SIM (write in SIM later) */
        {
            ts_sms_receive_new_t *p_ind = (ts_sms_receive_new_t *)p_param;

            alog_info("[%s][Info]TS_EVENT_SMS_RECEIVE_NEW, index:%d, total_count:%d, p_ind->is_full_in_sim = %d.\n", __func__, p_ind->index, p_ind->total_count, p_ind->is_full_in_sim);
            alog_info("[%s][Info]TS_EVENT_SMS_RECEIVE_NEW, sms_class(%d), ymd(%d-%d-%d), hms(%d:%d:%d), number(%s), content(%s).",
                      __func__,
                      p_ind->sms_class,
                      p_ind->time_stamp.year,
                      p_ind->time_stamp.month,
                      p_ind->time_stamp.day,
                      p_ind->time_stamp.hour,
                      p_ind->time_stamp.min,
                      p_ind->time_stamp.sec,
                      p_ind->number,
                      (char *)&p_ind->p_content);
            break;
        }

        case TS_EVENT_SMS_READ_IN_SIM_RESP:
            {
                ts_sms_read_in_sim_resp_t *p_sim = (ts_sms_read_in_sim_resp_t *)p_param;

                alog_info("[%s][Info]TS_EVENT_SMS_READ_IN_SIM_RESP, index:%d, textstart:0x%x \n", __func__, p_sim->index, (uint32_t)p_sim->p_content);
                if (p_sim->base_info.err_code == 0) {
                    alog_info("[%s][Info]index(%d), sms_class(%d), ymd(%d-%d-%d), hms(%d:%d:%d), number(%s), content(%s)",
                              __func__,
                              p_sim->index,
                              p_sim->sms_class,
                              p_sim->time_stamp.year,
                              p_sim->time_stamp.month,
                              p_sim->time_stamp.day,
                              p_sim->time_stamp.hour,
                              p_sim->time_stamp.min,
                              p_sim->time_stamp.sec,
                              p_sim->number,
                              (char *)&p_sim->p_content);
                }
            }
            break;

        case TS_EVENT_SMS_GET_SMSC_RESP:
            {
                ts_sms_get_smsc_resp_t* p_sms_smsc = (ts_sms_get_smsc_resp_t*)(p_param);

                alog_info("[%s][Info]TS_EVENT_SMS_GET_SMSC_RESP, p_sms_smsc->smsc is %s. \n", __func__, p_sms_smsc->smsc);
            }
            break;

        case TS_EVENT_SMS_SEND_RESP:
            {
                ts_sms_send_resp_t* p_sms_send = (ts_sms_send_resp_t*)(p_param);

                alog_info("[%s][Info]TS_EVENT_SMS_SEND_RESP, p_sms_send->base_info.err_code = %d.", __func__, p_sms_send->base_info.err_code);
                if (p_sms_send->base_info.err_code == 0){/* sms send successfully */
                    alog_info("[%s][Info]TS_EVENT_SMS_SEND_RESP, p_sms_send->index = %d.", __func__, p_sms_send->index);
                }
            }
            break;

        case TS_EVENT_SMS_DEL_IN_SIM_RESP:
            {
                ts_sms_del_in_sim_resp_t *p_sms_del = (ts_sms_del_in_sim_resp_t *)p_param;

                alog_info("[%s][Info]TS_EVENT_SMS_DEL_IN_SIM_RESP, p_sms_del->base_info.err_code = %d.", __func__, p_sms_del->base_info.err_code);
            }
            break;

        case TS_EVENT_SMS_LOAD_IN_SIM_RESP: {
            ts_sms_load_in_sim_resp_t *p_load_sms = (ts_sms_load_in_sim_resp_t *)p_param;
            uint16_t index = 0;

            alog_info("[%s]TS_EVENT_SMS_LOAD_IN_SIM_RESP, card_id = %d, err_code = %d, is_full_in_sim = %d, total_count = %d, count_in_sim = %d, have_reported_count = %d, cur_reported_count = %d, index_start = %d.",
                      __func__,
                      p_load_sms->base_info.card_id,
                      p_load_sms->base_info.err_code,
                      p_load_sms->is_full_in_sim,
                      p_load_sms->total_count,
                      p_load_sms->count_in_sim,
                      p_load_sms->have_reported_count,
                      p_load_sms->cur_reported_count,
                      p_load_sms->index_start);
            if (0 == p_load_sms->base_info.err_code) {
                /* load sim sms success. */
                if (true == p_load_sms->is_full_in_sim) {
                    /* If the sms in sim is full, user could do something here.(such as pop up some UI tips or delete some messages.) */
                }
                /* when user receive this response event, call this api to get sms content */
                for (index = p_load_sms->index_start; index <= p_load_sms->total_count; index++) {
                    ret = aic_srv_tele_sms_read_in_sim(p_load_sms->base_info.card_id, index, handleSmsCallback);
                    alog_info("[%s]aic_srv_tele_sms_read_in_sim[index = %d] return %d.", __func__, index, ret);
                }
            } else {
                /* load sim sms failed, and user could pop up some UI tips. */
            }
            break;
        }

        case TS_EVENT_SMS_WRITE_IN_SIM_STATUS_UPDATED: { /* sms status updated in sim after receive/send a new sms */
            ts_sms_write_in_sim_status_updated_t *p_write_to_sim = (ts_sms_write_in_sim_status_updated_t *)p_param;

            alog_info("[%s]TS_EVENT_SMS_WRITE_IN_SIM_STATUS_UPDATED, card_id = %d, index = %d, count_in_sim = %d, is_full_in_sim = %d, type = %d, write_result = %d.",
                      __func__,
                      p_write_to_sim->card_id,
                      p_write_to_sim->index,
                      p_write_to_sim->count_in_sim,
                      p_write_to_sim->is_full_in_sim,
                      p_write_to_sim->type,
                      p_write_to_sim->write_result);
            break;
        }

        default:
            break;
    }

    return 0;
}

static int32_t handleSimCallback(void *p_param, uint32_t size)
{
    ts_event_sim_t *p_sim_event = NULL;

    if(NULL == p_param) {
        alog_error("[%s][Err]p_param is null.", __func__);
        return -1;
    }

    p_sim_event = (ts_event_sim_t *)p_param;

    switch (p_sim_event->event_id) {
        case TS_EVENT_SIM_STATUS_UPDATED: {
            ts_sim_status_updated_t *p_sim_status = NULL;

            p_sim_status = (ts_sim_status_updated_t *)p_param;
            alog_info("[%s][Info]TS_EVENT_SIM_STATUS_UPDATED(sim_exist_status = %d, cur_card_id = %d).\r\n", __func__, p_sim_status->sim_exist_status, p_sim_status->cur_card_id);
            if (0 != p_sim_status->sim_exist_status) {
                /* load sim sms list */
                aic_srv_tele_sms_load_in_sim(p_sim_status->cur_card_id, handleSmsCallback);
            }
            break;
        }

        default:
            break;
    }

    return 0;
}


/**********************
 *   GLOBAL FUNCTIONS
 **********************/
void sms_send(char *p_number, char *p_text)
{
    int32_t ret = -1;
    ts_sms_send_info_t sms_send_info = {0};

    if (NULL == p_number) {
        alog_error("[%s][Error]p_number is NULL.", __func__);
        return;
    }

    if (NULL == p_text) {
        alog_error("[%s][Error]p_text is NULL.", __func__);
        return;
    }

    sms_send_info.p_number = (char *)malloc(sizeof(char)*20 + 1);
    if (NULL == sms_send_info.p_number) {
        alog_error("[%s][Error]sms_send_info.p_number malloc fail.", __func__);
        return;
    }
    memset((char *)sms_send_info.p_number, 0, sizeof(char)*20 + 1);
    memcpy((char *)sms_send_info.p_number, p_number, min(20, strlen(p_number)));
    sms_send_info.p_smsc = NULL;
    sms_send_info.p_text = (char *)malloc(sizeof(char)*100 + 1);
    if (NULL == sms_send_info.p_text) {
        free((char *)sms_send_info.p_number);
        sms_send_info.p_number = NULL;
        alog_error("[%s][Error]sms_send_info.p_text malloc fail.", __func__);
        return;
    }
    memset((char *)sms_send_info.p_text, 0, sizeof(char)*100 + 1);
    memcpy((char *)sms_send_info.p_text, p_text, min(100, strlen(p_text)));
    alog_info("[%s][Info]number: %s, text: %s.", __func__, sms_send_info.p_number, sms_send_info.p_text);
    ret = aic_srv_tele_sms_send(0, &sms_send_info, handleSmsCallback);
    alog_info("[%s][Info]aic_srv_tele_sms_send return %d.", __func__, ret);

    if (NULL != sms_send_info.p_number) {
        free((char *)sms_send_info.p_number);
        sms_send_info.p_number = NULL;
    }
    if (NULL != sms_send_info.p_text) {
        free((char *)sms_send_info.p_text);
        sms_send_info.p_text = NULL;
    }
}

void sms_init( void )
{
    int32_t sms_handle = 0;
    int32_t sim_handle = 0;
    aic_srv_bus_info_t bus_info = {0};

    /* register sms callback, this callback receive global events(eg. sms new ind, status report, etc.)
       and response events(eg. send response, get smsc response, etc.)*/
    bus_info.p_client_name = "sms_app";
    sms_handle = aic_srv_tele_sms_register(&bus_info, handleSmsCallback);
    alog_info("[%s][Info]aic_srv_tele_sms_register return handle 0x%x.", __func__, sms_handle);

    /* register sim callback, when sim ready, call aic_srv_tele_sms_load_in_sim to load sim sms. */
    sim_handle = aic_srv_tele_sim_register(&bus_info, handleSimCallback);
}
