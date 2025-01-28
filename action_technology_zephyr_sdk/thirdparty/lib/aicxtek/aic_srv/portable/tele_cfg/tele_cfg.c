/******************************************************************************/
/*                                                                            */
/*    Copyright 2023 by AICXTEK TECHNOLOGIES CO.,LTD. All rights reserved.    */
/*                                                                            */
/******************************************************************************/

/**
 * @file tele_cfg.c
 *
 */
/*********************
 *      INCLUDES
 *********************/
#include "tele_cfg.h"

/**********************
 *  STATIC VARIABLES
 **********************/
/* The default tele srv type */
static tele_srv_type_e g_default_tele_srv_type = TELE_SRV_ENABLE_ON_XX;

/* The default tele voice channel type */
static tele_voice_channel_type_e g_default_voice_channel_type = TELE_VOICE_CHANNEL_TYPE_CMUX;

/* The default tele voice ringback type */
static tele_voice_local_ringback_type_e g_default_voice_local_ringback_type = TELE_VOICE_LOCAL_RINGBACK_TYPE_DEFAULT;

/* The default tele voice volume */
static uint32_t g_default_voice_volume = 15;

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
uint8_t tele_cfg_get_sim_slot_count(void)
{
    return 1;
}

/* TODO: Support the configuration of slot 1 or slot 2 for single card. */

tele_srv_type_e tele_cfg_get_tele_srv_type(void)
{
    return g_default_tele_srv_type;
}

tele_voice_channel_type_e tele_cfg_get_voice_channel_type(void)
{
    return g_default_voice_channel_type;
}

tele_voice_local_ringback_type_e tele_cfg_get_voice_local_ringback_type(void)
{
    return g_default_voice_local_ringback_type;
}

uint32_t tele_cfg_get_voice_volume(void)
{
    return g_default_voice_volume;
}

