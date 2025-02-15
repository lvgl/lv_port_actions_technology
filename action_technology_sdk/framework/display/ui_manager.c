/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file ui manager interface
 */

#define LOG_MODULE_CUSTOMER

#include <os_common_api.h>
#include <msg_manager.h>
#include <mem_manager.h>
#include <srv_manager.h>
#include <ui_mem.h>
#include <string.h>
#include <ui_manager.h>

LOG_MODULE_REGISTER(ui_manager, LOG_LEVEL_INF);

#ifdef CONFIG_UI_SERVICE
#include <ui_service.h>
#endif

#ifdef CONFIG_SEG_LED_MANAGER
#include <seg_led_manager.h>
#endif

#ifdef CONFIG_LED_MANAGER
#include <led_manager.h>
#endif

#ifdef CONFIG_UI_SERVICE

typedef struct {
	os_sem sem;
	MSG_CALLBAK cb;
} uisrv_msg_notify_t;

#ifndef CONFIG_SIMULATOR
extern void _ui_service_main_loop(void *parama1, void *parama2, void *parama3);

__aligned(ARCH_STACK_PTR_ALIGN) __in_section_unique(uisrv.noinit.stack)
static char uisrv_stack_area[CONFIG_UISRV_STACKSIZE];

SERVICE_DEFINE(ui_service, uisrv_stack_area, sizeof(uisrv_stack_area), \
		CONFIG_UISRV_PRIORITY, BACKGROUND_APP, NULL, NULL, NULL, \
		_ui_service_main_loop);
#endif /* CONFIG_SIMULATOR */

static void *uisrv_tid;

static int _ui_service_start(void)
{
	static const ui_srv_init_param_t init_param = {
#if defined(CONFIG_VIEW_SCROLL_MEM_SAVE)
		.scrl_mem_mode = VIEW_SCROLL_MEM_SAVE,
#elif defined(CONFIG_VIEW_SCROLL_MEM_LOWEST)
		.scrl_mem_mode = VIEW_SCROLL_MEM_LOWEST,
#else
		.scrl_mem_mode = VIEW_SCROLL_MEM_DEF,
#endif

		.view_ovl_opa = CONFIG_UI_VIEW_OVERLAY_OPA,
		.trans_buf_cnt = CONFIG_UI_EFFECT_TRANSFORM_BUFFER_COUNT,
	};

	struct app_msg msg = {0};

	if (!srv_manager_check_service_is_actived(UI_SERVICE_NAME)) {
		if (srv_manager_active_service(UI_SERVICE_NAME)) {
			uisrv_tid = srv_manager_get_servicetid(UI_SERVICE_NAME);
			SYS_LOG_DBG("ui service start ok\n");
		} else {
			SYS_LOG_ERR("ui service start failed\n");
			return -ESRCH;
		}
	}

	msg.type = MSG_INIT_APP;
	msg.ptr = (void *)&init_param;

	return !send_async_msg(UI_SERVICE_NAME, &msg);
}

static int _ui_service_stop(void)
{
	int ret = 0;

	if (!srv_manager_check_service_is_actived(UI_SERVICE_NAME)) {
		SYS_LOG_ERR("ui service_stop failed\n");
		ret = -ESRCH;
		goto exit;
	}

	if (!srv_manager_exit_service(UI_SERVICE_NAME)) {
		ret = -ETIMEDOUT;
		goto exit;
	}

	SYS_LOG_DBG("ui service_stop success!\n");
exit:
	return ret;
}

static int _ui_service_send_async_ext(uint16_t view_id, uint8_t msg_id,
		uint32_t msg_data, uint32_t msg_data2, MSG_CALLBAK msg_cb, bool droppable)
{
	struct app_msg msg = {
		.type = MSG_UI_EVENT,
		.sender = view_id,
		.cmd = msg_id,
		.reserve = msg_data2 & 0xff,
		.__pad[0] = (msg_data2 >> 8) & 0xff,
		.__pad[1] = (msg_data2 >> 16) & 0xff,
		.__pad[2] = (msg_data2 >> 24) & 0xff,
		.value = msg_data,
		.callback = msg_cb,
	};

	return droppable ?
				!send_async_msg_discardable(UI_SERVICE_NAME, &msg) :
				!send_async_msg(UI_SERVICE_NAME, &msg);
}

static void _uisrv_msg_sync_callback(struct app_msg * msg, int result, void *data)
{
	if (msg->sync_sem) {
		uisrv_msg_notify_t *msg_notify =
				CONTAINER_OF(msg->sync_sem, uisrv_msg_notify_t, sem);

		if (msg_notify->cb)
			msg_notify->cb(msg, result, data);

#ifndef CONFIG_SIMULATOR
		os_sem_give(&msg_notify->sem);
#else
		msg_notify->cb = (void *)0xFFFFFFFF;
#endif
	}
}

static int _ui_service_send_sync(uint16_t view_id, uint8_t msg_id,
		uint32_t msg_data, uint16_t msg_data2, MSG_CALLBAK msg_cb)
{
	uisrv_msg_notify_t msg_notify;
	struct app_msg msg = {
		.type = MSG_UI_EVENT,
		.sender = view_id,
		.cmd = msg_id,
		.reserve = msg_data2 & 0xff,
		.__pad[0] = (msg_data2 >> 8) & 0xff,
		.__pad[1] = (msg_data2 >> 16) & 0xff,
		.__pad[2] = (msg_data2 >> 24) & 0xff,
		.value = msg_data,
		.sync_sem = &msg_notify.sem,
		.callback = _uisrv_msg_sync_callback,
	};

	os_sem_init(&msg_notify.sem, 0, 1);
	msg_notify.cb = msg_cb;

	if (send_async_msg(UI_SERVICE_NAME, &msg) == false) {
		return -ENOBUFS;
	}

#ifndef CONFIG_SIMULATOR
	if (os_sem_take(&msg_notify.sem, OS_FOREVER)) {
		return -ETIME;
	}
#else
	while (msg_notify.cb != (void *)0xFFFFFFFF) {
		os_sleep(10);
	}
#endif

	return 0;
}

#else

static int _ui_service_send_async_ext(uint16_t view_id, uint8_t msg_id,
		uint32_t msg_data, uint32_t msg_data2, MSG_CALLBAK msg_cb,
		bool droppable)
{
	return -ENOSYS;
}

static int _ui_service_send_sync(uint16_t view_id, uint8_t msg_id,
		uint32_t msg_data, uint16_t msg_data2, MSG_CALLBAK msg_cb,
		bool droppable)
{
	return -ENOSYS;
}
#endif /* CONFIG_UI_SERVICE */

static inline int _ui_service_send_async(uint16_t view_id, uint8_t msg_id,
		uint32_t msg_data, uint32_t msg_data2)
{
	return _ui_service_send_async_ext(view_id, msg_id, msg_data, msg_data2, NULL, false);
}

static inline int _ui_service_send_async_droppable(uint16_t view_id, uint8_t msg_id,
		uint32_t msg_data, uint32_t msg_data2)
{
	return _ui_service_send_async_ext(view_id, msg_id, msg_data, msg_data2, NULL, true);
}

int ui_message_send_async(uint16_t view_id, uint8_t msg_id, uint32_t msg_data)
{
	SYS_LOG_DBG("view_id %d, msg_id %d, msg_data 0x%x\n", view_id, msg_id, msg_data);

	return _ui_service_send_async(view_id, msg_id, msg_data, 0);
}

int ui_message_send_async2(uint16_t view_id, uint8_t msg_id, uint32_t msg_data, MSG_CALLBAK msg_cb)
{
	SYS_LOG_DBG("view_id %d, msg_id %d, msg_data 0x%x, msg_cb %p\n",
			view_id, msg_id, msg_data, msg_cb);

	return _ui_service_send_async_ext(view_id, msg_id, msg_data, 0, msg_cb, false);
}

int ui_message_send_sync(uint16_t view_id, uint8_t msg_id, uint32_t msg_data)
{
	SYS_LOG_DBG("view_id %d, msg_id %d, msg_data 0x%x\n", view_id, msg_id, msg_data);

	return _ui_service_send_sync(view_id, msg_id, msg_data, 0, NULL);
}

int ui_message_send_sync2(uint16_t view_id, uint8_t msg_id, uint32_t msg_data, MSG_CALLBAK msg_cb)
{
	SYS_LOG_DBG("view_id %d, msg_id %d, msg_data 0x%x, msg_cb %p\n",
			view_id, msg_id, msg_data, msg_cb);

	return _ui_service_send_sync(view_id, msg_id, msg_data, 0, msg_cb);
}

int ui_view_create(uint16_t view_id, const void *presenter, uint8_t flags)
{
	return _ui_service_send_async(view_id, MSG_VIEW_CREATE, (uintptr_t)presenter, flags);
}

int ui_view_layout(uint16_t view_id)
{
	return _ui_service_send_async(view_id, MSG_VIEW_LAYOUT, 0, 0);
}

int ui_view_update(uint16_t view_id)
{
	return _ui_service_send_async(view_id, MSG_VIEW_UPDATE, 0, 0);
}

int ui_view_delete(uint16_t view_id)
{
	return _ui_service_send_async(view_id, MSG_VIEW_DELETE, 0, 0);
}

int ui_view_show(uint16_t view_id)
{
	return _ui_service_send_async(view_id, MSG_VIEW_SET_HIDDEN, 0, 0);
}

int ui_view_hide(uint16_t view_id)
{
	return _ui_service_send_async(view_id, MSG_VIEW_SET_HIDDEN, 1, 0);
}

int ui_view_set_order(uint16_t view_id, uint16_t order)
{
	return _ui_service_send_async(view_id, MSG_VIEW_SET_ORDER, order, 0);
}

int ui_view_set_pos(uint16_t view_id, int16_t x, int16_t y)
{
	union {
		uint32_t val_32;
		int16_t val_16[2];
	} value;

	value.val_16[0] = x;
	value.val_16[1] = y;

	return _ui_service_send_async(view_id, MSG_VIEW_SET_POS, value.val_32, 0);
}

int ui_view_set_drag_attribute(uint16_t view_id, uint16_t drag_attribute, bool keep_pos)
{
	return _ui_service_send_async(view_id, MSG_VIEW_SET_DRAG_ATTRIBUTE, drag_attribute, keep_pos);
}

int ui_view_paint2(uint16_t view_id, uint16_t user_id, void *user_data)
{
	return _ui_service_send_async_droppable(view_id, MSG_VIEW_PAINT, (uintptr_t)user_data, user_id);
}

int ui_view_refresh(uint16_t view_id)
{
	return _ui_service_send_async_droppable(view_id, MSG_VIEW_REFRESH, 0, 0);
}

int ui_view_pause(uint16_t view_id)
{
	return _ui_service_send_async(view_id, MSG_VIEW_PAUSE, 0, 0);
}

int ui_view_resume(uint16_t view_id)
{
	return _ui_service_send_async(view_id, MSG_VIEW_RESUME, 0, 0);
}

int ui_view_send_user_msg2(uint16_t view_id, uint8_t msg_id, uint16_t user_id, void * user_data)
{
	if (msg_id >= MSG_VIEW_USER_OFFSET) {
		return _ui_service_send_async(view_id, msg_id, (uintptr_t)user_data, user_id);
	}

	return -EINVAL;
}

int ui_msgbox_popup(uint16_t msgbox_id, void *user_data)
{
	return _ui_service_send_async(msgbox_id, MSG_MSGBOX_POPUP, (uintptr_t)user_data, 0);
}

int ui_msgbox_close(uint16_t msgbox_id, bool bsync)
{
	if (bsync) {
		return _ui_service_send_sync(msgbox_id, MSG_MSGBOX_CLOSE, 0, 0, NULL);
	} else {
		return _ui_service_send_async(msgbox_id, MSG_MSGBOX_CLOSE, 0, 0);
	}
}

int ui_msgbox_paint(uint16_t msgbox_id, void * user_data)
{
	return _ui_service_send_async_droppable(msgbox_id, MSG_MSGBOX_PAINT, (uintptr_t)user_data, 0);
}

int ui_manager_init(void)
{
#ifdef CONFIG_UI_MEMORY_MANAGER
	ui_mem_init();
#endif

#ifdef CONFIG_UI_SERVICE
	_ui_service_start();
#endif

#ifdef CONFIG_LED_MANAGER
	led_manager_init();
#endif

#ifdef CONFIG_SEG_LED_MANAGER
	seg_led_manager_init();
#endif

	return 0;
}

int ui_manager_exit(void)
{
#ifdef CONFIG_LED_MANAGER
	led_manager_deinit();
#endif

#ifdef CONFIG_SEG_LED_MANAGER
	seg_led_manager_deinit();
#endif

#ifdef CONFIG_UI_SERVICE
	_ui_service_stop();
#endif

	return 0;
}

bool is_in_ui_thread(void)
{
#ifdef CONFIG_UI_SERVICE
	return os_current_get() == uisrv_tid;
#else
	return false;
#endif
}

int ui_manager_dispatch_key_event(uint32_t key_event)
{
#ifdef CONFIG_UI_SERVICE
	struct app_msg msg = {
		.type = MSG_KEY_INPUT,
		.value = key_event,
	};

	return !send_async_msg(UI_SERVICE_NAME, &msg);
#else
	return -ENOSYS;
#endif
}

int ui_manager_lock_display(void)
{
	return _ui_service_send_async(0, MSG_DISPLAY_LOCK, 1, 0);
}

int ui_manager_unlock_display(void)
{
	return _ui_service_send_async(0, MSG_DISPLAY_LOCK, 0, 0);
}

int ui_manager_set_display_rotation(uint16_t rotation)
{
	if (rotation != 0 && rotation != 90 && rotation != 180 && rotation != 270)
		return -EINVAL;

	return _ui_service_send_async(0, MSG_DISPLAY_ROTATE, rotation, 0);
}

int ui_manager_register_callback(uint8_t cb_id, void * cb_func)
{
	return _ui_service_send_async(0, MSG_VIEW_SET_CALLBACK, (uintptr_t)cb_func, cb_id);
}

int ui_manager_set_max_buffer_count(uint8_t buf_count)
{
	return _ui_service_send_async(0, MSG_VIEW_SET_BUF_COUNT, buf_count, 0);
}

int ui_gesture_set_scroll_dir(uint8_t dir)
{
#ifdef CONFIG_UI_SERVICE
	if (is_in_ui_thread()) {
		extern void gesture_manager_set_scroll_dir(uint8_t dir);
		gesture_manager_set_scroll_dir(dir);
		return 0;
	} else {
		return _ui_service_send_async(0, MSG_GESTURE_SET_SCROLL_DIR, dir, 0);
	}
#else
	return -ENOSYS;
#endif
}

int ui_gesture_lock_scroll(void)
{
#ifdef CONFIG_UI_SERVICE
	if (is_in_ui_thread()) {
		extern void gesture_manager_lock_scroll(void);
		gesture_manager_lock_scroll();
		return 0;
	} else {
		return _ui_service_send_sync(0, MSG_GESTURE_LOCK_SCROLL, 1, 0, NULL);
	}
#else
	return -ENOSYS;
#endif
}

int ui_gesture_unlock_scroll(void)
{
#ifdef CONFIG_UI_SERVICE
	if (is_in_ui_thread()) {
		extern void gesture_manager_unlock_scroll(void);
		gesture_manager_unlock_scroll();
		return 0;
	} else {
		return _ui_service_send_async(0, MSG_GESTURE_LOCK_SCROLL, 0, 0);
	}
#else
	return -ENOSYS;
#endif
}

int ui_gesture_stop_scroll(void)
{
	return _ui_service_send_async(0, MSG_GESTURE_STOP_SCROLL, 0, 0);
}

int ui_gesture_wait_release(void)
{
#ifdef CONFIG_UI_SERVICE
	if (is_in_ui_thread()) {
		extern void gesture_manager_wait_release(void);
		gesture_manager_wait_release();
		return 0;
	} else {
		return _ui_service_send_async(0, MSG_GESTURE_WAIT_RELEASE, 0, 0);
	}
#else
	return -ENOSYS;
#endif
}
