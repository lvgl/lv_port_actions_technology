/******************************************************************************/
/*                                                                            */
/*    Copyright 2023 by AICXTEK TECHNOLOGIES CO.,LTD. All rights reserved.    */
/*                                                                            */
/******************************************************************************/

/**
 * @file aic_srv_voice.h
 *
 */

#ifndef __AIC_SRV_VOICE_H__
#define __AIC_SRV_VOICE_H__

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include <stdint.h>
#include "aic_srv_bus.h"
#include "tele_cfg.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

typedef enum
{
    /* the event of prepare voice service start */
    VS_EVENT_PRE_START,

    /* the event of voice service start */
    VS_EVENT_START,

    /* the event of voice service stop */
    VS_EVENT_STOP,

    VS_EVENT_MAX
}vs_event_e;

typedef enum
{
    /* the event of prepare start ringback */
    VS_EVENT_DETAIL_PRE_START_RINGBCAK,

    /* the event of start local ringback */
    VS_EVENT_DETAIL_START_LOCAL_RINGBCAK,

    /* the event of start network ringback */
    VS_EVENT_DETAIL_START_NETWORK_RINGBCAK,

    /* the event of stop ringback */
    VS_EVENT_DETAIL_STOP_RINGBCAK,

    /* the event of prepare start voice */
    VS_EVENT_DETAIL_PRE_START_VOICE,

    /* the event of start voice */
    VS_EVENT_DETAIL_START_VOICE,

    /* the event of stop voice */
    VS_EVENT_DETAIL_STOP_VOICE,

    VS_EVENT_DETAIL_MAX
}vs_detail_event_e;

typedef struct
{
    vs_event_e         e_event;       /* the event of voice service */
    vs_detail_event_e  e_detail_event;/* the detail event when voice/ringback change */
}vs_event_t;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/*************** The APIs which customer used **********************/

/**
* aic_srv_voice_set_ringback_type - The API of set ringback type.
* @ringback_type:  [IN] value of ringback type
* Return: if success return 0, else return other value.
*/
int32_t aic_srv_voice_set_ringback_type(tele_voice_local_ringback_type_e ringback_type);

/**
* aic_srv_voice_get_ringback_type - The API of get ringback type.
* Return: current ringback type
*/
tele_voice_local_ringback_type_e aic_srv_voice_get_ringback_type(void);

/**
* aic_srv_voice_set_volume - The API of set voice volume.
* @volume:  [IN] value of voice volume
* Return: if success return 0, else return other value.
*/
int32_t aic_srv_voice_set_volume(uint32_t volume);

/**
* aic_srv_voice_get_volume - The API of get voice volume..
* Return: current voice volume.
*/
uint32_t aic_srv_voice_get_volume(void);

/**
* aic_srv_voice_enable_loopback - The API of enable loopback.
* @enable:  [IN] ture: enable, false: disable
* Return: if success return 0, else return other value.
*/
int32_t aic_srv_voice_enable_loopback(bool enable);

/**
* aic_srv_voice_is_enable_loopback - The API of get loopback enable state.
* Return: ture: enable, false: disable.
*/
bool aic_srv_voice_is_enable_loopback(void);

/**
* aic_srv_voice_exchange_data - The API of data exchange which is used when transmitting voice over the telephony.
* @write_buf:  [IN]
* @write_size: [IN/OUT]
* @read_buf:   [IN]
* @read_size:  [IN/OUT]
* @timeout:    [IN]
* Return: if success return 0, else return other value.
*/
int32_t aic_srv_voice_exchange_data(char *write_buf, uint32_t *write_size, char *read_buf, uint32_t *read_size, uint32_t timeout);

/**
* aic_srv_voice_register - register voice service to handle voice event using vs_event_e definitions.
* @p_bus_info: [IN] client name.
* @_srv_callback: [IN] the voice event handle callback.
* Return: client handle，error value: 0.
*/
int32_t aic_srv_voice_register(aic_srv_bus_info_t *p_bus_info, srv_func_callback _srv_callback);

/**
* aic_srv_voice_unregister - unregister voice service.
* @client_handle: [IN] client handle which return by aic_srv_voice_register.
* Return: if success return 0, else return other value.
*/
int32_t aic_srv_voice_unregister(int32_t client_handle);




/*************** The APIs which aic srv module used **********************/

/**
* aic_srv_voice_init - init voice service.
* Return: if success return 0, else return other value.
*/
int32_t aic_srv_voice_init(void);

/**
* aic_srv_voice_deinit - deinit voice service.
* Return: if success return 0, else return other value.
*/
int32_t aic_srv_voice_deinit(void);

/**
* aic_srv_voice_open - open voice service.
* Return: if success return 0, else return other value.
*/
int32_t aic_srv_voice_open(void);

/**
* aic_srv_voice_close - close voice service.
* Return: if success return 0, else return other value.
*/
int32_t aic_srv_voice_close(void);

/**
* get_voice_is_allowed - get voice is allowed.
* Return: ture: allowed, false: not allowed.
*/
bool get_voice_is_allowed(void);

/**
* aic_srv_voice_set_channel_type - The API of set voice channel type.
* @channel_type:  [IN] value of voice channel type
* Return: if success return 0, else return other value.
*/
int32_t aic_srv_voice_set_channel_type(tele_voice_channel_type_e channel_type);

/**
* aic_srv_voice_get_channel_type - The API of get voice channel type.
* Return:value of voice channel type
*/
tele_voice_channel_type_e aic_srv_voice_get_channel_type(void);


#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /* __AIC_SRV_VOICE_H__ */

