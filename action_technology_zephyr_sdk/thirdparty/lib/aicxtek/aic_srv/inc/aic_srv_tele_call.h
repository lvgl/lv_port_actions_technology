/******************************************************************************/
/*                                                                            */
/*    Copyright 2023 by AICXTEK TECHNOLOGIES CO.,LTD. All rights reserved.    */
/*                                                                            */
/******************************************************************************/

/**
 * @file aic_srv_tele_call.h
 *
 */

#ifndef __AIC_SRV_TELE_CALL_H__
#define __AIC_SRV_TELE_CALL_H__

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
/* 电话号码的最大长度 */
#define TS_CALL_PHONE_NUMBER_LEN_MAX (32)

/* the call max number */
#define TS_CALL_NUMBER_MAX                (7)

#define TS_EVENT_CALL_STRUCT_HEAD ts_event_call_e event_id;

/**********************
 *      TYPEDEFS
 **********************/
typedef enum
{
    /* call状态发生变化的事件，携带ts_call_status_updated_t数据信息, 0x1代表call service模块 */
    TS_EVENT_CALL_STATUS_UPDATED = 0x1 << 16,

    /* the response of call aic_srv_tele_call_dial api, with ts_call_dial_resp_t data */
    TS_EVENT_CALL_DIAL_RESP,

    /* the response of call aic_srv_tele_call_answer api, with ts_call_answer_resp_t data */
    TS_EVENT_CALL_ANSWER_RESP,

    /* the response of call aic_srv_tele_call_release api, with ts_call_release_resp_t data */
    TS_EVENT_CALL_RELEASE_RESP,

    /* the response of call aic_srv_tele_call_reject api, with ts_call_reject_resp_t data */
    TS_EVENT_CALL_REJECT_RESP,

    /* the VoLTE reg status updated event, with ts_call_volte_reg_status_updated_t data */
    TS_EVENT_CALL_VOLTE_REG_STATUS_UPDATED,

    /* the ringback voice event, with ts_call_ringback_voice_t data */
    TS_EVENT_CALL_RINGBACK_VOICE,

    /* the send dtmf response event, with ts_call_send_dtmf_resp_t data */
    TS_EVENT_CALL_SEND_DTMF_RESP,

    /* the dtmf key event, with ts_call_receive_dtmf_t data */
    TS_EVENT_CALL_RECEIVE_DTMF,

    /* the VoLTE enable event, with ts_call_volte_enable_resp_t data */
    TS_EVENT_CALL_VOLTE_ENABLE_RESP,

    TS_EVENT_CALL_MAX
}ts_event_call_e;

typedef struct
{
    ts_event_call_e event_id;
}ts_event_call_t;

/* tele call的状态枚举值 */
typedef enum
{
    /* 表示没有通话 */
    TS_CALL_STATUS_IDLE = 0,

    /* 表示有正常进行中的通话 */
    TS_CALL_STATUS_ACTIVE,

    /* 表示有保持的通话 */
    TS_CALL_STATUS_HELD,

    /* 表示有正在拨出的通话 */
    TS_CALL_STATUS_DIALING,

    /* 表示呼出的电话正在响铃 */
    TS_CALL_STATUS_ALERTING,

    /* 表示有呼入的通话 */
    TS_CALL_STATUS_INCOMING,

    /* 表示多通电话有来电正在等待接听 */
    TS_CALL_STATUS_WAITING,

    /* 表示通话挂断 */
    TS_CALL_STATUS_RELEASED,

    TS_CALL_STATUS_MAX
}ts_call_status_e;

typedef enum
{
    /* release during call active */
    TS_CALL_RELEASE_REASON_NORMAL = 1,

    /* release by remote no answer */
    TS_CALL_RELEASE_REASON_REMOTE_NO_ANSWER,

    /* release by DUT reject during MT */
    TS_CALL_RELEASE_REASON_DUT_REJECT,

    /* release by remote cancel during MT alerting */
    TS_CALL_RELEASE_REASON_REMOTE_CANCEL,

    /* release by network abnormal */
    TS_CALL_RELEASE_REASON_NETWORK_ABNORMAL,

    /* release by other reasons */
    TS_CALL_RELEASE_REASON_OTHER,

    TS_CALL_RELEASE_REASON_MAX
}ts_call_release_reason_e;

/* app拨打电话时传入的数据结构体信息 */
typedef struct
{
    /* sim卡的id，0-卡槽1，1-卡槽2，目前仅支持单卡（0） */
    uint8_t card_id;

    /* 拨打的电话号码 */
    char phone_number[TS_CALL_PHONE_NUMBER_LEN_MAX + 1];

    /* 主叫线路识别限制 */
    int32_t clir;
}ts_call_dial_info_t;

/* TS_EVENT_CALL_STATUS_UPDATED事件携带的数据结构体信息 */
typedef struct
{
    ts_event_call_e event_id;

    uint8_t card_id;

    /* 当前通话的index值 */
    uint8_t call_idx;

    /* 对端的电话号码 */
    char phone_number[TS_CALL_PHONE_NUMBER_LEN_MAX + 1];

    /* 电话的direction：0-MO，1-MT */
    uint8_t direction;

    /* 当前通话状态 */
    ts_call_status_e call_status;

    /* relase reason value */
    ts_call_release_reason_e reason_val;
}ts_call_status_updated_t;

/* the call information for single call */
typedef struct
{
    uint8_t card_id;

    /* The call index */
    uint8_t call_idx;

    /* The telephone number */
    char phone_number[TS_CALL_PHONE_NUMBER_LEN_MAX + 1];

    /* The call direction：0-MO, 1-MT */
    uint8_t direction;

    /* The call status */
    ts_call_status_e call_status;
}ts_call_info_t;

typedef struct
{
    /* The number of call */
    uint32_t call_number;

    /* The call information */
    ts_call_info_t call_info[TS_CALL_NUMBER_MAX];
}ts_call_info_list_t;

/* the struct relative with TS_EVENT_CALL_DIAL_RESP */
typedef struct
{
    TS_EVENT_CALL_STRUCT_HEAD

    /* Currently only 0 and -1, 0:dial success, -1:dial fail. */
    int32_t ret_value;
}ts_call_dial_resp_t;

/* the struct relative with TS_EVENT_CALL_ANSWER_RESP */
typedef struct
{
    TS_EVENT_CALL_STRUCT_HEAD

    /* Currently only 0 and -1, 0:answer success, -1:answer fail. */
    int32_t ret_value;
}ts_call_answer_resp_t;

/* the struct relative with TS_EVENT_CALL_RELEASE_RESP */
typedef struct
{
    TS_EVENT_CALL_STRUCT_HEAD

    /* Currently only 0 and -1, 0:release success, -1:release fail. */
    int32_t ret_value;
}ts_call_release_resp_t;

/* the struct relative with TS_EVENT_CALL_REJECT_RESP */
typedef struct
{
    TS_EVENT_CALL_STRUCT_HEAD

    /* Currently only 0 and -1, 0:reject success, -1:reject fail. */
    int32_t ret_value;
}ts_call_reject_resp_t;

typedef struct
{
    ts_event_call_e event_id;
    uint8_t card_id;
    bool volte_reg_status;
}ts_call_volte_reg_status_updated_t;

typedef struct
{
    ts_event_call_e event_id;

    uint8_t card_id;

    /* 0: local alerting; 1: network alerting */
    int32_t voice_type;

    /* 0: stop play, 1: start play */
    int32_t voice_state;
}ts_call_ringback_voice_t;

typedef struct
{
    char *p_dtmf_str;
    uint8_t str_length;
    int32_t duration;
}ts_call_send_dtmf_info_t;

/* the struct relative with TS_EVENT_CALL_SEND_DTMF_RESP */
typedef struct
{
    TS_EVENT_CALL_STRUCT_HEAD

    /* Currently only 0 and -1, 0:send dtmf success, -1:send dtmf fail. */
    int32_t ret_value;
}ts_call_send_dtmf_resp_t;

typedef struct
{
    ts_event_call_e event_id;
    uint8_t card_id;
    char dtmf_key;
}ts_call_receive_dtmf_t;

typedef struct
{
    ts_event_call_e event_id;
    uint8_t card_id;
    /* if success return 0, else return other value. */
    int32_t ret_value;
}ts_call_volte_enable_resp_t;

/**********************
 * GLOBAL PROTOTYPES
 **********************/
/**
* aic_srv_tele_call_register - tele call service的注册接口，注册的callback用于接收service层上报的事件.
* @p_bus_info: [IN] 需要client传给service层当前client的名称以及client需要关注的信号相关数据信息.
* @_srv_callback: [IN] client传给service层的回调函数.
*                 该回调函数中的参数p_param上报数据结构体指针, 参数size表示数据的大小（包含event id）
*                 目前可上报的事件有：
*                 1. TS_EVENT_CALL_STATUS_UPDATED（call状态发生变化的事件），携带ts_call_status_updated_t数据信息；
*                    其中，p_param可强转为ts_event_call_t指针，对应的事件id是TS_EVENT_CALL_STATUS_UPDATED；
*                          size值为sizeof(ts_call_status_updated_t)；
*                 2. TS_EVENT_CALL_VOLTE_REG_STATUS_UPDATED(the VoLTE reg status updated event, with ts_call_volte_reg_status_updated_t data)
*                    p_param can be transferred to ts_event_call_t pointer，and the relative event id is TS_EVENT_CALL_VOLTE_REG_STATUS_UPDATED
*                    size is sizeof(ts_call_volte_reg_status_updated_t)；
*                 3. TS_EVENT_CALL_RINGBACK_VOICE(the ringback voice event, with ts_call_ringback_voice_t data)
*                    p_param can be transferred to ts_event_call_t pointer，and the relative event id is TS_EVENT_CALL_RINGBACK_VOICE
*                    size is sizeof(ts_call_ringback_voice_t)；
*                 4. TS_EVENT_CALL_DIAL_RESP(the event of dial api response, with ts_call_dial_resp_t data(the return value of dial))
*                 5. TS_EVENT_CALL_ANSWER_RESP(the event of answer api response, with ts_call_answer_resp_t data(the return value of answer))
*                 6. TS_EVENT_CALL_RELEASE_RESP(the event of release api response, with ts_call_release_resp_t data(the return value of release))
*                 7. TS_EVENT_CALL_REJECT_RESP(the event of reject api response, with ts_call_reject_resp_t data(the return value of reject))
*                 8. TS_EVENT_CALL_RECEIVE_DTMF(the event of receive dtmf, with ts_call_receive_dtmf_t data)
*                 9. TS_EVENT_CALL_SEND_DTMF_RESP(the event of send dtmf api response, with ts_call_send_dtmf_resp_t data)
*                 10. TS_EVENT_CALL_VOLTE_ENABLE_RESP(the event of VoLTE enable api response, with ts_call_volte_enable_resp_t data)
*                 TODO: 后续在这里补充callback中返回event id的列表.
* Return: 返回给client对应的client handle，其中-1代表出错，已有client注册.
*/
int32_t aic_srv_tele_call_register(aic_srv_bus_info_t *p_bus_info, srv_func_callback _srv_callback);

/**
* aic_srv_tele_call_unregister - tele call service的去注册接口.
* @client_handle: [IN] 调用aic_srv_tele_call_register接口时返回给client的client handle值.
* Return: 成功返回0，失败返回其它值.
*/
int32_t aic_srv_tele_call_unregister(int32_t client_handle);

/**
* aic_srv_tele_call_dial - 拨打电话接口.
* @p_dial_info: [IN] 拨打电话的信息.
* @_response_callback: [IN] client调用该接口时传入的响应回调函数，在at命令的response结束后被调用
*                      The response event id is TS_EVENT_CALL_DIAL_RESP, with ts_call_dial_resp_t struct.
*                      p_param can be transferred to ts_event_call_t pointer，and the relative event id is TS_EVENT_CALL_DIAL_RESP.
*                      size is sizeof(ts_call_dial_resp_t)
* Return: 成功返回0，失败返回其它值.
*/
int32_t aic_srv_tele_call_dial(ts_call_dial_info_t *p_dial_info, srv_func_callback _response_callback);

/**
* aic_srv_tele_call_answer - 接听来电接口.
* @_response_callback: [IN] client调用该接口时传入的响应回调函数，在at命令的response结束后被调用
*                      The response event id is TS_EVENT_CALL_ANSWER_RESP, with ts_call_answer_resp_t struct.
*                      p_param can be transferred to ts_event_call_t pointer，and the relative event id is TS_EVENT_CALL_ANSWER_RESP.
*                      size is sizeof(ts_call_answer_resp_t)
* Return: 成功返回0，失败返回其它值.
*/
int32_t aic_srv_tele_call_answer(srv_func_callback _response_callback);

/**
* aic_srv_tele_call_release - 挂断call_idx对应的电话接口.
* @card_id: [IN] sim id, 0-sim1, 1-sim2.
* @call_idx: [IN] 要挂断的电话index值.
* @_response_callback: [IN] client调用该接口时传入的响应回调函数，在at命令的response结束后被调用.
*                      The response event id is TS_EVENT_CALL_RELEASE_RESP, with ts_call_release_resp_t struct.
*                      p_param can be transferred to ts_event_call_t pointer，and the relative event id is TS_EVENT_CALL_RELEASE_RESP.
*                      size is sizeof(ts_call_release_resp_t)
* Return: 成功返回0，失败返回其它值.
*/
int32_t aic_srv_tele_call_release(uint8_t card_id, uint8_t call_idx, srv_func_callback _response_callback);

/**
* aic_srv_tele_call_reject - 拒接来电的接口.
* @_response_callback: [IN] client调用该接口时传入的响应回调函数，在at命令的response结束后被调用.
*                      The response event id is TS_EVENT_CALL_REJECT_RESP, with ts_call_reject_resp_t struct.
*                      p_param can be transferred to ts_event_call_t pointer，and the relative event id is TS_EVENT_CALL_REJECT_RESP.
*                      size is sizeof(ts_call_reject_resp_t)
* Return: 成功返回0，失败返回其它值.
*/
int32_t aic_srv_tele_call_reject(srv_func_callback _response_callback);

/**
* aic_srv_tele_call_volte_enable - the open/close VoLTE api.
* @card_id: [IN] sim id, 0-sim1, 1-sim2.
* @enable: [IN] true-open VoLTE, false-close VoLTE
* @_response_callback: [IN] the response callback when client call this api, and it will be called after at response return.
*                      The response event id is TS_EVENT_CALL_VOLTE_ENABLE_RESP, with ts_call_volte_enable_resp_t struct.
*                      p_param can be transferred to ts_event_call_t pointer，and the relative event id is TS_EVENT_CALL_VOLTE_ENABLE_RESP.
*                      size is sizeof(ts_call_volte_enable_resp_t)
* Return: if success return 0, else return other value.
*/
int32_t aic_srv_tele_call_volte_enable(uint8_t card_id, bool enable, srv_func_callback _response_callback);

/**
* aic_srv_tele_call_is_volte_enable
* @card_id: [IN] sim id, 0-sim1, 1-sim2.
* Return: true-VoLTE is enabled, false-VoLTE is disabled.
*/
bool aic_srv_tele_call_is_volte_enable(uint8_t card_id);

/**
* aic_srv_tele_call_is_emergency_number - the whether number is emergency api.
* @card_id: [IN] sim id, 0-sim1, 1-sim2.
* @p_number: [IN] telephone number.
* Return: true-is emergency number, false-is not emergency number.
*/
bool aic_srv_tele_call_is_emergency_number(uint8_t card_id, char *p_number);

/**
* aic_srv_tele_call_get_call_number - the get call number api.
* @p_enable: [OUT] p_call_number - the call number.
* Return: if success return 0, else return other value.
*/
int32_t aic_srv_tele_call_get_call_number(uint8_t *p_call_number);

/**
* aic_srv_tele_call_send_dtmf
* @card_id: [IN] sim id, 0-sim1, 1-sim2.
* @p_dtmf_info: [IN] the dtmf information.
* @_response_callback: [IN] the response callback when client call this api, and it will be called after at response return.
*                      The response event id is TS_EVENT_CALL_SEND_DTMF_RESP, with ts_call_send_dtmf_resp_t struct.
*                      p_param can be transferred to ts_call_send_dtmf_resp_t pointer，and the relative event id is TS_EVENT_CALL_SEND_DTMF_RESP.
*                      size is sizeof(ts_call_send_dtmf_resp_t)
* Return: if success return 0, else return other value.
*/
int32_t aic_srv_tele_call_send_dtmf(uint8_t card_id, ts_call_send_dtmf_info_t *p_dtmf_info, srv_func_callback _response_callback);

/**
* aic_srv_tele_call_have_volte_reg
* @card_id: [IN] sim id, 0-sim1, 1-sim2.
* Return: true: volte has registered, false: volte has not registered.
*/
bool aic_srv_tele_call_have_volte_reg(uint8_t card_id);

/**
* aic_srv_tele_call_set_auto_answer
* @is_auto: [IN] is_auto, true: Anwser automaticlly when incoming call.
*                         false: NOT automaticlly.
* Return: if success return 0, else return other value.
*/
int32_t aic_srv_tele_call_set_auto_answer(bool is_auto);

/**
* aic_srv_tele_call_is_auto_answer
* Return: true, auto answer, false not auto answer.
*/
bool aic_srv_tele_call_is_auto_answer(void);

/**
* aic_srv_tele_call_get_call_info_list
* @card_id: [IN] sim id, 0-sim1, 1-sim2.
*           [OUT] p_call_info_list, the call info list pointer.
* Return: if success return 0, else return other value.
*/
int32_t aic_srv_tele_call_get_call_info_list(uint8_t card_id, ts_call_info_list_t *p_call_info_list);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /* __AIC_SRV_TELE_CALL_H__ */
