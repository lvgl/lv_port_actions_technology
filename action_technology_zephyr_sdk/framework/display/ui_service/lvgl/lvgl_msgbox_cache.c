/*
 * Copyright (c) 2020 Actions Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_MODULE_CUSTOMER

#include <os_common_api.h>
#include <mem_manager.h>
#include <msgbox_cache.h>
#include <lvgl/lvgl.h>
#include <assert.h>
#include <sys/atomic.h>
#include <sys/dlist.h>

LOG_MODULE_REGISTER(msgbox, LOG_LEVEL_INF);

typedef struct {
	sys_dlist_t node;

	lv_obj_t * hwnd;
	void *user_data;

	const msgbox_dsc_t *dsc;
	uint16_t view_id;
} msgbox_popup_t;

typedef struct {
	const msgbox_dsc_t *dsc;
	uint8_t num;

	uint8_t en : 1;
	uint16_t top; /* top most msgbox popup id */

	atomic_t num_popups;
	sys_dlist_t popup_list;
} msgbox_cache_ctx_t;

static void _msgbox_popup_handler(uint16_t msgbox_id, uint8_t msg_id, void * msg_data, void * user_data);
static const msgbox_dsc_t *_msgbox_find_dsc(uint16_t msgbox_id);
static msgbox_popup_t *_msgbox_find_popup_by_id(uint16_t msgbox_id);
static msgbox_popup_t *_msgbox_find_popup_by_hwnd(void * hwnd);
static void _msgbox_update_top(void * hwnd);

static void _call_view_proc(uint16_t view_id, uint16_t msgbox_id, uint8_t msg_id);

static msgbox_cache_ctx_t msgbox_ctx;

int msgbox_cache_init(const msgbox_dsc_t *dsc, uint8_t num)
{
	memset(&msgbox_ctx, 0, sizeof(msgbox_ctx));

	msgbox_ctx.en = 1;
	msgbox_ctx.dsc = dsc;
	msgbox_ctx.num = num;
	msgbox_ctx.top = MSGBOX_ID_ALL;
	atomic_set(&msgbox_ctx.num_popups, 0);
	sys_dlist_init(&msgbox_ctx.popup_list);

	ui_manager_set_msgbox_popup_callback(_msgbox_popup_handler);
	return 0;
}

void msgbox_cache_deinit(void)
{
	msgbox_cache_close(MSGBOX_ID_ALL, false);
	ui_manager_set_msgbox_popup_callback(NULL);
}

void msgbox_cache_set_en(bool en)
{
	msgbox_ctx.en = en ? 1 : 0;
}

uint8_t msgbox_cache_num_popup_get(void)
{
	return (uint8_t)atomic_get(&msgbox_ctx.num_popups);
}

uint16_t msgbox_cache_get_top(void)
{
	return msgbox_ctx.top;
}

int msgbox_cache_popup(uint16_t msgbox_id, void *user_data)
{
	if (msgbox_ctx.dsc == NULL || msgbox_ctx.en == 0) {
		SYS_LOG_WRN("msgbox %u disabled", msgbox_id);
		return -EPERM;
	}

	const msgbox_dsc_t *dsc = _msgbox_find_dsc(msgbox_id);
	if (!dsc || (dsc->handler == NULL && dsc->popup_cb == NULL)) {
		SYS_LOG_WRN("msgbox %u invalid", msgbox_id);
		return -EINVAL;
	}

	atomic_inc(&msgbox_ctx.num_popups);

	if (ui_msgbox_popup(msgbox_id, user_data)) {
		atomic_dec(&msgbox_ctx.num_popups);
		return -ENOMSG;
	}

	return 0;
}

int msgbox_cache_close(uint16_t msgbox_id, bool bsync)
{
	return ui_msgbox_close(msgbox_id, bsync);
}

int msgbox_cache_paint(uint16_t msgbox_id, void * user_data)
{
	return ui_msgbox_paint(msgbox_id, user_data);
}

void msgbox_cache_dump(void)
{
	sys_dnode_t *node;

	os_printk("msgbox (en %d, num %d, top %u):\n", msgbox_ctx.en,
			msgbox_cache_num_popup_get(), msgbox_cache_get_top());

	if (msgbox_cache_num_popup_get() == 0) {
		return;
	}

	os_printk("  id | order |   hwnd   | view\n"
	          "-----+-------+----------+-----\n");

	SYS_DLIST_FOR_EACH_NODE(&msgbox_ctx.popup_list, node) {
		msgbox_popup_t * popup = CONTAINER_OF(node, msgbox_popup_t, node);

		os_printk(" %3u |  %02x   | %08x | %3u\n", popup->dsc->id,
			popup->dsc->order, popup->hwnd, popup->view_id);
	}

	os_printk("\n");
}

static void _msgbox_popup(uint16_t msgbox_id, uint8_t msg_id, void * msg_data, void * user_data)
{
	uint16_t view_id = (uint16_t)(uintptr_t)msg_data;
	view_data_t *view_data = view_get_data(view_id);
	lv_obj_t *top_layer = lv_disp_get_layer_top(view_data->display);
	const msgbox_dsc_t *dsc;
	msgbox_popup_t *popup = NULL;
	sys_dnode_t *node;
	int index;

	if (!top_layer) {
		goto fail_exit;
	}

	if (_msgbox_find_popup_by_id(msgbox_id)) {
		goto fail_exit;
	}

	dsc = _msgbox_find_dsc(msgbox_id);
	assert(dsc != NULL && (dsc->popup_cb != NULL || dsc->handler != NULL));

	popup = mem_malloc(sizeof(*popup));
	if (popup == NULL) {
		if (dsc->handler) {
			dsc->handler(msgbox_id, MSG_MSGBOX_CANCEL, user_data, NULL);
		} else {
			dsc->popup_cb(msgbox_id, MSG_MSGBOX_CANCEL, NULL, user_data);
		}

		goto fail_exit;
	}

	memset(popup, 0, sizeof(*popup));
	popup->dsc = dsc;
	popup->view_id = view_id;
	popup->user_data = user_data;

	popup->hwnd = lv_obj_create(top_layer);
	if (popup->hwnd == NULL) {
		goto fail_exit;
	}

	lv_obj_set_size(popup->hwnd, LV_PCT(100), LV_PCT(100));
	lv_obj_add_flag(popup->hwnd, LV_OBJ_FLAG_CLICKABLE);
	lv_obj_clear_flag(popup->hwnd, LV_OBJ_FLAG_GESTURE_BUBBLE);

	if (dsc->handler) {
		if (dsc->handler(msgbox_id, MSG_MSGBOX_POPUP, user_data, popup->hwnd)) {
			goto fail_exit;
		}
	} else {
		if (dsc->popup_cb(msgbox_id, MSG_MSGBOX_POPUP, popup->hwnd, user_data) == NULL) {
			goto fail_exit;
		}
	}

	index = lv_obj_get_child_cnt(top_layer) - 1;
	SYS_DLIST_FOR_EACH_NODE(&msgbox_ctx.popup_list, node) {
		msgbox_popup_t * item = CONTAINER_OF(node, msgbox_popup_t, node);

		if (item->dsc->order < popup->dsc->order) index--;
	}

	/* change the order */
	lv_obj_move_to_index(popup->hwnd, index);

	sys_dlist_append(&msgbox_ctx.popup_list, &popup->node);

	_msgbox_update_top(lv_obj_get_child(top_layer, lv_obj_get_child_cnt(top_layer) - 1));

	_call_view_proc(popup->view_id, popup->dsc->id, MSG_MSGBOX_POPUP);

	SYS_LOG_INF("create msgbox %d (%p), attached to view %u, top %p",
			msgbox_id, popup->hwnd, view_id, top_layer);
	return;

fail_exit:
	SYS_LOG_ERR("fail to create msgbox %d", msgbox_id);

	if (popup) {
		if (popup->hwnd) {
			lv_obj_delete(popup->hwnd);
		}

		mem_free(popup);
	}

	atomic_dec(&msgbox_ctx.num_popups);
	ui_gesture_unlock_scroll();
}

static void _msgbox_cancel(uint16_t msgbox_id, uint8_t msg_id, void * msg_data, void * user_data)
{
	msgbox_popup_t *popup = _msgbox_find_popup_by_id(msgbox_id);

	SYS_LOG_WRN("msgbox %u cancel", msgbox_id);

	if (popup == NULL) {
		const msgbox_dsc_t *dsc = _msgbox_find_dsc(msgbox_id);
		assert(dsc != NULL);

		if (dsc->handler) {
			dsc->handler(msgbox_id, MSG_MSGBOX_CANCEL, user_data, NULL);
		} else {
			dsc->popup_cb(msgbox_id, MSG_MSGBOX_CANCEL, NULL, user_data);
		}

		atomic_dec(&msgbox_ctx.num_popups);
	}
}

static void _msgbox_paint(uint16_t msgbox_id, uint8_t msg_id, void * msg_data)
{
	msgbox_popup_t *popup = _msgbox_find_popup_by_id(msgbox_id);

	if (popup) {
		if (popup->dsc->handler) {
			popup->dsc->handler(msgbox_id, MSG_MSGBOX_PAINT, msg_data, popup->hwnd);
		} else {
			popup->dsc->popup_cb(msgbox_id, MSG_MSGBOX_PAINT, msg_data, popup->user_data);
		}
	}
}

static void _msgbox_close(uint16_t msgbox_id, uint8_t msg_id, void * msg_data)
{
	uint16_t view_id = (uint16_t)(uintptr_t)msg_data;
	sys_dnode_t *dn, *dns;

	SYS_DLIST_FOR_EACH_NODE_SAFE(&msgbox_ctx.popup_list, dn, dns) {
		msgbox_popup_t * popup = CONTAINER_OF(dn, msgbox_popup_t, node);

		if ((msgbox_id == popup->dsc->id) ||
			(msgbox_id == MSGBOX_ID_ALL && (popup->view_id == view_id || view_id == VIEW_INVALID_ID))) {
			lv_obj_t * top_layer = lv_obj_get_parent(popup->hwnd);

			SYS_LOG_INF("delete msgbox %d (%p)", popup->dsc->id, popup->hwnd);

			sys_dlist_remove(&popup->node);

			if (popup->dsc->handler) {
				popup->dsc->handler(popup->dsc->id, MSG_MSGBOX_CLOSE, NULL, popup->hwnd);
			} else {
				popup->dsc->popup_cb(popup->dsc->id, MSG_MSGBOX_CLOSE, popup->hwnd, popup->user_data);
			}

			if (atomic_dec(&msgbox_ctx.num_popups) == 1) {
				/* no msgbox now */
				msgbox_ctx.top = MSGBOX_ID_ALL;
			} else if (top_layer && popup->hwnd == lv_obj_get_child(top_layer, lv_obj_get_child_cnt(top_layer) - 1)) {
				_msgbox_update_top(lv_obj_get_child(top_layer, lv_obj_get_child_cnt(top_layer) - 2));
			}

			lv_obj_delete(popup->hwnd);
			_call_view_proc(popup->view_id, popup->dsc->id, MSG_MSGBOX_CLOSE);

			mem_free(popup);

			ui_gesture_unlock_scroll();
		}
	}
}

static void _msgbox_proc_key(ui_key_msg_data_t *data)
{
	if (msgbox_ctx.top != MSGBOX_INVALID_ID) {
		msgbox_popup_t *popup = _msgbox_find_popup_by_id(msgbox_ctx.top);
		if (popup) {
			if (popup->dsc->handler) {
				popup->dsc->handler(popup->dsc->id, MSG_MSGBOX_KEY, data, popup->hwnd);
			} else {
				popup->dsc->popup_cb(popup->dsc->id, MSG_MSGBOX_KEY, data, popup->user_data);
			}

			if (data->done)
				SYS_LOG_INF("msgbox %u done key 0x%x", popup->dsc->id, data->event);
		}
	}
}

static void _msgbox_popup_handler(uint16_t msgbox_id, uint8_t msg_id, void * msg_data, void * user_data)
{
	switch (msg_id) {
	case MSG_MSGBOX_POPUP:
		_msgbox_popup(msgbox_id, msg_id, msg_data, user_data);
		break;
	case MSG_MSGBOX_CANCEL:
		_msgbox_cancel(msgbox_id, msg_id, NULL, user_data);
		break;
	case MSG_MSGBOX_CLOSE:
		_msgbox_close(msgbox_id, msg_id, msg_data);
		break;
	case MSG_MSGBOX_PAINT:
		_msgbox_paint(msgbox_id, msg_id, msg_data);
		break;
	case MSG_MSGBOX_KEY:
		_msgbox_proc_key(msg_data);
		break;
	default:
		break;
	}
}

static const msgbox_dsc_t *_msgbox_find_dsc(uint16_t msgbox_id)
{
	int i;

	for (i = msgbox_ctx.num - 1; i >= 0; i--) {
		if (msgbox_id == msgbox_ctx.dsc[i].id) {
			return &msgbox_ctx.dsc[i];
		}
	}

	return NULL;
}

static msgbox_popup_t *_msgbox_find_popup_by_id(uint16_t msgbox_id)
{
	sys_dnode_t *node;

	SYS_DLIST_FOR_EACH_NODE(&msgbox_ctx.popup_list, node) {
		msgbox_popup_t * item = CONTAINER_OF(node, msgbox_popup_t, node);

		if (item->dsc->id == msgbox_id) {
			return item;
		}
	}

	return NULL;
}

static msgbox_popup_t *_msgbox_find_popup_by_hwnd(void * hwnd)
{
	sys_dnode_t *node;

	SYS_DLIST_FOR_EACH_NODE(&msgbox_ctx.popup_list, node) {
		msgbox_popup_t *item = CONTAINER_OF(node, msgbox_popup_t, node);

		if (item->hwnd == hwnd) {
			return item;
		}
	}

	return NULL;
}

static void _msgbox_update_top(void * hwnd)
{
	msgbox_popup_t *popup;

	popup = _msgbox_find_popup_by_hwnd(hwnd);
	if (popup) {
		msgbox_ctx.top = popup->dsc->id;
	} else {
		msgbox_ctx.top = MSGBOX_INVALID_ID;
	}
}

static void _call_view_proc(uint16_t view_id, uint16_t msgbox_id, uint8_t msg_id)
{
	view_entry_t *entry = view_manager_get_view_entry(view_id);

	if (entry != NULL) {
		if (entry->proc2) {
			entry->proc2(view_id, view_get_data(view_id), msg_id, (void *)(uintptr_t)msgbox_id);
		} else if (entry->proc) {
			entry->proc(view_id, msg_id, (void *)(uintptr_t)msgbox_id);
		}
	}
}
