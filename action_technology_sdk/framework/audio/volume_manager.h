/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file volume manager interface
 */

#ifndef __VOLUME_MANGER_H__
#define __VOLUME_MANGER_H__

typedef enum
{
    SYNC_TYPE_TWS,

    SYNC_TYPE_AVRCP,
    SYNC_TYPE_HFP,
    SYNC_TYPE_ROLE_SWITCH,    
}sync_type_e;

/**
 * @defgroup volume_manager_apis App volume Manager APIs
 * @{
 */

/**
 * @brief get system volume value
 *
 * This routine calls get system volume ,can't called by ISR.
 *
 * @return current system volume
 */
int system_volume_get(int stream_type);

/**
 * @brief set system volume value
 *
 * This routine calls set system volume ,can't called by ISR.
 * @param stream_type volume of stream type
 * @param volume the volume level user want to set, rang is 0-40
 * @param display display or not display volume
 *
 * @return 0 set success
 * @return others set failed
 */
int system_volume_set(int stream_type, int volume, bool display);

/**
 * @brief set system volume down decrement
 *
 * This routine calls set system volume down decrment , can't called by ISR.
 * @param decrement the volume level user want to down, rang is 0-40
 *
 *
 * @return 0 set success
 * @return others set failed
 */
int system_volume_down(int stream_type, int decrement);

/**
 * @brief set system volume up increment
 *
 * This routine calls set system volume up increment , can't called by ISR.
 * @param decrement the volume level user want to up,rang is 0-40
 *
 *
 * @return 0 set success
 * @return others set failed
 */

int system_volume_up(int stream_type, int increment);


/**
 * @brief remote volume sync 
 *
 * This routine calls remote volume sync 
 *
 *
 * @return N/A
 */
void system_volume_sync_remote_to_device(uint32_t stream_type, uint32_t volume);

/**
 * @} end defgroup volume_manager_apis
 */

#endif