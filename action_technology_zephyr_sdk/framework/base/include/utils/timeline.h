/********************************************************************************
 *                            USDK(ATS350B_linux)
 *                            Module: SYSTEM
 *                 Copyright(c) 2003-2017 Actions Semiconductor,
 *                            All Rights Reserved.
 *
 * History:
 *      <author>      <time>                      <version >          <desc>
 *      rongxing      2021-9-5                       1.0             build this file
 ********************************************************************************/
/*!
 * \file     timeline.h
 * \brief
 * \author
 * \version  1.0
 * \date  2021-9-5
 *******************************************************************************/

#ifndef TIMELINE_H_
#define TIMELINE_H_

#include <os_common_api.h>
#include <mem_manager.h>

enum{
	TIMELINE_TYPE_BLUETOOTH_SCO,
	TIMELINE_TYPE_BLUETOOTH_LE_TX,
	TIMELINE_TYPE_BLUETOOTH_LE_RX,
	TIMELINE_TYPE_AUDIO_TX,
	TIMELINE_TYPE_AUDIO_RX,
	TIMELINE_TYPE_MEDIA_TIMER,
	TIMELINE_TYPE_MEDIA_ENCODE,
	TIMELINE_TYPE_MEDIA_DECODE,
};

enum{
	TIMELINE_STATUS_PENDING,
	TIMELINE_STATUS_RUNNING,	
};

typedef struct
{
	sys_snode_t node;
	int32_t type;
	int32_t status;
    sys_slist_t listener_list;
	int32_t interval_us;
}timeline_t;

typedef struct
{
    sys_snode_t node;
	int (*trigger)(void *param);
	void* param;
}timeline_listener_t;

timeline_t * timeline_create(int32_t type,int32_t interval_us);

int timeline_start(timeline_t *tl);

int timeline_add_listener(timeline_t *tl,timeline_listener_t* listener);

int timeline_remove_listener(timeline_t *tl,timeline_listener_t* listener);

int timeline_trigger_listener(timeline_t *tl);

int timeline_get_interval(timeline_t * tl);

int timeline_stop(timeline_t *tl);

int timeline_release(timeline_t * tl);

timeline_t * timeline_get_by_type(int32_t type);

#endif /* TIMELINE_H_ */
