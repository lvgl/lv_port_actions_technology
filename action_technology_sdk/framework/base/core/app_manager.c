/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file app manager interface
 */

#define SYS_LOG_DOMAIN "app_manager"

#include <os_common_api.h>
#include <srv_manager.h>
#include <app_manager.h>
#include <assert.h>
#include <msg_manager.h>
#include <string.h>


extern struct app_entry_t __app_entry_table[];
extern struct app_entry_t __app_entry_end[];

#define APP_INFO(_node) CONTAINER_OF(_node, struct app_info, node)


struct app_info {
	sys_snode_t node;     /* used for app list*/
	char *name;
	os_tid_t tid;          /* app thread id*/
	char *stackptr;
	struct app_entry_t *entry;
};


OS_MUTEX_DEFINE(app_manager_mutex);

static sys_slist_t	global_app_list;

static struct app_entry_t *actived_app = NULL;
static struct app_entry_t *prev_app = NULL;
static struct app_entry_t *default_app = NULL;
static bool exit_to_default = false;

static struct app_info *app_manager_get_app_info(char *app_name)
{
	sys_snode_t *node, *tmp;

	SYS_SLIST_FOR_EACH_NODE_SAFE(&global_app_list, node, tmp) {

		struct app_info *app = APP_INFO(node);

		if (!strcmp(app->name, app_name))	{
			return app;
		}

	}
	return NULL;
}

static bool app_manager_check_stack_safe(struct app_info *appinfo)
{
	sys_snode_t *node, *tmp;

	SYS_SLIST_FOR_EACH_NODE_SAFE(&global_app_list, node, tmp) {

		struct app_info *app = APP_INFO(node);

		if (app->stackptr == appinfo->stackptr) {
			SYS_LOG_ERR("app->name %s used stack %p ", app->name, appinfo->stackptr);
			return false;
		}

	}
	return true;
}

static void _app_notify_callback(struct app_msg *msg, int result, void *reply)
{
	os_sem *callback_sem = msg->ptr;

	if (callback_sem) {
		os_sem_give(callback_sem);
	}
}

static struct app_entry_t *get_app_entry_byname(char *app_name)
{
	struct app_entry_t *app_entry;

	if (app_name) {
		for (app_entry = __app_entry_table;
		     app_entry < __app_entry_end;
		     app_entry++) {
			if (!strcmp(app_entry->name, app_name)) {
				return app_entry;
			}
		}
	}

	return NULL;
}

static bool app_manager_init_app(struct app_entry_t *app, bool create_thread)
{
	struct app_info *appinfo = NULL;
	int prio = os_thread_priority_get(os_current_get());

	if ((app->attribute & DEFAULT_APP) == DEFAULT_APP) {
		if (default_app != NULL && default_app != app) {
			SYS_LOG_ERR("default app already set %s\n"
					, app->name);
			return false;
		}
	}

	appinfo = mem_malloc(sizeof(struct app_info));

	if (!appinfo)
		return false;

	appinfo->entry = app;

	appinfo->name = app->name;
	appinfo->stackptr = app->stack;

	if (!app_manager_check_stack_safe(appinfo)) {
		SYS_LOG_INF("stack not safe\n");

		goto exit_failed;
	}

	if (create_thread) {
		if (!app->thread_loop) {
			SYS_LOG_ERR("%s thread loop is NULL\n", app->name);
			goto exit_failed;
		}
		os_thread_priority_set(os_current_get(), app->priority - 1);

		appinfo->tid = (os_tid_t)os_thread_create(app->stack,
									app->stack_size,
									app->thread_loop,
									app->p1, app->p2, app->p3,
									app->priority,
									0,
									0);

		os_thread_name_set(appinfo->tid, app->name);

	} else {
		appinfo->tid = os_current_get();
	}

	if (!appinfo->tid) {
		SYS_LOG_ERR(" %s thread spawn failed\n", app->name);
		goto exit_failed;
	}

	sys_slist_append(&global_app_list, (sys_snode_t *)appinfo);

	if (!msg_manager_add_listener(appinfo->name, appinfo->tid)) {
		SYS_LOG_ERR("%s add listener failed\n", appinfo->name);
		goto exit_failed;
	}

	if ((app->attribute & DEFAULT_APP) == DEFAULT_APP
		 && default_app == NULL) {
		SYS_LOG_INF(" app %s is default app\n", app->name);
		default_app = app;
	}
	os_thread_priority_set(os_current_get(), prio);
	return true;

exit_failed:
	mem_free(appinfo);
	os_thread_priority_set(os_current_get(), prio);
	return false;
}

void *app_manager_get_apptid(char *app_name)
{
	struct app_info *appinfo = app_manager_get_app_info(app_name);

	if (appinfo) {
		return appinfo->tid;
	}

	return NULL;
}

void *app_manager_get_current_app(void)
{
	void *result = NULL;

	os_mutex_lock(&app_manager_mutex, OS_FOREVER);

	if (actived_app != NULL) {
		result = (void *)actived_app->name;
	}
	os_mutex_unlock(&app_manager_mutex);

	return result;
}
void *app_manager_get_prev_app(void)
{
	void *result = NULL;

	os_mutex_lock(&app_manager_mutex, OS_FOREVER);

	if (prev_app != NULL) {
		result = (void *)prev_app->name;
	}

	os_mutex_unlock(&app_manager_mutex);

	return result;
}
bool app_manager_thread_exit(char *app_name)
{
	bool result = false;
	struct app_msg msg = {0};
	struct app_info *appinfo = app_manager_get_app_info(app_name);

	if (!appinfo) {
		SYS_LOG_ERR(" %s not found\n", app_name);
		return result;
	}

	os_thread_priority_set(appinfo->tid, -CONFIG_NUM_COOP_PRIORITIES);

	os_mutex_lock(&app_manager_mutex, OS_FOREVER);

	/*clear all remain message */
	while (receive_msg(&msg, 0)) {
		if (msg.callback) {
			msg.callback(&msg, 0, NULL);
		}
	}

	sys_slist_find_and_remove(&global_app_list, (sys_snode_t *)appinfo);

	if (!msg_manager_remove_listener(appinfo->name)) {
		SYS_LOG_ERR("%s remove listener failed\n", appinfo->name);
		goto exit;
	}

	result = true;

	if (!strcmp(actived_app->name, appinfo->name)) {
		prev_app = actived_app;
		actived_app = NULL;

		if (default_app != NULL
			&& strcmp(default_app->name, appinfo->name)
			&& exit_to_default) {
			struct app_msg msg = {0};

			msg.type = MSG_START_APP;
			msg.ptr = default_app->name;
			send_async_msg("main", &msg);
		}
	}

	mem_free(appinfo);

	SYS_LOG_INF(" %s success\n", app_name);
exit:
	os_mutex_unlock(&app_manager_mutex);
	return result;
}

bool app_manager_exit_app(char *app_name, bool to_default)
{
	int tid = 0;
	struct app_msg msg = {0};
	os_sem callback_sem;
	struct app_info *appinfo = app_manager_get_app_info(app_name);

	if (!appinfo) {
		return true;
	}

	exit_to_default = to_default;

	os_sem_init(&callback_sem, 0, 1);

	msg.type = MSG_EXIT_APP;
	msg.callback = _app_notify_callback;
	msg.ptr = &callback_sem;
	tid = (int)appinfo->tid;
	os_thread_prepare_terminal(tid);
	os_sched_lock();

	if (!send_async_msg(app_name, &msg)) {
		os_sched_unlock();
		return false;
	}

	/*
	 *set the exit app to the highest priority. avoid the app can't exit
	 */
	//os_thread_priority_set(appinfo->tid, -CONFIG_NUM_COOP_PRIORITIES);

	os_sched_unlock();

wait:
	if (os_sem_take(&callback_sem, 6000) != 0) {
		SYS_LOG_WRN("app %s exit timout ", appinfo->name);
		if (app_manager_get_apptid(app_name)) {
			goto wait;
		}
	}

	os_thread_wait_terminal(tid);

	return true;
}

bool app_manager_active_app(char *app_name)
{
	bool result = true;
	struct app_entry_t *target_app_entry = NULL;

	target_app_entry = get_app_entry_byname(app_name);

	os_mutex_lock(&app_manager_mutex, OS_FOREVER);

	/* do nothing if current actived app is we need app*/
	if (target_app_entry == actived_app)	{
		SYS_LOG_INF(" current app is you need\n");
		goto exit;
	}

	/* if actived_app_id is not main ,we must exit current app first*/
	if ((target_app_entry == NULL ||
		(target_app_entry->attribute & APP_ATRTRIBUTE_MASK) == FOREGROUND_APP)
		&& actived_app != NULL){
		os_mutex_unlock(&app_manager_mutex);
		app_manager_exit_app(actived_app->name, false);
		os_mutex_lock(&app_manager_mutex, OS_FOREVER);
	}

	if (!target_app_entry) {
		if (app_name) {
			SYS_LOG_ERR("app not defined\n");
			result = false;
		}
		goto exit;
	}

	if (!app_manager_init_app(target_app_entry, true)) {
		SYS_LOG_ERR("app init failed\n");
		result = false;
		goto exit;
	}

	if ((target_app_entry->attribute & APP_ATRTRIBUTE_MASK) == FOREGROUND_APP) {
		actived_app = target_app_entry;
	}

exit:
	os_mutex_unlock(&app_manager_mutex);
	return result;

}

void app_manager_notify_app(char *app_name, int msg_type)
{
	sys_snode_t *node, *tmp;

	SYS_SLIST_FOR_EACH_NODE_SAFE(&global_app_list, node, tmp) {
		struct app_info *app = APP_INFO(node);
		if (app && (!app_name || !strcmp(app->name, app_name))) {
			struct app_msg msg = {0};

			os_sem callback_sem;
			os_sem_init(&callback_sem, 0, 1);
			msg.type = msg_type;
			msg.callback = _app_notify_callback;
			msg.ptr = &callback_sem;

			if (!send_async_msg(app->name, &msg)) {
				return;
			}

			if (os_sem_take(&callback_sem, OS_FOREVER) != 0) {
				SYS_LOG_ERR("app %s exit timout", app->name);
				assert(false);
			}
		}
	}
}

bool app_manager_key_event_dispatch(int key_event, int event_stage)
{
	bool result = false;
	struct app_entry_t *app_entry;

	for (app_entry = __app_entry_table;
		     app_entry < __app_entry_end;
		     app_entry++) {
		if (app_entry->input_event_handle != NULL) {
			result = app_entry->input_event_handle(key_event, event_stage);
			if (result) {
				break;
			}
		}
	}
	return result;
}

bool app_manager_init(void)
{
	struct app_entry_t *target_app_entry;

	sys_slist_init(&global_app_list);

	target_app_entry = get_app_entry_byname("main");

	if (!target_app_entry)
		return false;

	if (!app_manager_init_app(target_app_entry, false)) {
		SYS_LOG_ERR("app init failed\n");
		return false;
	}

	return true;
}
