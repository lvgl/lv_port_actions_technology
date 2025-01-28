/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief alarm app main.
 */

#include "alarm.h"
#include "tts_manager.h"

#ifdef CONFIG_PROPERTY
#include "property_manager.h"
#endif
#include <os_common_api.h>

static struct alarm_app_t *p_alarm = NULL;
os_delayed_work alarm_callback_work;
static void _alarm_isr_work(struct k_work *work);
OS_DELAY_WORK_DEFINE(alarm_callback_work, _alarm_isr_work);

void alarm_event_cb(void)
{
	SYS_LOG_INF("\n");
	os_delayed_work_submit(&alarm_callback_work, 0);
}

void alarm_start_msg_send(void)
{
	struct app_msg msg = { 0 };

	msg.type = MSG_ALARM_EVENT;
	msg.cmd= MSG_ALARM_PROC_START;
	SYS_LOG_INF("start by alarm\n");
	send_async_msg(app_manager_get_current_app(), &msg);
}

void alarm_exit_msg_send(void)
{
	struct app_msg msg = { 0 };

	msg.type = MSG_ALARM_EVENT;
	msg.cmd= MSG_ALARM_PROC_EXIT;
	SYS_LOG_INF("exit alarm\n");
	send_async_msg(app_manager_get_current_app(), &msg);
}

static void _alarm_isr_work(struct k_work *work)
{
#ifdef CONFIG_LAUNCHER_APP
	alarm_start_msg_send();
#else
	struct app_msg msg = { 0 };
	if (memcmp(app_manager_get_current_app(), APP_ID_ALARM, strlen(APP_ID_ALARM)) == 0) {
		SYS_LOG_INF("prev alarm is playing\n");
	} else if (memcmp(app_manager_get_current_app(), APP_ID_BTCALL, strlen(APP_ID_BTCALL)) == 0) {
		SYS_LOG_INF("in btcall\n");
	} else {
		if (p_alarm) {/*exit alarm display*/
			msg.type = MSG_INPUT_EVENT;
			msg.cmd = MSG_ALARM_ENTRY_EXIT;
			msg.reserve = 0x01;
			send_async_msg(APP_ID_MAIN, &msg);
		}
		msg.type = MSG_START_APP;
		msg.ptr = APP_ID_ALARM;
		msg.reserve = APP_SWITCH_CURR;
		send_async_msg(APP_ID_MAIN, &msg);

		SYS_LOG_INF("start by alarm\n");
	}
#endif
}
static int _alarm_init(void)
{
	if (p_alarm)
		return 0;

	p_alarm = app_mem_malloc(sizeof(struct alarm_app_t));
	if (!p_alarm) {
		SYS_LOG_ERR("malloc failed!\n");
		return -ENOMEM;
	}

	memset(p_alarm, 0, sizeof(struct alarm_app_t));
#ifdef CONFIG_UI_MANAGER
	alarm_view_init();
#endif
	alarm_manager_init();

	alarm_thread_timer_init();

	//system_registry_alarm_callback(alarm_event_cb);

	//os_delayed_work_init(&alarm_callback_work, _alarm_isr_work);

	SYS_LOG_INF("ok\n");
	return 0;
}

void alarm_exit(bool need_switch_last)
{
	if (!p_alarm)
		goto exit;
#ifdef CONFIG_THREAD_TIMER
	if (thread_timer_is_running(&p_alarm->monitor_timer))
		thread_timer_stop(&p_alarm->monitor_timer);

	if (thread_timer_is_running(&p_alarm->play_timer))
		thread_timer_stop(&p_alarm->play_timer);
#endif
	alarm_ring_stop(p_alarm, false);
#ifdef CONFIG_LOCAL_PLAYER
	alarm_exit_iterator();
#endif
#ifdef CONFIG_UI_MANAGER
	alarm_view_deinit();
#endif
	app_mem_free(p_alarm);

	p_alarm = NULL;

#ifdef CONFIG_PROPERTY
	property_flush(NULL);
#endif

	if (alarm_wakeup_source_check()) {
#ifndef CONFIG_ACTS_POWER_ONOFF_TEST
		find_and_set_alarm();
		sys_event_send_message(MSG_POWER_OFF);
#endif
	}

exit:
	find_and_set_alarm();

	if (memcmp(app_manager_get_current_app(), APP_ID_ALARM, strlen(APP_ID_ALARM)) == 0) {
		app_manager_thread_exit(APP_ID_ALARM);
	} else {
	#ifdef CONFIG_LAUNCHER_APP
		if (need_switch_last)
			alarm_switch_to_last();
	#endif
	}
	SYS_LOG_INF("ok\n");
}
struct alarm_app_t *alarm_get_app(void)
{
	return p_alarm;
}

void alarm_need_exit(void)
{
	if (memcmp(app_manager_get_current_app(), APP_ID_ALARM, strlen(APP_ID_ALARM)) && p_alarm) {
		struct app_msg msg = { 0 };

		msg.type = MSG_INPUT_EVENT;
		msg.cmd = MSG_ALARM_ENTRY_EXIT;
		msg.reserve = 0x01;
		send_async_msg(APP_ID_MAIN, &msg);
	}
}
void alarm_entry_exit(uint8_t force_exit)
{
	struct app_msg msg = { 0 };
	SYS_LOG_INF("force_exit=%d, p_alarm= %p\n", force_exit, p_alarm);

	if (memcmp(app_manager_get_current_app(), APP_ID_ALARM, strlen(APP_ID_ALARM)) == 0) {
		/*TBD KEY_TYPE_DOUBLE_CLICK not exit app,when alarm playing*/
		return;
	} else {
		if (p_alarm) {
			alarm_exit(true);
			return;
		} else {
			if (force_exit || (memcmp(app_manager_get_current_app(), APP_ID_BTCALL, strlen(APP_ID_BTCALL)) == 0))
				return;
			_alarm_init();
		}
		msg.cmd = MSG_ALARM_CHANGE_MODE;
		msg.type = MSG_INPUT_EVENT;
		msg.reserve = ALARM_PLAYER;
		send_async_msg(APP_ID_MAIN, &msg);
	}
}

static bool _alarm_disk_ready_check(void)
{
	if (fs_manager_get_volume_state("/USB:")) {
		strncpy(p_alarm->dir, "/USB:/", sizeof(p_alarm->dir));
	} else if (fs_manager_get_volume_state("/SD:")) {
		strncpy(p_alarm->dir, "/SD:/", sizeof(p_alarm->dir));
	} else if (fs_manager_get_volume_state("/NAND:")) {
		strncpy(p_alarm->dir, "/NAND:/", sizeof(p_alarm->dir));
	} else if (fs_manager_get_volume_state("/NOR:")) {
		strncpy(p_alarm->dir, "/NOR:/", sizeof(p_alarm->dir));
	} else {
#ifdef CONFIG_DISK_MUSIC_APP
		strncpy(p_alarm->dir, "/NOR:/", sizeof(p_alarm->dir));
#else
		alarm_disk_check_timer_start(p_alarm);
		return false;
#endif
	}
	return true;
}
#ifdef CONFIG_LAUNCHER_APP
void alarm_proc_start(void)
{
	struct app_msg msg = { 0 };

	SYS_LOG_INF("p_alarm %p\n", p_alarm);

	if (p_alarm)
		return;

	if (_alarm_init()) {
		alarm_exit(true);
		SYS_LOG_ERR("init failed\n");
		return;
	}

	if (_alarm_disk_ready_check()) {
		msg.type = MSG_ALARM_EVENT;
		msg.cmd = MSG_ALARM_RING_PLAY_INIT;
		send_async_msg(app_manager_get_current_app(), &msg);
	}
}
extern uint8_t alarm_snooze_times;
void alarm_snooze(bool need_switch_last)
{
	if (p_alarm == NULL) {
		p_alarm = app_mem_malloc(sizeof(struct alarm_app_t));
		if (!p_alarm) {
			SYS_LOG_ERR("malloc failed!\n");
			return;
		}
	}
	if (alarm_snooze_times < ALARM_SNOOSE_TIMS) {
		alarm_snooze_times++;
		alarm_set_snooze(p_alarm, ALARM_SNOOSE_TIMEOUT);
	}
	alarm_exit(need_switch_last);
}
#endif

static void _alarm_main_loop(void *parama1, void *parama2, void *parama3)
{
	struct app_msg msg = { 0 };
	bool terminated = false;

	if (_alarm_init()) {
		alarm_exit(false);
		SYS_LOG_ERR("init failed\n");
		return;
	}

	if (_alarm_disk_ready_check()) {
		msg.type = MSG_ALARM_EVENT;
		msg.cmd = MSG_ALARM_RING_PLAY_INIT;
		send_async_msg(APP_ID_ALARM, &msg);
	}
	app_switch_lock(1);

	while (!terminated) {
		if (receive_msg(&msg, thread_timer_next_timeout())) {
			SYS_LOG_INF("type %d, cmd %d\n", msg.type, msg.cmd);
			switch (msg.type) {
			case MSG_INPUT_EVENT:
				alarm_input_event_proc(&msg);
			break;
			case MSG_ALARM_EVENT:
				alarm_event_proc(&msg);
				break;

			case MSG_TTS_EVENT:
				alarm_tts_event_proc(&msg);
				break;

			case MSG_EXIT_APP:
				alarm_exit(false);
				terminated = true;
				break;
			default:
				break;
			}
			if (msg.callback)
				msg.callback(&msg, 0, NULL);
		}
		if (!terminated)
			thread_timer_handle_expired();
	}
	if (alarm_wakeup_source_check()) {
		sys_event_send_message(MSG_POWER_OFF);
	}
}

APP_DEFINE(alarm, share_stack_area, sizeof(share_stack_area),
	   CONFIG_APP_PRIORITY, FOREGROUND_APP, NULL, NULL, NULL,
	   _alarm_main_loop, NULL);

