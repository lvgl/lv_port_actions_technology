/******************************************************************************/
/*                                                                            */
/*    Copyright 2023 by AICXTEK TECHNOLOGIES CO.,LTD. All rights reserved.    */
/*                                                                            */
/******************************************************************************/

/**
 * @file aic_srv_tele_radio.h
 *
 */

#ifndef __AIC_SRV_TELE_RADIO_H__
#define __AIC_SRV_TELE_RADIO_H__

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
/* TODO: Service层的错误返回值需要进行统一整理 */
#define TS_RADIO_ERR_REPEAT_STATUS           (-4)
#define TS_RADIO_ERR_GENERIC_FAILURE         (-5)

#define TS_RADIO_OPERATOR_SHORT_NAME_LEN_MAX (10)
#define TS_RADIO_OPERATOR_LONG_NAME_LEN_MAX  (32)
#define TS_RADIO_PLMN_MCC_LEN_MAX            (3)
#define TS_RADIO_PLMN_MNC_LEN_MAX            (3)
#define TS_RADIO_MAX_LTE_NCELL_NUM           (6)
#define TS_RADIO_IMEI_LEN_MAX                (15)

/* the supported filter value */
#define TS_RADIO_FILTER_ALL                  (0xFFFFFFFF)
#define TS_RADIO_FILTER_NONE                 (0)
#define TS_RADIO_FILTER_REG_STATUS           (1 << 0)
#define TS_RADIO_FILTER_SIGNAL_INFO          (1 << 1)

#define TS_EVENT_RADIO_STRUCT_HEAD ts_event_radio_e event_id;

/**********************
 *      TYPEDEFS
 **********************/
typedef enum
{
    /* 返回ts_radio_signal_info_updated_t对应的数据信息 */
    TS_EVENT_RADIO_SIGNAL_INFO_UPDATED = 0x2 << 16,

    /* return ts_radio_status_updated_t relative data information */
    TS_EVENT_RADIO_STATUS_UPDATED,

    /* the confirmation event of radio status on or off */
    TS_EVENT_RADIO_ON_RESP,

    /* return ts_radio_reg_status_updated_t relative data information */
    TS_EVENT_RADIO_REG_STATUS_UPDATED,

    /* return ts_radio_ims_reg_status_updated_t relative data information */
    TS_EVENT_RADIO_IMS_REG_STATUS_UPDATED,

    /* return ts_radio_operator_info_updated_t relative data information */
    TS_EVENT_RADIO_OPERATOR_INFO_UPDATED,

    /* the event of NITZ, with ts_radio_network_time_updated_t data */
    TS_EVENT_RADIO_NETWORK_TIME_UPDATED,

    /* the response of aic_srv_tele_radio_get_cell_info api, with ts_radio_get_cell_info_resp_t data */
    TS_EVENT_RADIO_GET_CELL_INFO_RESP,

    /* the response of aic_srv_tele_radio_get_neighbor_cell_info_list api, with ts_radio_get_neighbor_cell_info_list_resp_t data */
    TS_EVENT_RADIO_GET_NEIGHBOR_CELL_INFO_LIST_RESP,

    /* the response event for set filter */
    TS_EVENT_RADIO_SET_FILTER_RESP,

    /* the response of aic_srv_tele_radio_set_imei api, with ts_radio_set_imei_resp_t data */
    TS_EVENT_RADIO_SET_IMEI_RESP,

    /* the response of aic_srv_tele_radio_start_search api, with ts_radio_start_search_resp_t data */
    TS_EVENT_RADIO_START_SEARCH_RESP,

    /* with ts_radio_get_lte_connection_status_resp_t data */
    TS_EVENT_RADIO_GET_LTE_CONNECTION_STATUS_RESP,

    TS_EVENT_RADIO_MAX
}ts_event_radio_e;

typedef struct
{
    ts_event_radio_e event_id;
}ts_event_radio_t;

typedef enum
{
    /* unknow network type */
    TS_RADIO_NETWORK_TYPE_UNKNOWN,

    /* GSM: Global System For Mobile Communication */
    TS_RADIO_NETWORK_TYPE_GSM,

    /* CDMA: Code Division Multiple Access */
    TS_RADIO_NETWORK_TYPE_CDMA,

    /* WCDMA: Wideband Code Division Multiple Access */
    TS_RADIO_NETWORK_TYPE_WCDMA,

    /* TDSCDMA: TimeDivision-Synchronous Code Division Multiple Access */
    TS_RADIO_NETWORK_TYPE_TDSCDMA,

    /* LTE: Long Term Evolution */
    TS_RADIO_NETWORK_TYPE_LTE,

    /* 5G NR: New Radio */
    TS_RADIO_NETWORK_TYPE_NR,

    TS_RADIO_NETWORK_TYPE_MAX
}ts_radio_network_type_e;

/* 信号强度等信息的数据结构体 */
typedef struct
{
    ts_event_radio_e event_id;

    uint8_t card_id;

    /* radio signal network type */
    ts_radio_network_type_e signal_type;

    /* 信号强度等级，目前支持0~4 */
    uint32_t signal_level;

    /* signal level value, relative with signal_strength(dB) */
    uint32_t signal_level_val;

    /* 信号强度:dB */
    int32_t signal_strength;
}ts_radio_signal_info_updated_t;

/* the struct of radio signal information */
typedef struct
{
    /* radio signal network type */
    ts_radio_network_type_e signal_type;

    /* signal level, currenty support 0~4 */
    uint32_t signal_level;

    /* signal level value, relative with signal_strength(dB) */
    uint32_t signal_level_val;

    /* signal strength: dB */
    int32_t signal_strength;
}ts_radio_signal_info_t;

typedef enum
{
    /* the invalid operator, such as no card, out of service, etc. */
    TS_RADIO_OPERATOR_ID_INVALID,

    TS_RADIO_OPERATOR_ID_CMCC,
    TS_RADIO_OPERATOR_ID_CUCC,
    TS_RADIO_OPERATOR_ID_CTCC,

    /* the unknown operator except for cmcc, cucc and ctcc */
    TS_RADIO_OPERATOR_ID_UNKNOWN,

    TS_RADIO_OPERATOR_ID_MAX,
}ts_radio_operator_id_e;

typedef enum
{
    TS_RADIO_OPERATOR_STATUS_UNKNOW = 0,
    TS_RADIO_OPERATOR_STATUS_AVAILABLE,
    TS_RADIO_OPERATOR_STATUS_CURRENT,
    TS_RADIO_OPERATOR_STATUS_FORBIDDEN,

    TS_RADIO_OPERATOR_STATUS_MAX
}ts_radio_operator_status_e;

typedef struct
{
    char operator_short_name[TS_RADIO_OPERATOR_SHORT_NAME_LEN_MAX + 1];
    char operator_long_name[TS_RADIO_OPERATOR_LONG_NAME_LEN_MAX + 1];
    char plmn_mcc[TS_RADIO_PLMN_MCC_LEN_MAX + 1];
    char plmn_mnc[TS_RADIO_PLMN_MNC_LEN_MAX + 1];
    ts_radio_operator_id_e operator_id;
    ts_radio_network_type_e network_type;
    ts_radio_operator_status_e status;
}ts_radio_operator_info_t;

typedef enum
{
    TS_RADIO_REG_STATUS_NO_SERVICE,
    TS_RADIO_REG_STATUS_IN_SERVICE,
    TS_RADIO_REG_STATUS_EMERGENCY_CALL_ONLY, /* TBD this will be supported later */
    TS_RADIO_REG_STATUS_POWER_OFF,
}ts_radio_reg_status_e;

typedef struct
{
    ts_radio_reg_status_e       reg_status;     /* register network status */
    ts_radio_operator_info_t    operator_info;  /* information of opertator: short/long name, and plmn */
}ts_radio_network_status_t;

typedef struct
{
    ts_radio_network_type_e rat_type;/* Radio Access Technology  */
    char plmn_mcc[TS_RADIO_PLMN_MCC_LEN_MAX + 1];   /* Mobile country code */
    char plmn_mnc[TS_RADIO_PLMN_MNC_LEN_MAX + 1];   /* Mobile network code */
    int32_t tac;    /* Tracking Area Code. */
    int32_t earfcn; /* E-UTRA-ARFCN of the cell that was scanned */
    int32_t cellid;
    int32_t rsrp;   /* Reference Signal Received Power -140~-44， dBm */
    int32_t rsrq;   /* Reference Signal Received Quality -19.5~-3 */
    int32_t pcid;   /* Physical cell ID. 0~503 */
    int32_t is_roaming;
    int32_t band;
    int32_t ul_bandwidth;
    int32_t dl_bandwidth;
    int32_t sinr;
    int32_t srxlev;
    int32_t rssi;
}ts_radio_cell_info_t;

typedef struct
{
    ts_radio_network_type_e rat_type;/* Radio Access Technology  */
    char plmn_mcc[TS_RADIO_PLMN_MCC_LEN_MAX + 1];   /* Mobile country code */
    char plmn_mnc[TS_RADIO_PLMN_MNC_LEN_MAX + 1];   /* Mobile network code */
    int32_t tac;    /* Tracking Area Code. */
    int32_t cellid;
    int32_t earfcn; /* E-UTRA-ARFCN of the cell that was scanned */
    int32_t rsrp;   /* Reference Signal Received Power -140~-44， dBm */
    int32_t rsrq;   /* Reference Signal Received Quality -19.5~-3 */
    int32_t pcid;   /* Hexadecimal format.Physical cell ID. 0~503 */
    int32_t sinr;
    int32_t srxlev;
    int32_t rssi;
}ts_radio_neighbor_cell_info_t;

typedef struct
{
    uint32_t ncell_num;
    ts_radio_neighbor_cell_info_t ncell_info[TS_RADIO_MAX_LTE_NCELL_NUM];
}ts_radio_neighbor_cell_info_list_t;

typedef struct
{
    ts_event_radio_e event_id;

    uint8_t card_id;

    /* true: radio on, false: radio off */
    bool is_on;
}ts_radio_status_updated_t;

typedef struct
{
    TS_EVENT_RADIO_STRUCT_HEAD

    /* 0(success), TS_RADIO_ERR_REPEAT_STATUS, TS_RADIO_ERR_GENERIC_FAILURE */
    int32_t ret_value;
}ts_radio_on_resp_t;

typedef struct
{
    ts_event_radio_e event_id;
    uint8_t card_id;
    ts_radio_reg_status_e reg_status;
}ts_radio_reg_status_updated_t;

typedef struct
{
    ts_event_radio_e event_id;

    uint8_t card_id;

    /* true: ims on, false: ims off */
    bool is_ims_on;
}ts_radio_ims_reg_status_updated_t;

typedef struct
{
    ts_event_radio_e event_id;
    uint8_t card_id;
    ts_radio_operator_info_t operator_info;
}ts_radio_operator_info_updated_t;

typedef struct
{
    ts_event_radio_e event_id;
    uint8_t card_id;
    /* NOTE: year, month, day, hour, min, and sec have been converted to local time. */
    uint32_t year;
    uint32_t month;
    uint32_t day;
    uint32_t hour;
    uint32_t min;
    uint32_t sec;
    /* time zone(expressed in hour, and include daylight saving time), range: -12 ~ +14 */
    int32_t time_zone;
    /* daylight saving time which timezone includes, range: 0,1,2(hour) */
    uint32_t dst;
}ts_radio_network_time_updated_t;

typedef struct
{
    TS_EVENT_RADIO_STRUCT_HEAD

    /* return value: 0 - success, -1 - fail */
    int32_t ret_value;

    /* cell info data */
    ts_radio_cell_info_t cell_info;
}ts_radio_get_cell_info_resp_t;

typedef struct
{
    TS_EVENT_RADIO_STRUCT_HEAD

    /* return value: 0 - success, -1 - fail */
    int32_t ret_value;

    /* cell info data */
    ts_radio_neighbor_cell_info_list_t ncell_info_list;
}ts_radio_get_neighbor_cell_info_list_resp_t;

typedef struct
{
    TS_EVENT_RADIO_STRUCT_HEAD

    /* 0(success), other(fail) */
    int32_t ret_value;
}ts_radio_set_filter_resp_t;

typedef struct
{
    TS_EVENT_RADIO_STRUCT_HEAD

    /* return value: 0 - success, -1 - fail */
    int32_t ret_value;
}ts_radio_set_imei_resp_t;

typedef struct
{
    TS_EVENT_RADIO_STRUCT_HEAD

    /* return value: 0 - success, -1 - fail */
    int32_t ret_value;

    /* the number of operator when search finish */
    uint32_t operator_num;

    /* data_header is pointed to the search result list, ie. the array of struct ts_radio_operator_info_t */
    void *data_header;
}ts_radio_start_search_resp_t;

typedef enum
{
    TS_RADIO_LTE_CONNECTION_STATUS_IDLE = 0,
    TS_RADIO_LTE_CONNECTION_STATUS_CONNECTED,

    TS_RADIO_LTE_CONNECTION_STATUS_MAX
}ts_radio_lte_connection_status_e;

typedef struct
{
    TS_EVENT_RADIO_STRUCT_HEAD

    /* return value: 0 - success, -1 - fail */
    int32_t ret_value;

    /* the lte status */
    ts_radio_lte_connection_status_e lte_connection_status;
}ts_radio_get_lte_connection_status_resp_t;

/**********************
 * GLOBAL PROTOTYPES
 **********************/
/**
* aic_srv_tele_radio_register - tele radio service的注册接口，注册的callback用于接收service层上报的事件.
* @p_bus_info: [IN] 需要client传给service层当前client的名称以及client需要关注的信号起始值和末尾值.
* @_srv_callback: [IN] client传给service层的回调函数.
*                 该回调函数中的参数p_param上报的事件结构体指针, 参数size表示数据的大小（包含event id）
*                 目前可上报的事件有：
*                 1. TS_EVENT_RADIO_SIGNAL_INFO_UPDATED（信号强度发生变化的事件），携带ts_radio_signal_info_updated_t类型的数据信息；
*                    其中，p_param可强转为ts_event_radio_t指针，对应的事件id是TS_EVENT_RADIO_SIGNAL_INFO_UPDATED；
*                          size值为sizeof(ts_radio_signal_info_updated_t)；
*                 2. TS_EVENT_RADIO_STATUS_UPDATED（radio状态发生变化的事件），携带ts_radio_status_updated_t类型的数据信息；
*                    其中，p_param可强转为ts_event_radio_t指针，对应的事件id是TS_EVENT_RADIO_STATUS_UPDATED；
*                          size值为sizeof(ts_radio_status_updated_t)；
*                 3. TS_EVENT_RADIO_ON_RESP（radio打开或关闭的响应事件），携带ts_radio_on_resp_t类型的数据信息；
*                    其中，p_param可强转为ts_event_radio_t指针，对应的事件id是TS_EVENT_RADIO_ON_RESP；
*                          size值为sizeof(ts_radio_on_resp_t)；
*                 4. TS_EVENT_RADIO_REG_STATUS_UPDATED(the radio reg status updated event, with ts_radio_reg_status_updated_t data)
*                    p_param can be transferred to ts_radio_event_t pointer，and the relative event id is TS_EVENT_RADIO_REG_STATUS_UPDATED
*                    size is sizeof(ts_radio_reg_status_updated_t)；
*                 5. TS_EVENT_RADIO_IMS_REG_STATUS_UPDATED(the ims reg status updated event, with ts_radio_ims_reg_status_updated_t data)
*                    p_param can be transferred to ts_radio_event_t pointer，and the relative event id is TS_EVENT_RADIO_IMS_REG_STATUS_UPDATED
*                    size is sizeof(ts_radio_ims_reg_status_updated_t)；
*                 6. TS_EVENT_RADIO_OPERATOR_INFO_UPDATED(the operator inforamtion updated event, with ts_radio_operator_info_updated_t data)
*                    p_param can be transferred to ts_radio_event_t pointer，and the relative event id is TS_EVENT_RADIO_OPERATOR_INFO_UPDATED
*                    size is sizeof(ts_radio_operator_info_updated_t)；
*                 7. TS_EVENT_RADIO_GET_CELL_INFO_RESP(the get cell info response event, with ts_radio_get_cell_info_resp_t data)
*                    p_param can be transferred to ts_radio_get_cell_info_resp_t pointer，and the relative event id is TS_EVENT_RADIO_GET_CELL_INFO_RESP
*                    size is sizeof(ts_radio_get_cell_info_resp_t)；
*                 8. TS_EVENT_RADIO_GET_NEIGHBOR_CELL_INFO_LIST_RESP(the get neighbor cell info list response event, with ts_radio_get_neighbor_cell_info_list_resp_t data)
*                    p_param can be transferred to ts_radio_get_neighbor_cell_info_list_resp_t pointer，and the relative event id is TS_EVENT_RADIO_GET_NEIGHBOR_CELL_INFO_LIST_RESP
*                    size is sizeof(ts_radio_get_neighbor_cell_info_list_resp_t)；
*                 9. TS_EVENT_RADIO_SET_FILTER_RESP(the set filter response event, with ts_radio_set_filter_resp_t data)
*                    p_param can be transferred to ts_radio_set_filter_resp_t pointer，and the relative event id is TS_EVENT_RADIO_SET_FILTER_RESP
*                    size is sizeof(ts_radio_set_filter_resp_t)；
*                 10. TS_EVENT_RADIO_SET_IMEI_RESP(the set imei response event, with ts_radio_set_imei_resp_t data)
*                    p_param can be transferred to ts_radio_set_imei_resp_t pointer，and the relative event id is TS_EVENT_RADIO_SET_IMEI_RESP
*                    size is sizeof(ts_radio_set_imei_resp_t)；
*                 11. TS_EVENT_RADIO_START_SEARCH_RESP(the search start response event, with ts_radio_start_search_resp_t data)
*                     p_param can be transferred to ts_radio_start_search_resp_t pointer，and the relative event id is TS_EVENT_RADIO_START_SEARCH_RESP
*                     size is sizeof(ts_radio_start_search_resp_t)；
*                 TODO: 后续在这里补充callback中返回event id的列表.
* Return: 返回给client对应的client handle，其中-1代表出错，已有client注册.
*/
int32_t aic_srv_tele_radio_register(aic_srv_bus_info_t *p_bus_info, srv_func_callback _srv_callback);

/**
* aic_srv_tele_radio_unregister - tele radio service的去注册接口.
* @client_handle: [IN] 调用aic_srv_tele_radio_register接口时返回给client的client handle值.
* Return: 成功返回0，失败返回其它值.
*/
int32_t aic_srv_tele_radio_unregister(int32_t client_handle);

/**
* aic_srv_tele_radio_get_signal_info - 获取信号强度等相关信息的接口.
* @card_id: [IN] sim卡的id，0-卡槽1，1-卡槽2，目前仅支持单卡（0）.
* @p_signal_info: [OUT] 信号信息结构体.
* Return: 成功返回0，失败返回其它值.
*         如果client设置了filter属性，则返回TS_RADIO_ERR_FILTER；
*/
int32_t aic_srv_tele_radio_get_signal_info(uint8_t card_id, ts_radio_signal_info_t *p_signal_info);

/**
* aic_srv_tele_radio_get_operator_info - get operator information api.
* @card_id: [IN] sim id，0-sim 1，1-sim 2，and currently only support single card(0).
* @p_operator_info: [OUT] the operation information.
* Return: if success return 0, else return other value.
*/
int32_t aic_srv_tele_radio_get_operator_info(uint8_t card_id, ts_radio_operator_info_t *p_operator_info);

/**
* aic_srv_tele_radio_on - the open/close radio api.
* @card_id: [IN] sim id, 0-sim 1, 1-sim2, currently only support single card(0).
* @raido_on: [IN] true-open radio, false-close radio
* @_response_callback: [IN] the response callback that client registered, and it will be called to return the operate result.
*                TS_EVENT_RADIO_STATUS_UPDATED will be reported after radio open/close succeed, and it will notify client with the registered callback.
*                The response event id is TS_EVENT_RADIO_ON_RESP, with ts_radio_on_resp_t struct.
*                p_param can be transferred to ts_event_radio_t pointer，and the relative event id is TS_EVENT_RADIO_ON_RESP.
*                size is sizeof(ts_radio_on_resp_t)
* Return: if success return 0, else return other value.
*/
int32_t aic_srv_tele_radio_on(uint8_t card_id, bool radio_on, srv_func_callback _response_callback);

/**
* aic_srv_tele_radio_is_on
* @card_id: [IN] sim id, 0-sim 1, 1-sim2, currently only support single card(0).
* Return: true: is on, false: is off.
*/
bool aic_srv_tele_radio_is_on(uint8_t card_id);

/**
* aic_srv_tele_radio_get_reg_status
* @card_id: [IN] sim id, 0-sim 1, 1-sim2.
* @p_reg_status: [OUT] reg status, network register status.
* Return: if success return 0, else return other value.
*/
int32_t aic_srv_tele_radio_get_reg_status(uint8_t card_id, ts_radio_reg_status_e *p_reg_status);

/**
* aic_srv_tele_radio_have_ims_reg
* @card_id: [IN] sim id, 0-sim 1, 1-sim2.
* Return: true: ims has registered, false: ims has not registered.
*/
bool aic_srv_tele_radio_have_ims_reg(uint8_t card_id);

/**
* aic_srv_tele_radio_set_imei - the set imei api.
* @card_id: [IN] sim id, 0-sim1, 1-sim2.
* @p_imei: [IN] the imei string.
* @_response_callback: [IN] the response callback that client registered, and it will be called to return the operate result.
*                TS_EVENT_RADIO_SET_IMEI_RESP will be reported, and it will notify client with the registered callback.
*                The response event id is TS_EVENT_RADIO_SET_IMEI_RESP, with ts_radio_set_imei_resp_t struct.
*                p_param can be transferred to ts_event_radio_t pointer，and the relative event id is TS_EVENT_RADIO_SET_IMEI_RESP.
*                size is sizeof(ts_radio_set_imei_resp_t)
* Return: if success return 0, else return other value.
*/
int32_t aic_srv_tele_radio_set_imei(uint8_t card_id, char *p_imei, srv_func_callback _response_callback);

/**
* aic_srv_tele_radio_get_imei - the get imei api.
* @card_id: [IN] sim id, 0-sim1, 1-sim2.
* @p_imei: [OUT] the imei string.
* Return: if success return 0, else return other value.
*/
int32_t aic_srv_tele_radio_get_imei(uint8_t card_id, char *p_imei);

/**
* aic_srv_tele_radio_have_cali.
* Return: true - have calibrated, false - have not calibrated.
*/
bool aic_srv_tele_radio_have_cali(void);

/**
* aic_srv_tele_radio_get_cell_info - the get cell info api.
* @card_id: [IN] sim id, 0-sim1, 1-sim2.
* @_response_callback: [IN] the response callback that client registered, and it will be called to return the operate result.
*                TS_EVENT_RADIO_GET_CELL_INFO_RESP will be reported after get cell info response back, and it will notify client with the registered callback.
*                The response event id is TS_EVENT_RADIO_GET_CELL_INFO_RESP, with ts_radio_get_cell_info_resp_t struct.
*                p_param can be transferred to ts_event_radio_t pointer，and the relative event id is TS_EVENT_RADIO_GET_CELL_INFO_RESP.
*                size is sizeof(ts_radio_get_cell_info_resp_t)
* Return: if success return 0, else return other value.
*/
int32_t aic_srv_tele_radio_get_cell_info(uint8_t card_id, srv_func_callback _response_callback);

/**
* aic_srv_tele_radio_get_neighbor_cell_info_list - the get neighbor cell info list api.
* @card_id: [IN] sim id, 0-sim1, 1-sim2.
* @_response_callback: [IN] the response callback that client registered, and it will be called to return the operate result.
*                TS_EVENT_RADIO_GET_NEIGHBOR_CELL_INFO_LIST_RESP will be reported after get neighbor cell info list response back, and it will notify client with the registered callback.
*                The response event id is TS_EVENT_RADIO_GET_NEIGHBOR_CELL_INFO_LIST_RESP, with ts_radio_get_neighbor_cell_info_list_resp_t struct.
*                p_param can be transferred to ts_event_radio_t pointer，and the relative event id is TS_EVENT_RADIO_GET_NEIGHBOR_CELL_INFO_LIST_RESP.
*                size is sizeof(ts_radio_get_neighbor_cell_info_list_resp_t)
* Return: if success return 0, else return other value.
*/
int32_t aic_srv_tele_radio_get_neighbor_cell_info_list(uint8_t card_id, srv_func_callback _response_callback);

/**
* aic_srv_tele_radio_set_filter
* @card_id: [IN] sim id, 0-sim 1, 1-sim2.
* @filter_val: [IN] the filter value.
*                   #define TS_RADIO_FILTER_ALL         (0xFFFFFFFF)
*                   #define TS_RADIO_FILTER_NONE        (0)
*                   #define TS_RADIO_FILTER_REG_STATUS  (1 << 0)
*                   #define TS_RADIO_FILTER_SIGNAL_INFO (1 << 1)
* Return: if success return 0, else return other value.
*/
int32_t aic_srv_tele_radio_set_filter(uint8_t card_id, uint32_t filter_val, srv_func_callback _response_callback);

/**
* aic_srv_tele_radio_start_search - the search network api.
* @card_id: [IN] sim id, 0-sim1, 1-sim2.
* @_response_callback: [IN] the response callback that client registered, and it will be called to return the operate result.
*                TS_EVENT_RADIO_START_SEARCH_RESP will be reported after start search response back, and it will notify client with the registered callback.
*                The response event id is TS_EVENT_RADIO_START_SEARCH_RESP, with ts_radio_get_neighbor_cell_info_list_resp_t struct.
*                p_param can be transferred to ts_radio_start_search_resp_t pointer，and the relative event id is TS_EVENT_RADIO_START_SEARCH_RESP.
*                size is the size of event_id, ret_value, operator_num and the search result data(the array of struct ts_radio_operator_info_t) in ts_radio_start_search_resp_t.
* Return: if success return 0, else return other value.
*/
int32_t aic_srv_tele_radio_start_search(uint8_t card_id, srv_func_callback _response_callback);

/**
* aic_srv_tele_radio_get_lte_connection_status.
* @card_id: [IN] sim id, 0-sim1, 1-sim2.
* @_response_callback: [IN] the response callback that client registered, and it will be called to return the operate result.
*                TS_EVENT_RADIO_GET_LTE_CONNECTION_STATUS_RESP will be reported after response back, and it will notify client with the registered callback.
*                The response event id is TS_EVENT_RADIO_GET_LTE_CONNECTION_STATUS_RESP, with ts_radio_get_lte_connection_status_resp_t struct.
*                p_param can be transferred to ts_radio_get_lte_connection_status_resp_t pointer，and the relative event id is TS_EVENT_RADIO_GET_LTE_CONNECTION_STATUS_RESP.
*                size is the size of ts_radio_get_lte_connection_status_resp_t.
* Return: if success return 0, else return other value.
*/
int32_t aic_srv_tele_radio_get_lte_connection_status(uint8_t card_id, srv_func_callback _response_callback);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /* __AIC_SRV_TELE_RADIO_H__ */
