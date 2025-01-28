/******************************************************************************/
/*                                                                            */
/*    Copyright 2023 by AICXTEK TECHNOLOGIES CO.,LTD. All rights reserved.    */
/*                                                                            */
/******************************************************************************/

/**
 * @file aic_srv_tele_sim.h
 *
 */

#ifndef __AIC_SRV_TELE_SIM_H__
#define __AIC_SRV_TELE_SIM_H__

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include <stdint.h>
#include "aic_srv_bus.h"

/*********************
 *      DEFINES
 *********************/
#define TS_SIM_PIN_PWD_LEN_MAX (8)

#define TS_SIM_IMSI_LEN_MAX    (15)

#define TS_SIM_MSISDN_LEN_MAX  (14)

#define TS_EVENT_SIM_STRUCT_HEAD ts_event_sim_e event_id;

/* if the request is triggered by service internal, no need to notify client */
#define TS_REQUEST_BY_SERVICE_INTERNAL 0xFFFFFFFF

/**********************
 *      TYPEDEFS
 **********************/
typedef enum
{
    /* sim卡状态发生变化的事件，携带ts_sim_status_updated_t的数据信息 */
    TS_EVENT_SIM_STATUS_UPDATED = 0x3 << 16,

    /* sim卡状态发生变化的事件，携带ts_sim_status_updated_with_data_t的指针数据信息 */
    TS_EVENT_SIM_STATUS_UPDATED_WITH_DATA,

    /* the set sim pin lock status response event, with ts_sim_set_pin_lock_status_resp_t data. */
    TS_EVENT_SIM_SET_PIN_LOCK_STATUS_RESP,

    /* the get sim pin lock status response event, with ts_sim_get_pin_lock_status_resp_t data */
    TS_EVENT_SIM_GET_PIN_LOCK_STATUS_RESP,

    /* the unlock sim pin response event, with ts_sim_unlock_pin_resp_t data */
    TS_EVENT_SIM_UNLOCK_PIN_RESP,

    /* the change sim pin response event, with ts_sim_change_pin_resp_t data */
    TS_EVENT_SIM_CHANGE_PIN_RESP,

    /* the get sim pin reamin times response event, with ts_sim_get_pin_remain_times_resp_t data */
    TS_EVENT_SIM_GET_PIN_REAMIN_TIMES_RESP,

    /* the select card response event, with ts_sim_select_card_resp_t data */
    TS_EVENT_SIM_SELECT_CARD_RESP,

    TS_EVENT_SIM_MAX
}ts_event_sim_e;

typedef struct
{
    ts_event_sim_e event_id;
}ts_event_sim_t;

typedef enum
{
    TS_CARD_ID_0, /* slot 1 */
    TS_CARD_ID_1, /* slot 2 */

    TS_CARD_ID_MAX
}ts_card_id_e;

typedef enum
{
    /* sim卡not ready */
    TS_SIM_STATUS_NOT_READY = -1,

    /* sim卡不存在 */
    TS_SIM_STATUS_NOT_PRESENT,

    /* sim卡ready */
    TS_SIM_STATUS_READY,

    /* sim卡pin */
    TS_SIM_STATUS_PIN,

    /* sim卡puk */
    TS_SIM_STATUS_PUK,

    /* sim卡pin2 */
    TS_SIM_STATUS_PIN2,

    /* sim卡puk2 */
    TS_SIM_STATUS_PUK2,

    TS_SIM_STATUS_MAX
}ts_sim_status_e;

typedef struct
{
    ts_event_sim_e event_id;
    /* the sim exist status(0(0000): no sim card, 1(0001): sim1 exist, 2(0010): sim2 exist, 3(0011): sim1 & sim2 both exist */
    uint32_t sim_exist_status;
    uint8_t cur_card_id;
}ts_sim_status_updated_t;

typedef struct
{
    ts_event_sim_e event_id;
    ts_sim_status_e sim_status[TS_CARD_ID_MAX];
}ts_sim_status_updated_with_data_t;

typedef struct
{
    bool is_enable;
    char pin_pwd[TS_SIM_PIN_PWD_LEN_MAX];
}ts_sim_pin_lock_status_t;

typedef struct
{
    TS_EVENT_SIM_STRUCT_HEAD

    /* 0:success, other:fail. */
    int32_t ret_value;

    /* -1: set lock status success, other value: pin remain times if set lock status fail */
    int32_t remain_times;
}ts_sim_set_pin_lock_status_resp_t;

typedef struct
{
    TS_EVENT_SIM_STRUCT_HEAD

    /* 0:success, -1:fail. */
    int32_t ret_value;

    /* 0: pin is closed, 1: pin is open. */
    int32_t status;
}ts_sim_get_pin_lock_status_resp_t;

typedef struct
{
    TS_EVENT_SIM_STRUCT_HEAD

    /* 0:success, other:fail. */
    int32_t ret_value;

    /* -1: unlcok pin success, other value: if unlock pin fail */
    int32_t remain_times;
}ts_sim_unlock_pin_resp_t;

typedef struct
{
    TS_EVENT_SIM_STRUCT_HEAD

    /* 0:success, other:fail. */
    int32_t ret_value;

    /* -1: change pin success, other value: if change pin fail */
    int32_t remain_times;
}ts_sim_change_pin_resp_t;

typedef struct
{
    TS_EVENT_SIM_STRUCT_HEAD

    /* 0:success, -1:fail. */
    int32_t ret_value;

    /* the remain times */
    int32_t remain_times;
}ts_sim_get_pin_remain_times_resp_t;

typedef struct
{
    TS_EVENT_SIM_STRUCT_HEAD

    uint8_t card_id;

    /* 0:success, -1:fail. */
    int32_t ret_value;
}ts_sim_select_card_resp_t;

/**********************
 * GLOBAL PROTOTYPES
 **********************/
/**
* aic_srv_tele_sim_register - tele sim service的注册接口，注册的callback用于接收service层上报的事件.
* @p_bus_info: [IN] 需要client传给service层当前client的名称以及client需要关注的信号起始值和末尾值.
* @_srv_callback: [IN] client传给service层的回调函数.
*                 该回调函数中的参数p_param上报的事件结构体指针, 参数size表示数据的大小（包含event id）
*                 目前可上报的事件有：
*                 1. TS_EVENT_SIM_STATUS_UPDATED（sim卡状态发生变化的事件），携带ts_sim_status_updated_t数据；
*                    其中，p_param可强转为ts_event_sim_t指针，对应的事件id是TS_EVENT_SIM_STATUS_UPDATED；
*                          size值为sizeof(ts_sim_status_updated_t)；
*                 2. TS_EVENT_SIM_STATUS_UPDATED_WITH_DATA（sim卡状态发生变化的事件，携带ts_sim_status_updated_with_data_t类型的数据信息）
*                    其中，p_param可强转为ts_event_sim_t指针，对应的事件id是TS_EVENT_SIM_STATUS_UPDATED_WITH_DATA；
*                                 也可强转为ts_sim_status_updated_with_data_t指针，包含事件id、card id和sim卡状态
*                          size值为ts_sim_status_updated_with_data_t的结构体大小；
*                 3. TS_EVENT_SIM_SET_PIN_LOCK_STATUS_RESP(the set pin lock status response event, with ts_sim_set_pin_lock_status_resp_t data)
*                    p_param can be transferred to ts_sim_set_pin_lock_status_resp_t pointer，and the relative event id is TS_EVENT_SIM_SET_PIN_LOCK_STATUS_RESP
*                    size is sizeof(ts_sim_set_pin_lock_status_resp_t);
*                 4. TS_EVENT_SIM_GET_PIN_LOCK_STATUS_RESP(the get pin lock status response event, with ts_sim_get_pin_lock_status_resp_t data)
*                    p_param can be transferred to ts_sim_get_pin_lock_status_resp_t pointer，and the relative event id is TS_EVENT_SIM_GET_PIN_LOCK_STATUS_RESP
*                    size is sizeof(ts_sim_get_pin_lock_status_resp_t);
*                 5. TS_EVENT_SIM_UNLOCK_PIN_RESP(the unlock pin response event, with ts_sim_unlock_pin_resp_t data)
*                    p_param can be transferred to ts_sim_unlock_pin_resp_t pointer，and the relative event id is TS_EVENT_SIM_UNLOCK_PIN_RESP
*                    size is sizeof(ts_sim_unlock_pin_resp_t);
*                 6. TS_EVENT_SIM_CHANGE_PIN_RESP(the change pin response event, with ts_sim_change_pin_resp_t data)
*                    p_param can be transferred to ts_sim_change_pin_resp_t pointer，and the relative event id is TS_EVENT_SIM_CHANGE_PIN_RESP
*                    size is sizeof(ts_sim_change_pin_resp_t);
*                 7. TS_EVENT_SIM_GET_PIN_REAMIN_TIMES_RESP(the get pin remain times response event, with ts_sim_get_pin_remain_times_resp_t data)
*                    p_param can be transferred to ts_sim_get_pin_remain_times_resp_t pointer，and the relative event id is TS_EVENT_SIM_GET_PIN_REAMIN_TIMES_RESP
*                    size is sizeof(ts_sim_get_pin_remain_times_resp_t);
*                 8. TS_EVENT_SIM_SELECT_CARD_RESP(the select card response event, with ts_sim_select_card_resp_t data)
*                    p_param can be transferred to ts_sim_select_card_resp_t pointer，and the relative event id is TS_EVENT_SIM_SELECT_CARD_RESP
*                    size is sizeof(ts_sim_select_card_resp_t);
*                 TODO: 后续在这里补充callback中返回msg id的列表.
* Return: 返回给client对应的client handle，其中-1代表出错，已有client注册.
*/
int32_t aic_srv_tele_sim_register(aic_srv_bus_info_t *p_bus_info, srv_func_callback _srv_callback);

/**
* aic_srv_tele_sim_unregister - tele sim service的去注册接口.
* @client_handle: [IN] 调用aic_srv_tele_sim_register接口时返回给client的client handle值.
* Return: 成功返回0，失败返回其它值.
*/
int32_t aic_srv_tele_sim_unregister(int32_t client_handle);

/**
* aic_srv_tele_sim_get_status - 获取sim卡状态的接口.
* @card_id: [IN] sim卡的id，0-卡槽1，1-卡槽2，目前仅支持单卡（0）.
* @p_sim_status: [OUT] sim卡的状态.
* Return: 成功返回0，失败返回其它值.
*/
int32_t aic_srv_tele_sim_get_status(uint8_t card_id, ts_sim_status_e *p_sim_status);

/**
* aic_srv_tele_sim_set_pin_lock_status.
* @card_id: [IN] sim card id, 0- card 1, 1- card 2
* @p_lock_status: [IN] the sim pin lock status pointer.
* @_response_callback: [IN] the response callback when client call this api, and it will be called after at response return.
*                      The response event id is TS_EVENT_SIM_SET_PIN_LOCK_STATUS_RESP, with ts_sim_set_pin_lock_status_resp_t struct.
*                      p_param can be transferred to ts_sim_set_pin_lock_status_resp_t pointer，and the relative event id is TS_EVENT_SIM_SET_PIN_LOCK_STATUS_RESP.
*                      size is sizeof(ts_sim_set_pin_lock_status_resp_t)
* Return: 0:success;fail,other value.
*/
int32_t aic_srv_tele_sim_set_pin_lock_status(uint8_t card_id, ts_sim_pin_lock_status_t *p_lock_status, srv_func_callback _response_callback);

/**
* aic_srv_tele_sim_get_pin_lock_status.
* @card_id: [IN] sim card id, 0- card 1, 1- card 2
* @_response_callback: [IN] the response callback when client call this api, and it will be called after at response return.
*                      The response event id is TS_EVENT_SIM_GET_PIN_LOCK_STATUS_RESP, with ts_sim_get_pin_lock_status_resp_t struct.
*                      p_param can be transferred to ts_sim_get_pin_lock_status_resp_t pointer，and the relative event id is TS_EVENT_SIM_GET_PIN_LOCK_STATUS_RESP.
*                      size is sizeof(ts_sim_get_pin_lock_status_resp_t)
* Return: 0:success;fail,other value.
*/
int32_t aic_srv_tele_sim_get_pin_lock_status(uint8_t card_id, srv_func_callback _response_callback);

/**
* aic_srv_tele_sim_unlock_pin.
* @card_id: [IN] sim card id, 0- card 1, 1- card 2
* @p_pin: [IN] the input pin pwd string pointer
* @_response_callback: [IN] the response callback when client call this api, and it will be called after at response return.
*                      The response event id is TS_EVENT_SIM_UNLOCK_PIN_RESP, with ts_sim_unlock_pin_resp_t struct.
*                      p_param can be transferred to ts_sim_unlock_pin_resp_t pointer，and the relative event id is TS_EVENT_SIM_UNLOCK_PIN_RESP.
*                      size is sizeof(ts_sim_unlock_pin_resp_t)
* Return: 0:success;fail,other value.
*/
int32_t aic_srv_tele_sim_unlock_pin(uint8_t card_id, char *p_pin, srv_func_callback _response_callback);

/**
* aic_srv_tele_sim_change_pin.
* @card_id: [IN] sim card id, 0- card 1, 1- card 2
* @p_new_pin: [IN] the new input pin pwd string pointer
* @p_old_pin: [IN] the old input pin pwd string pointer
* @_response_callback: [IN] the response callback when client call this api, and it will be called after at response return.
*                      The response event id is TS_EVENT_SIM_CHANGE_PIN_RESP, with ts_sim_change_pin_resp_t struct.
*                      p_param can be transferred to ts_sim_change_pin_resp_t pointer，and the relative event id is TS_EVENT_SIM_CHANGE_PIN_RESP.
*                      size is sizeof(ts_sim_change_pin_resp_t)
* Return: 0:success;fail,other value.
*/
int32_t aic_srv_tele_sim_change_pin(uint8_t card_id, char *p_new_pin, char *p_old_pin, srv_func_callback _response_callback);

/**
* aic_srv_tele_sim_get_pin_remain_times.
* @card_id: [IN] sim card id, 0- card 1, 1- card 2
* @_response_callback: [IN] the response callback when client call this api, and it will be called after at response return.
*                      The response event id is TS_EVENT_SIM_GET_PIN_REMAIN_TIMES_RESP, with ts_sim_get_pin_remain_times_resp_t struct.
*                      p_param can be transferred to ts_sim_get_pin_remain_times_resp_t pointer，and the relative event id is TS_EVENT_SIM_GET_PIN_REMAIN_TIMES_RESP.
*                      size is sizeof(ts_sim_get_pin_remain_times_resp_t)
* Return: 0:success;fail,other value.
*/
int32_t aic_srv_tele_sim_get_pin_remain_times(uint8_t card_id, srv_func_callback _response_callback);

/**
* aic_srv_tele_sim_select_card.
* @card_id: [IN] sim card id, 0- card 1, 1- card 2
* @_response_callback: [IN] the response callback when client call this api, and it will be called after at response return.
*                      The response event id is TS_EVENT_SIM_SELECT_CARD_RESP, with ts_sim_select_card_resp_t struct.
*                      p_param can be transferred to ts_sim_select_card_resp_t pointer，and the relative event id is TS_EVENT_SIM_SELECT_CARD_RESP.
*                      size is sizeof(ts_sim_select_card_resp_t)
* Return: 0:success;fail,other value.
*/
int32_t aic_srv_tele_sim_select_card(uint8_t card_id, srv_func_callback _response_callback);

/**
* aic_srv_tele_sim_get_current_card.
* @p_card_id: [OUT] the sim card id
* Return: 0:success;fail,other value.
*/
int32_t aic_srv_tele_sim_get_current_card(uint8_t *p_card_id);

/**
* aic_srv_tele_sim_get_exist_status(only for check whether sim card is existed).
* @p_sim_status: [OUT] the sim exist status(0(0000): no sim card, 1(0001): sim1 exist, 2(0010): sim2 exist, 3(0011): sim1 & sim2 both exist.
* Return: 0:success;fail,other value.
*/
int32_t aic_srv_tele_sim_get_exist_status(uint32_t *p_sim_status);

/**
* aic_srv_tele_sim_get_imsi - the get imsi api.
* @card_id: [IN] sim id, 0-sim1, 1-sim2.
* @p_sim_imsi: [OUT] the sim imsi string.
* Return: if success return 0, else return other value.
*/
int32_t aic_srv_tele_sim_get_imsi(uint8_t card_id, char *p_sim_imsi);

/**
* aic_srv_tele_sim_get_msisdn - the get msisdn api.
* @card_id: [IN] sim id, 0-sim1, 1-sim2.
* @p_sim_msisdn: [OUT] the sim msisdn string.
* Return: if success return 0, else return other value.
*/
int32_t aic_srv_tele_sim_get_msisdn(uint8_t card_id, char *p_sim_msisdn);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /* __AIC_SRV_TELE_SIM_H__ */
