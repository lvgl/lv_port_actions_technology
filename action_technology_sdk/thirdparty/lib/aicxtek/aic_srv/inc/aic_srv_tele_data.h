/******************************************************************************/
/*                                                                            */
/*    Copyright 2023 by AICXTEK TECHNOLOGIES CO.,LTD. All rights reserved.    */
/*                                                                            */
/******************************************************************************/

/**
 * @file aic_srv_tele_data.h
 *
 */

#ifndef __AIC_SRV_TELE_DATA_H__
#define __AIC_SRV_TELE_DATA_H__

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
#define TS_DATA_APN_NAME_LEN_MAX  (60)
#define TS_DATA_USER_NAME_LEN_MAX (30)
#define TS_DATA_PASSWORD_LEN_MAX  (30)

#define TS_DATA_TYPE_LEN_MAX      (30)
#define TS_DATA_IPV4_ADDR_LEN_MAX (16)
#define TS_DATA_IPV6_ADDR_LEN_MAX (64)
#define TS_DATA_IPV4_DNS_LEN_MAX  (16)
#define TS_DATA_IPV6_DNS_LEN_MAX  (64)
#define TS_DATA_GATEWAY_LEN_MAX   (16)

#define TS_DATA_TYPE_IPV4         "IPV4"
#define TS_DATA_TYPE_IPV6         "IPV6"
#define TS_DATA_TYPE_IPV4V6       "IPV4V6"

/**********************
 *      TYPEDEFS
 **********************/
typedef enum
{
    /* the event that pdp status updated, with ts_data_pdp_status_updated information, 0x4 indicates data service module */
    TS_EVENT_DATA_PDP_STATUS_UPDATED = 0x4 << 16,

    TS_EVENT_DATA_MAX
}ts_event_data_e;

typedef struct
{
    ts_event_data_e event_id;
}ts_event_data_t;

typedef enum
{
    TS_DATA_APN_TYPE_INTERNET,
    TS_DATA_APN_TYPE_IMS,

    TS_DATA_APN_TYPE_MAX
}ts_data_apn_type_e;

typedef enum
{
    TS_DATA_PDP_TYPE_IPV4V6,
    TS_DATA_PDP_TYPE_IPV4,
    TS_DATA_PDP_TYPE_IPV6,

    TS_DATA_PDP_TYPE_MAX
}ts_data_pdp_type_e;

typedef enum
{
    TS_DATA_AUTH_TYPE_PAP,
    TS_DATA_AUTH_TYPE_CHAP,
    TS_DATA_AUTH_TYPE_PAP_OR_CHAP,

    TS_DATA_AUTH_TYPE_MAX
}ts_data_auth_type_e;

/* pdp activate information struct */
typedef struct
{
    /* apn type */
    ts_data_apn_type_e apn_type;

    /* access point name */
    char apn[TS_DATA_APN_NAME_LEN_MAX + 1];

    /* user name */
    char user_name[TS_DATA_USER_NAME_LEN_MAX + 1];

    /* password */
    char password[TS_DATA_PASSWORD_LEN_MAX + 1];

    /* pdp type */
    ts_data_pdp_type_e pdp_type;

    /* auth type */
    ts_data_auth_type_e auth_type;
}ts_data_pdp_activate_info_t;

typedef enum
{
    TS_DATA_PDP_STATUS_DEACTIVED,
    TS_DATA_PDP_STATUS_ACTIVED,

    TS_DATA_PDP_STATUS_MAX
}ts_data_pdp_status_e;

typedef struct
{
    uint32_t cid;
    char     type[TS_DATA_TYPE_LEN_MAX + 1]; /* "IPV4", "IPV6", "IPV4V6" */
    char     apn_name[TS_DATA_APN_NAME_LEN_MAX + 1];
    char     gateway[TS_DATA_GATEWAY_LEN_MAX + 1];
    char     ipv4_addr[TS_DATA_IPV4_ADDR_LEN_MAX + 1];
    char     ipv4_prim_dns[TS_DATA_IPV4_DNS_LEN_MAX + 1];
    char     ipv4_sec_dns[TS_DATA_IPV4_DNS_LEN_MAX + 1];
    char     ipv6_addr[TS_DATA_IPV6_ADDR_LEN_MAX + 1];
    char     ipv6_prim_dns[TS_DATA_IPV6_DNS_LEN_MAX + 1];
    char     ipv6_sec_dns[TS_DATA_IPV6_DNS_LEN_MAX + 1];
}ts_data_link_info_t;

/* pdp information struct */
typedef struct
{
    ts_data_pdp_status_e pdp_status;
    ts_data_link_info_t link_info;/* current save cid=1(internet link info) */
    /* TODO: add member apn detail info,eg. apn,apn type,gateway,port */
}ts_data_pdp_info_t;

typedef struct
{
    ts_event_data_e         event_id; /* TS_EVENT_DATA_PDP_STATUS_UPDATED */
    uint8_t                 card_id;
    ts_data_pdp_status_e    pdp_status;
    ts_data_link_info_t     link_info; /* network link info */
}ts_data_pdp_status_updated_t;

/**********************
 * GLOBAL PROTOTYPES
 **********************/
/**
* aic_srv_tele_data_register - the tele data service register api，and the registered callback is used to handle the events that serviced notified.
* @p_bus_info: [IN] 需要client传给service层当前client的名称以及client需要关注的信号相关数据信息.
* @_srv_callback: [IN] client传给service层的回调函数.
*                 该回调函数中的参数p_param上报数据结构体指针, 参数size表示数据的大小（包含event id）
*                 目前可上报的事件有：
*                 1. TS_EVENT_DATA_PDP_STATUS_UPDATED(the event id that pdp status changed), with ts_data_pdp_status_updated_t information.
*                    p_param can be converted to ts_event_data_t pointer, and the relative event id is TS_EVENT_DATA_PDP_STATUS_UPDATED.
*                    size is sizeof(ts_data_pdp_status_updated_t)；
*                 TODO: later add the event id list handled in callback.
* Return: the client handle, and the error value -1 indicates some client has already registered.
*/
int32_t aic_srv_tele_data_register(aic_srv_bus_info_t *p_bus_info, srv_func_callback _srv_callback);

/**
* aic_srv_tele_data_unregister - tele data service unregister api.
* @client_handle: [IN] the client handle that be returned when client call aic_srv_tele_call_register.
* Return: return 0 if success, else return other value.
*/
int32_t aic_srv_tele_data_unregister(int32_t client_handle);

/**
* aic_srv_tele_data_get_internet_pdp_info
* @card_id: [IN] sim id, 0-sim 1, 1-sim2.
* @p_pdp_data_info: [OUT] pdp data info.
* Return: return 0 if success, else return other value.
*/
int32_t aic_srv_tele_data_get_internet_pdp_info(uint8_t card_id, ts_data_pdp_info_t *p_pdp_data_info);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /* __AIC_SRV_TELE_DATA_H__ */

