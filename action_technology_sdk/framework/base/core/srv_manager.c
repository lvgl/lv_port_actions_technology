/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file service manager interface
 */
#define SYS_LOG_DOMAIN "srv_manager"

#include <os_common_api.h>
#include <app_manager.h>
#include <srv_manager.h>
#include <msg_manager.h>
#include <assert.h>
#include <string.h>

#include "srv_manager_inner.h"

OS_MUTEX_DEFINE(srv_manager_mutex);

static sys_slist_t	global_srv_list;

extern struct service_entry_t __service_entry_table[];

extern struct service_entry_t __service_entry_end[];

static struct service_entry_t *get_srv_entry_byname(char *srv_name)
{
	struct service_entry_t *srv_entry;

	for (srv_entry = __service_entry_table;
	     srv_entry < __service_entry_end;
	     srv_entry++) {

		if (!strcmp(srv_entry->name, srv_name)) {
			return srv_entry;
		}
	}
	return NULL;
}

static struct service_info *srv_manager_get_service_info(char *srv_name)
{
	sys_snode_t *node, *tmp;

	SYS_SLIST_FOR_EACH_NODE_SAFE(&global_srv_list, node, tmp) {
		struct service_info *srv = SRV_INFO(node);

		if (!strcmp(srv->name, srv_name)) {
			return srv;
		}
	}
	return NULL;
}

static bool srv_manager_check_stack_safe(struct service_info *srvinfo)
{
	sys_snode_t *node, *tmp;

	SYS_SLIST_FOR_EACH_NODE_SAFE(&global_srv_list, node, tmp) {

		struct service_info *srv = SRV_INFO(node);

		if (srv->stackptr == srvinfo->stackptr) {
			return false;
		}

	}
	return true;
}

static bool srv_manager_init_service(struct service_entry_t *srv)
{
	struct service_info *srvinfo =
						(struct service_info *)mem_malloc(sizeof(struct service_info));

	if (!srvinfo)
		return false;

	srvinfo->entry = srv;

	srvinfo->name = srv->name;

	srvinfo->stackptr = srv->stack;

	if (!srv_manager_check_stack_safe(srvinfo)) {
		SYS_LOG_INF("stack not safe\n");
		goto exit_failed;
	}

	srvinfo->tid = (os_tid_t)os_thread_create(srv->stack,
									srv->stack_size,
									srv->thread_loop,
									srv->p1, srv->p2, srv->p3,
									-CONFIG_NUM_COOP_PRIORITIES,
									0,
									0);

	if (srvinfo->tid != NULL) {
		os_yield();

		os_thread_priority_set(srvinfo->tid, srv->priority);
		os_thread_name_set(srvinfo->tid, srv->name);

		sys_slist_append(&global_srv_list, (sys_snode_t *)srvinfo);

		if (!msg_manager_add_listener(srvinfo->name, srvinfo->tid)) {
			SYS_LOG_ERR(" %s add listener failed\n",
							srvinfo->name);
			goto exit_failed;
		}
	} else {
		SYS_LOG_ERR(" srv %s thread spawn failed\n", srv->name);
		goto exit_failed;
	}

	return true;

exit_failed:
	mem_free(srvinfo);
	return false;
}
static void _srv_exit_callback(struct app_msg *msg, int result, void *reply)
{
	os_sem *callback_sem = msg->ptr;

	if (callback_sem != NULL) {
		os_sem_give(callback_sem);
	}
}

void *srv_manager_get_servicetid(char *srv_name)
{
	struct service_info *srvinfo = srv_manager_get_service_info(srv_name);

	if (srvinfo != NULL) {
		return srvinfo->tid;
	}

	return NULL;
}

bool srv_manager_thread_exit(char *srv_name)
{
	bool result = false;
	struct app_msg msg = {0};
	struct service_info *srvinfo;

	os_thread_priority_set(os_current_get(), -CONFIG_NUM_COOP_PRIORITIES);

	os_mutex_lock(&srv_manager_mutex, OS_FOREVER);

	srvinfo = srv_manager_get_service_info(srv_name);
	if (srvinfo == NULL) {
		SYS_LOG_ERR(" %s not found\n", srv_name);
		goto exit;
	}

	/*clear all remain message */
	while (receive_msg(&msg, 0)) {
		if (msg.callback != NULL) {
			/* msg.callback(&msg, 0, NULL); */
		}
	}

	sys_slist_find_and_remove(&global_srv_list, (sys_snode_t *)srvinfo);

	if (!msg_manager_remove_listener(srvinfo->name)) {
		SYS_LOG_ERR(" %s remove listener failed\n", srvinfo->name);
		goto exit;
	}

	result = true;

	SYS_LOG_INF(" %s ok\n", srvinfo->name);

	mem_free(srvinfo);
exit:
	os_mutex_unlock(&srv_manager_mutex);
	return result;
}

bool srv_manager_exit_service(char *srv_name)
{
	bool result = true;
	struct app_msg msg = {0};
	os_sem callback_sem;
	struct service_info *srvinfo = NULL;
	int tid = 0;

	srvinfo = srv_manager_get_service_info(srv_name);

	if (srvinfo == NULL) {
		result = false;
		goto exit;
	}

	os_sem_init(&callback_sem, 0, 1);

	msg.type = MSG_EXIT_APP;
	msg.callback = _srv_exit_callback;
	msg.ptr = &callback_sem;
	tid = (int)srvinfo->tid;
	os_thread_prepare_terminal(tid);
	os_sched_lock();



	if (!send_async_msg(srv_name, &msg)) {
		result = false;
		os_sched_unlock();
		goto exit;
	}

	/*
	 *set the exit app to the highest priority. avoid the app can't exit
	 */
	os_thread_priority_set(srvinfo->tid, -CONFIG_NUM_COOP_PRIORITIES);

	os_sched_unlock();

wait:

	if (os_sem_take(&callback_sem, 6000) != 0) {
		SYS_LOG_WRN("srv %s exit timout ", srvinfo->name);
		if (srv_manager_get_servicetid(srv_name) != NULL) {
			goto wait;
		}
	}

	os_thread_wait_terminal(tid);
exit:

	return result;
}

bool srv_manager_active_service(char *srv_name)
{
	bool result = true;
	struct service_entry_t *target_srv_entry;

	target_srv_entry = get_srv_entry_byname(srv_name);

	if (!target_srv_entry) {
		SYS_LOG_ERR("app not define\n");
		return false;
	}

	os_mutex_lock(&srv_manager_mutex, OS_FOREVER);

	if (!srv_manager_init_service(target_srv_entry)) {
		SYS_LOG_ERR("int failed\n");
		result = false;
		goto exit;
	}

exit:
	os_mutex_unlock(&srv_manager_mutex);
	return result;
}

bool srv_manager_check_service_is_actived(char *srv_name)
{
	bool result = false;

	os_mutex_lock(&srv_manager_mutex, OS_FOREVER);

	if (srv_manager_get_service_info(srv_name) != NULL) {
		result = true;
	}

	os_mutex_unlock(&srv_manager_mutex);
	return result;

}
static void _srv_notify_callback(struct app_msg *msg, int result, void *reply)
{
	os_sem *callback_sem = msg->ptr;

	if (callback_sem) {
		os_sem_give(callback_sem);
	}
}

void srv_manager_notify_service(char *srv_name, int msg_type)
{
	sys_snode_t *node, *tmp;

	SYS_SLIST_FOR_EACH_NODE_SAFE(&global_srv_list, node, tmp) {
		struct service_info *srv = SRV_INFO(node);
		if (srv && (!srv_name || !strcmp(srv->name, srv_name))) {
			struct app_msg msg = {0};

			os_sem callback_sem;
			os_sem_init(&callback_sem, 0, 1);
			msg.type = msg_type;
			msg.callback = _srv_notify_callback;
			msg.ptr = &callback_sem;

			if (!send_async_msg(srv->name, &msg)) {
				return;
			}

			if (os_sem_take(&callback_sem, OS_FOREVER) != 0) {
				SYS_LOG_ERR("srv %s exit timout", srv->name);
				assert(false);
			}
		}
	}
}

bool srv_manager_init(void)
{
	sys_slist_init(&global_srv_list);
	return true;
}


