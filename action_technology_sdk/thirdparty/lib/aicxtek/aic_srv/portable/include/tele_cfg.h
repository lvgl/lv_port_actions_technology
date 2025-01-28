/******************************************************************************/
/*                                                                            */
/*    Copyright 2023 by AICXTEK TECHNOLOGIES CO.,LTD. All rights reserved.    */
/*                                                                            */
/******************************************************************************/

/**
 *  DESCRIPTION
 *
 *    This file defines tele config.
 */

#ifndef __TELE_CFG_H__
#define __TELE_CFG_H__

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include <aic_type.h>

/**********************
 *      TYPEDEFS
 **********************/
typedef enum
{
    /* Enable tele service module. You can control tele function by tele service APIs. */
    TELE_SRV_ENABLE,
    /* Enable tele service module bypass. */
    TELE_SRV_ENABLE_BYPASS,
    /* Enable tele service module on XX */
    TELE_SRV_ENABLE_ON_XX,
    /* Disable tele service module and disable voice. */
    TELE_SRV_DISABLE_AND_VOICE_DISABLE,
    /* Disable tele service module and enable voice. */
    TELE_SRV_DISABLE_AND_VOICE_ENABLE,

    TELE_SRV_TYPE_MAX,
    /* Ensure takeup 4 bytes. */
    TELE_SRV_TYPE_END = 0xFFFFFFFF
} tele_srv_type_e;

typedef enum
{
    TELE_VOICE_CHANNEL_TYPE_CMUX,
    TELE_VOICE_CHANNEL_TYPE_IIS,

    TELE_VOICE_CHANNEL_TYPE_MAX,
    /* Ensure takeup 4 bytes. */
    TELE_VOICE_CHANNEL_TYPE_END = 0xFFFFFFFF
} tele_voice_channel_type_e;

typedef enum
{
    /* Default is 1s on, 4s off */
    TELE_VOICE_LOCAL_RINGBACK_TYPE_DEFAULT,
    /* No play local ringback voice */
    TELE_VOICE_LOCAL_RINGBACK_TYPE_NO_PLAY,
    /* Local ringback tone of ANSI */
    TELE_VOICE_LOCAL_RINGBACK_TYPE_ANSI,
    /* Local ringback tone of Japan */
    TELE_VOICE_LOCAL_RINGBACK_TYPE_JAPAN,
    /* Local ringback tone of GB */
    TELE_VOICE_LOCAL_RINGBACK_TYPE_GB,
    /* Local ringback tone of Australia */
    TELE_VOICE_LOCAL_RINGBACK_TYPE_AUSTRALIA,
    /* Local ringback tone of Singapore */
    TELE_VOICE_LOCAL_RINGBACK_TYPE_SINGAPORE,
    /* Local ringback tone of HongKong */
    TELE_VOICE_LOCAL_RINGBACK_TYPE_HONGKONG,
    /* Local ringback tone of Ireland */
    TELE_VOICE_LOCAL_RINGBACK_TYPE_IRELAND,
    /* Local ringback tone of Cept */
    TELE_VOICE_LOCAL_RINGBACK_TYPE_CEPT,

    TELE_VOICE_LOCAL_RINGBACK_TYPE_MAX,
    /* Ensure takeup 4 bytes. */
    TELE_VOICE_LOCAL_RINGBACK_TYPE_END = 0xFFFFFFFF
} tele_voice_local_ringback_type_e;

/**********************
 * GLOBAL PROTOTYPES
 **********************/
/**
* tele_cfg_get_sim_slot_count.
* Return: the sim slot count.
*/
uint8_t tele_cfg_get_sim_slot_count(void);

/**
* tele_cfg_get_tele_srv_type.
* Return: the tele service type.
*/
tele_srv_type_e tele_cfg_get_tele_srv_type(void);

/**
* tele_cfg_get_voice_channel_type.
* Return: the tele voice channel type.
*/
tele_voice_channel_type_e tele_cfg_get_voice_channel_type(void);

/**
* tele_cfg_get_voice_local_ringback_type.
* Return: the tele voice ringback type.
*/
tele_voice_local_ringback_type_e tele_cfg_get_voice_local_ringback_type(void);

/**
* tele_cfg_get_voice_volume.
* Return: the tele voice volume level(currently support 0~15).
*/
uint32_t tele_cfg_get_voice_volume(void);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /* __TELE_CFG_H__ */

