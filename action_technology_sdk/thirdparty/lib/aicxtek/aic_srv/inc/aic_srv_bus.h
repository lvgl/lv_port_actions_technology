/******************************************************************************/
/*                                                                            */
/*    Copyright 2023 by AICXTEK TECHNOLOGIES CO.,LTD. All rights reserved.    */
/*                                                                            */
/******************************************************************************/

/**
 * @file aic_srv_bus.h
 *
 */

#ifndef __AIC_SRV_BUS_H__
#define __AIC_SRV_BUS_H__

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include <stdint.h>
#include <stdbool.h>

/*********************
 *      DEFINES
 *********************/
#define aic_handler  int32_t

/**********************
 *      TYPEDEFS
 **********************/
typedef int32_t (*srv_func_callback)(void *p_param, uint32_t size);

/* info struct of aic service bus */
typedef struct
{
    char *p_client_name; /* name of client */

    /* TODO: Add filters which events are needed for client */
}aic_srv_bus_info_t;

typedef struct
{
    char              *p_service_name;  /* name of service which you want to mount */
    char              *p_client_name;   /* name of client eg:"call app","call_audio" */

    srv_func_callback _client_callback; /* the callback API which will be call when service notify clients */
}bs_reg_client_info_t;

typedef struct {
    char *p_service_name; /* name of service whose clients will be notified */
}bs_reg_service_info_t;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
* aic_srv_bus_register_client - service register client api.
* @bs_reg_client_info_t:[IN] srv info,include service name,client name and client callback.
* Return: return handler,0:is invalid value;greater 0,is valid value.
*/
aic_handler aic_srv_bus_register_client(bs_reg_client_info_t *p_reg_info);

/**
* aic_srv_bus_unregister_client - service unregister client api.
* @handle: [IN]  handle,0:is invalid value;greater 0,is valid value.
* @p_client_name: [IN] app name,eg."call audio","call UI"
* Return: 0:success，other:fail.
* Note:first judge handle,then judge client name
*/
int32_t aic_srv_bus_unregister_client(aic_handler handle);

/**
* aic_srv_bus_notify_client -service notify client api.
* @p_service_info: [IN] service moudle name & respcallback.
* @p_param: [IN] param pointer.
* @param_size: [IN] param data length.
* Return: 0:success，other:fail.
*/
int32_t aic_srv_bus_notify_clients(bs_reg_service_info_t *p_service_info, void *p_param, uint32_t param_size);

/**
* aic_srv_bus_have_callback_in_clients -haved callback in clients.
* @_resp_callback: [IN] resp callback.
* Return: true:have:false:not have.
*/
bool aic_srv_bus_have_callback_in_clients(srv_func_callback _resp_callback);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /* __AIC_SRV_BUS_H__ */
