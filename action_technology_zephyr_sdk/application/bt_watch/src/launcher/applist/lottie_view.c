/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

 /**
  * @file sport view
  */

#include <app_ui.h>

#ifdef CONFIG_LV_USE_LOTTIE

#include <ui_mem.h>
#include <memory/mem_cache.h>

#ifdef CONFIG_SYS_WAKELOCK
#  include <sys_wakelock.h>
#endif

#ifdef CONFIG_ACTS_DVFS_DYNAMIC_LEVEL
#  include <dvfs.h>
#endif

#define TEST_FILE     DEF_UI_DISK"/lottie.jsn"
#define CANVAS_WIDTH  256
#define CANVAS_HEIGHT 256
#define CANVAS_SIZE   (CANVAS_WIDTH * CANVAS_HEIGHT * 4)

typedef struct {
	uint8_t *fdata; /* file content */
	uint8_t *cdata; /* canvas content */
} lottie_view_data_t;

static void _lottie_draw_main_begin_cb(lv_event_t *e)
{
	lottie_view_data_t *data = lv_event_get_user_data(e);
	mem_dcache_clean(data->cdata, CANVAS_SIZE);
}

static int _lottie_view_layout(view_data_t *view_data)
{
	lottie_view_data_t *data = app_mem_malloc(sizeof(*data));
	if (data == NULL)
		return -ENOMEM;

	memset(data, 0, sizeof(*data));

	data->cdata = ui_mem_alloc(MEM_RES, CANVAS_SIZE, __func__);
	if (data->cdata == NULL)
		goto fail_free_data;

	memset(data->cdata, 0, CANVAS_SIZE);

    lv_fs_file_t file;
    if (LV_FS_RES_OK != lv_fs_open(&file, TEST_FILE, LV_FS_MODE_RD))
		goto fail_free_canvas_data;

    uint32_t fsize = 0;
    lv_fs_seek(&file, 0, LV_FS_SEEK_END);
    lv_fs_tell(&file, &fsize);
    lv_fs_seek(&file, 0, LV_FS_SEEK_SET);

    data->fdata = ui_mem_alloc(MEM_RES, fsize, __func__);
	if (data->fdata == NULL) {
		lv_fs_close(&file);
		goto fail_free_canvas_data;
	} else {
        lv_fs_read(&file, data->fdata, fsize, NULL);
		lv_fs_close(&file);

        data->fdata[fsize] = 0;
	}

#ifdef CONFIG_SYS_WAKELOCK
	sys_wake_lock(FULL_WAKE_LOCK);
#endif

#ifdef CONFIG_ACTS_DVFS_DYNAMIC_LEVEL
	dvfs_set_level(DVFS_LEVEL_HIGH_PERFORMANCE, "lottie");
#endif

	lv_obj_t *scr = lv_display_get_screen_active(view_data->display);
	lv_obj_t *obj = lv_lottie_create(scr);
	lv_lottie_set_src_data(obj, data->fdata, fsize + 1, false);
	lv_lottie_set_buffer(obj, CANVAS_WIDTH, CANVAS_HEIGHT, data->cdata);
	lv_obj_center(obj);
	lv_obj_add_event_cb(obj, _lottie_draw_main_begin_cb, LV_EVENT_DRAW_MAIN_BEGIN, data);

	view_data->user_data = data;
	return 0;

fail_free_canvas_data:
	ui_mem_free(MEM_RES, data->cdata);
fail_free_data:
	app_mem_free(data);
	return -ENOMEM;
}

static int _lottie_view_delete(view_data_t* view_data)
{
	lottie_view_data_t *data = view_data->user_data;
	if (data == NULL)
		return 0;

	ui_mem_free(MEM_RES, data->fdata);
	ui_mem_free(MEM_RES, data->cdata);
	app_mem_free(data);

#ifdef CONFIG_ACTS_DVFS_DYNAMIC_LEVEL
	dvfs_unset_level(DVFS_LEVEL_HIGH_PERFORMANCE, "lottie");
#endif

#ifdef CONFIG_SYS_WAKELOCK
	sys_wake_unlock(FULL_WAKE_LOCK);
#endif

	return 0;
}

int lottie_view_handler(uint16_t view_id, view_data_t* view_data, uint8_t msg_id, void* msg_data)
{
	switch (msg_id) {
	case MSG_VIEW_PRELOAD:
		return ui_view_layout(view_id);
	case MSG_VIEW_LAYOUT:
		return _lottie_view_layout(view_data);
	case MSG_VIEW_DELETE:
		return _lottie_view_delete(view_data);
	default:
		return 0;
	}
	return 0;
}

VIEW_DEFINE2(lottie_view, lottie_view_handler, NULL, NULL, LOTTIE_VIEW,
	NORMAL_ORDER, UI_VIEW_LVGL, DEF_UI_VIEW_WIDTH, DEF_UI_VIEW_HEIGHT);

#endif /* CONFIG_LV_USE_LOTTIE */
