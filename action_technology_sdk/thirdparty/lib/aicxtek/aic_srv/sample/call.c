/******************************************************************************/
/*                                                                            */
/*    Copyright 2023 by AICXTEK TECHNOLOGIES CO.,LTD. All rights reserved.    */
/*                                                                            */
/******************************************************************************/

/**
 *  DESCRIPTION
 *
 *    This file is enter of call
 */

/*********************
 *      INCLUDES
 *********************/
#include "aic_type.h"
#include "aic_srv_tele.h"

/**********************
 *  STATIC VARIABLES
 **********************/
static ts_call_info_list_t s_call_info_list = {0};

/**********************
 *   STATIC FUNCTIONS
 **********************/

static int32_t handleReleaseResp(void *p_param, uint32_t size)
{
    ts_call_release_resp_t *p_release_resp = NULL;

    if(NULL == p_param) {
        alog_error("handleReleaseResp: param is null");
        return -1;
    }
    p_release_resp = (ts_call_release_resp_t *)(p_param);
    if(0 != p_release_resp->ret_value) {
        /* release fail:
           popup prompt message */

    } else {
        /* release success:
           to ui task:to display disconnected win */
    }
    return 0;
}

static int32_t handleAnswerResp(void *p_param, uint32_t size)
{
    ts_call_answer_resp_t *p_answer_resp = NULL;

    if(NULL == p_param) {
        alog_error("handleAnswerResp: param is null");
        return -1;
    }
    p_answer_resp = (ts_call_answer_resp_t *)(p_param);
    if(0 != p_answer_resp->ret_value) {
        /* anwser fail:
           to ui task:popup prompt message "auwser fail" */
    } else {
       /* anwser success:
          to ui task:to display connected call win */
    }
    return 0;
}

static int32_t handleRejectResp(void *p_param, uint32_t size)
{
    ts_call_reject_resp_t *p_reject_resp = NULL;

    if(NULL == p_param) {
        alog_error("handleRejectResp: param is null");
        return -1;
    }
    p_reject_resp = (ts_call_reject_resp_t *)(p_param);
    if(0 != p_reject_resp->ret_value) {
        /* Reject fail:
           to ui task:popup prompt message */
    } else {
        /* Reject fail:
           to ui task::close incoming call win */
    }
    return 0;
}

static int32_t handleDialResp(void *p_param, uint32_t size)
{
    ts_call_dial_resp_t *p_dial_resp = NULL;

    if(NULL == p_param) {
        alog_error("handleDialResp: param is null");
        return -1;

    }
    p_dial_resp = (ts_call_dial_resp_t *)(p_param);
    alog_info("[%s], dial resp val = %d,", __func__, p_dial_resp->ret_value);
    if(0 != p_dial_resp->ret_value) {
        /* dial fail:
           to ui task:popup prompt message */
        return -1;

    } else {
        /* dial success:
           to ui task:enter outgoing call win */
    }
    return 0;
}

static int32_t handleCallCallback(void *p_param, uint32_t size)
{
    ts_event_call_t *p_call_event = NULL;
    int ret_audio = 0;

    if(NULL == p_param) {
        alog_error("handleCallCallback,p_param is null");
        return -1;
    }

    p_call_event = (ts_event_call_t *)p_param;

    switch (p_call_event->event_id) {
        case TS_EVENT_CALL_STATUS_UPDATED: {
            ts_call_status_updated_t *p_call_status = {0};
            ts_call_info_list_t call_info_list = {0};

            p_call_status = (ts_call_status_updated_t *)p_param;
            //alog_info("handleCallCallback,p_call_status status = %d",  p_call_status->call_status);
            aic_srv_tele_call_get_call_info_list(0, &call_info_list);
            memcpy(&s_call_info_list, &call_info_list, sizeof(ts_call_info_list_t));

            switch(p_call_status->call_status) {
                case TS_CALL_STATUS_ACTIVE:/* remote accept call,setup call success,display connected call win(include,phone number and call duration) */
                    /* 1.stop play local ring */

                    /* 2.stop play local ringback */

                    /* 3.start voice */

                    /* 4.enter connected win and display call time*/
                    break;

                case TS_CALL_STATUS_INCOMING: {/* incoming a call,phone recieve a call */
                    alog_info("handleCallCallback,call_active: = %4 call_number = %4", call_info_list.call_number);
                    if (1 < call_info_list.call_number) {
                        int ret = -1;
                        uint8_t cur_card_id = TS_CARD_ID_MAX;
                        aic_srv_tele_sim_get_current_card(&cur_card_id);
                        ret = aic_srv_tele_call_release(cur_card_id, p_call_status->call_idx, NULL);
                        alog_info("handleCallCallback,aic_srv_tele_call_release return,ret = %d.\r\n", ret);
                        return -1;
                    } else {
                        /* play local ring */
                    }

                    /* 2. to ui task,display incoming call win */
                    break;
                }

                case TS_CALL_STATUS_RELEASED: {/*hung up call,user hungup and remote hungup,this message is coming */
                    /* if there is other call index, do not need to stop voice */
                    alog_info("handleCallCallback,TS_CALL_STATUS_RELEASED call_number = %d.\r\n", call_info_list.call_number);
                    if (0 == call_info_list.call_number) {
                        /* 1. stop voice */

                    } else {
                        alog_info("handleCallCallback,Release the second call, do nothing and return only.\r\n");
                        return -1;
                    }

                    /* 2.release call，
                       ui task:close call win */
                    break;
                }

                default:
                    break;
            }
            break;
        }
        case TS_EVENT_CALL_VOLTE_REG_STATUS_UPDATED:/* ims register state*/
        {
            ts_call_volte_reg_status_updated_t *p_volte_status = NULL;
            alog_info("handleCallCallback,TS_EVENT_CALL_VOLTE_REG_STATUS_UPDATED");
            p_volte_status = (ts_call_volte_reg_status_updated_t *)p_param;

            alog_info("handleCallCallback,volte status = %d", p_volte_status->volte_reg_status);
            if(true == p_volte_status->volte_reg_status) {
                /* 1.ims register success
                   UI task:refresh volte icon. */

            } else {
                /* 1.ims register fail
                   UI task:refresh volte icon. */

            }
            break;
        }

        case TS_EVENT_CALL_RINGBACK_VOICE: {/* play tone,maybe customed ringback by operator from network,maybe play local ringback settinged by user*/
            ts_call_ringback_voice_t *p_ringback_voice = NULL;

            p_ringback_voice = (ts_call_ringback_voice_t *)p_param;
            /* voice_type: 2-network ringback voice, 0-local ringback voice */
            /* voice_state: 1-start play, 0-stop play */
            alog_info("handleCallCallback,TS_EVENT_CALL_RINGBACK_VOICE arrived(voice_type = %d, voice_state = %d).\r\n",
                     p_ringback_voice->voice_type,
                     p_ringback_voice->voice_state);

            if (1 == p_ringback_voice->voice_state) {/* if voice_state is 1, start play ringback voice */
                /* TODO: open audio codec and start transform audio data to play ringback voice */

            } else {/* if voice_state is 0, no need to do things currently */
                alog_info("handleCallCallback,currently no need to do things for stop play ringback voice.\r\n");
            }
            break;
        }

        case TS_EVENT_CALL_SEND_DTMF_RESP: {
            ts_call_send_dtmf_resp_t *p_send_dtmf_resp = NULL;

            p_send_dtmf_resp = (ts_call_send_dtmf_resp_t *)p_param;
            alog_info("[%s][Info]p_send_dtmf_resp->ret_value = %d.", __func__, p_send_dtmf_resp->ret_value);
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
void call_send_dtmf_single_char(char dtmf_char)
{
    int32_t ret = -1;
    ts_call_send_dtmf_info_t send_dtmf_info = {0};
    uint32_t dtmf_len = 1;
    char dtmf[2] = {0};

    /* send single char */
    alog_info("[%s][Info]dtmf_char: %c, dtmf_len: %d.", __func__, dtmf_char, dtmf_len);
    sprintf(dtmf, "%c", dtmf_char);
    send_dtmf_info.p_dtmf_str = dtmf;
    send_dtmf_info.str_length = dtmf_len;
    send_dtmf_info.duration = 300;
    alog_info("[%s][Info]p_dtmf_str: %s, str_length: %d.", __func__, send_dtmf_info.p_dtmf_str, send_dtmf_info.str_length);
    ret = aic_srv_tele_call_send_dtmf(0, &send_dtmf_info, NULL);
}

void call_send_dtmf_string(char *p_dtmf_str, uint32_t dtmf_str_len)
{
    int32_t ret = -1;
    ts_call_send_dtmf_info_t send_dtmf_info = {0};

    /* send single char */
    send_dtmf_info.p_dtmf_str = p_dtmf_str;
    send_dtmf_info.str_length = dtmf_str_len;
    send_dtmf_info.duration = 300;
    ret = aic_srv_tele_call_send_dtmf(0, &send_dtmf_info, NULL);
}

/* power on and call init */
void app_call_init(void)
{
    int32_t call_handle = 0;

    /* register service callback,receive call message(eg.incoming call) */
    call_handle = aic_srv_tele_call_register(NULL, handleCallCallback);
}

