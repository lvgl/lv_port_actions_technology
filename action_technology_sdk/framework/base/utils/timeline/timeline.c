/********************************************************************************
 *                            USDK(ATS350B_linux)
 *                            Module: SYSTEM
 *                 Copyright(c) 2003-2017 Actions Semiconductor,
 *                            All Rights Reserved.
 *
 * History:
 *      <author>      <time>                      <version >          <desc>
 *      rongxing      2021-9-5                      1.0             build this file
 ********************************************************************************/
/*!
 * \file     timeline.c
 * \brief    
 * \author
 * \version  1.0
 * \date  2021-9-5
 *******************************************************************************/
 
#include <timeline.h>

#ifdef CONFIG_SYS_LOG
#define SYS_LOG_NO_NEWLINE
#ifdef SYS_LOG_DOMAIN
#undef SYS_LOG_DOMAIN
#endif
#define SYS_LOG_DOMAIN "timeline"
#include <logging/sys_log.h>
#endif

static sys_slist_t g_timeline_list = SYS_SLIST_STATIC_INIT(&g_timeline_list);
OS_MUTEX_DEFINE(timeline_lock);

timeline_t * timeline_create(int32_t type,int32_t interval_us){
	timeline_t * timeline = mem_malloc(sizeof(timeline_t));
	if (!timeline) {
		SYS_LOG_ERR("timeline %d malloc failed\n",type);
		return NULL;
	}
	timeline->type = type;
	timeline->status = TIMELINE_STATUS_PENDING;
	timeline->interval_us = interval_us;
	sys_slist_init(&timeline->listener_list);

	SYS_LOG_INF("%d %d\n",type,interval_us);
	os_mutex_lock(&timeline_lock, OS_FOREVER);
	sys_slist_append(&g_timeline_list, &timeline->node);
	os_mutex_unlock(&timeline_lock);
	return timeline;
}

static int timeline_is_validate(timeline_t *tl){
	timeline_t * p_tl = NULL;
	sys_snode_t *pnode;
	uint8_t validate = 0;

	if(!k_is_in_isr())
		os_mutex_lock(&timeline_lock, OS_FOREVER);

	SYS_SLIST_FOR_EACH_NODE(&g_timeline_list, pnode) {
		p_tl = CONTAINER_OF(pnode, timeline_t, node);
		if(p_tl == tl){
			validate = 1;
		}
	}

	if(!k_is_in_isr())
		os_mutex_unlock(&timeline_lock);

	if(!validate)
		SYS_LOG_INF("%p already release??\n",tl);
	return validate;
}

int timeline_start(timeline_t *tl){
	if(tl && timeline_is_validate(tl)){
		if(!k_is_in_isr())
			os_mutex_lock(&timeline_lock, OS_FOREVER);
		tl->status = TIMELINE_STATUS_RUNNING;
		if(!k_is_in_isr())
			os_mutex_unlock(&timeline_lock);
	}
	return 0;
}

int timeline_add_listener(timeline_t *tl,timeline_listener_t* listener)
{
	if(tl && timeline_is_validate(tl) && listener){
		SYS_LOG_INF("tl %p add %p\n",tl,listener);
		if(!k_is_in_isr())
	    	os_mutex_lock(&timeline_lock, OS_FOREVER);
		sys_slist_append(&tl->listener_list, &listener->node);
		if(!k_is_in_isr())
	    	os_mutex_unlock(&timeline_lock);
		return 0;
	}else
		return -EINVAL;
}

int timeline_remove_listener(timeline_t *tl,timeline_listener_t* listener){
	if(tl && timeline_is_validate(tl) && listener){
		SYS_LOG_INF("tl %p remove %p\n",tl,listener);
		if(!k_is_in_isr())
	    	os_mutex_lock(&timeline_lock, OS_FOREVER);
		sys_slist_find_and_remove(&tl->listener_list, &listener->node);
		if(!k_is_in_isr())
	    	os_mutex_unlock(&timeline_lock);
		return 0;
	}else
		return -EINVAL;
}

int timeline_trigger_listener(timeline_t *tl){
	timeline_listener_t *listener;
	sys_snode_t *pnode;

	
	if(tl && timeline_is_validate(tl) && tl->status == TIMELINE_STATUS_RUNNING){
		if(!k_is_in_isr())
	    	os_mutex_lock(&timeline_lock, OS_FOREVER);
		SYS_SLIST_FOR_EACH_NODE(&tl->listener_list, pnode) {
			listener = CONTAINER_OF(pnode, timeline_listener_t, node);
			listener->trigger(listener->param);
		}
		if(!k_is_in_isr())
	    	os_mutex_unlock(&timeline_lock);
		return 0;
	}else
		return -EINVAL;
}

int timeline_get_interval(timeline_t * tl){
	if(tl && timeline_is_validate(tl))
		return tl->interval_us;
	else
		return 0;
}

int timeline_stop(timeline_t *tl){
	if(tl && timeline_is_validate(tl)){
		if(!k_is_in_isr())
			os_mutex_lock(&timeline_lock, OS_FOREVER);
		tl->status = TIMELINE_STATUS_PENDING;
		if(!k_is_in_isr())
			os_mutex_unlock(&timeline_lock);
	}
	return 0;
}

int timeline_release(timeline_t * tl){
	if(tl && timeline_is_validate(tl)){
		SYS_LOG_INF("%p\n",tl);
		if(!k_is_in_isr())
			os_mutex_lock(&timeline_lock, OS_FOREVER);
		sys_slist_find_and_remove(&g_timeline_list, &tl->node);
		mem_free(tl);
		if(!k_is_in_isr())
			os_mutex_unlock(&timeline_lock);
	}
	return 0;
}

timeline_t * timeline_get_by_type(int32_t type){
	timeline_t * tl;
	sys_snode_t *pnode;
	
	if(!k_is_in_isr())
		os_mutex_lock(&timeline_lock, OS_FOREVER);
	SYS_SLIST_FOR_EACH_NODE(&g_timeline_list, pnode) {
		tl = CONTAINER_OF(pnode, timeline_t, node);
		if(tl->type == type){
			if(!k_is_in_isr())
				os_mutex_unlock(&timeline_lock);
			return tl;
		}
	}
	if(!k_is_in_isr())
		os_mutex_unlock(&timeline_lock);
	return NULL;
}

