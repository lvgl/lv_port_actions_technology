/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file esd manager interface
 */

#ifndef __EDS_MANGER_H__
#define __EDS_MANGER_H__

/**
 * @defgroup esd_manager_apis esd manager APIs
 * @ingroup system_apis
 * @{
 */

/** esd device link state */
typedef enum
{
	/** esd device link single state */
	DEVICE_STATE_SINGLE_LINK = 1,
	/** esd device link double state */
    DEVICE_STATE_DOUBLE_LINK = 2,
	/** esd device link tws state */
    DEVICE_STATE_TWS = 4,
}esd_device_state_e;

/** esd tag info */
typedef enum
{
	/** tag of app */
    TAG_APP_ID,
	/** tag of play state */
    TAG_PLAY_STATE,
	/** tag of device state */
	TAG_DEVICE_STATE,
	/** tag of music bp info */
	TAG_BP_INFO,
	/** tag of volume info */
	TAG_VOLUME,
}esd_tag_info_e;

/**
 *
 * @brief init esd manager
 *
 * @details init the esd manager memory and manager struct stat
 *
 * @return N/A
 */
void esd_manager_init(void);
/**
 *
 * @brief check esd manager state
 *
 * @details check esd manager state, reset by esd or not reset by esd
 *
 * @return true current reset because of esd
 * @return false current reset not because of esd
 */
bool esd_manager_check_esd(void);
/**
 *
 * @brief reset esd manager
 *
 * @details reset esd manager state to init state,
 *  state saved before will be clear by this function 
 *
 * @return N/A
 */
void esd_manager_reset_finished(void);
/**
 *
 * @brief save current scene to esd mananger 
 * @param tag tag of esd tag info 
 * @param value valu of tag witch need to save
 * @param len max length of value.
 *
 * @return 0 success.
 * @return others failed.
 */
int esd_manager_save_scene(int tag, uint8_t *value , int len);
/**
 *
 * @brief restore current scene from esd mananger 
 * @param tag tag of esd tag info 
 * @param value valu of tag find from esd manager
 * @param len max length of value.
 *
 * @return 0 success.
 * @return others failed.
 */
int esd_manager_restore_scene(int tag, uint8_t *value , int len);
/**
 *
 * @brief deinit esd manager
 *
 * @details clear esd manager state and release manager memory
 *
 * @return N/A
 */
void esd_manager_deinit(void);

/**
 * @} end defgroup esd_manager_apis
 */
#endif