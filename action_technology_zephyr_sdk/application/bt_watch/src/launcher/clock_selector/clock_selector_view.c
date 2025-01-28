#include <lvgl.h>
#include <lvgl/lvgl_bitmap_font.h>
#include <lvgl/lvgl_res_loader.h>
#include <lvgl/lvgl_img_loader.h>
#include <ui_manager.h>
#include <view_manager.h>
#include <assert.h>
#include "widgets/simple_img.h"
#include "clock_selector_view.h"


#define SCENE_CLOCK_SELECTOR_ID SCENE_CLOCK_SEL_VIEW

#define PRELOAD_5_PICS

typedef struct clock_selector_view_data {
	lv_obj_t *tileview;

	lv_obj_t **img;
	lv_obj_t **txt;

	lv_image_dsc_t src[3];

	int8_t sel_src; /* src index of the selected clock index */
	int8_t sel_idx; /* selected clock index */

	lv_font_t font;
	lv_style_t sty_txt;
	lv_style_t sty_bg;

	lvgl_res_picregion_t picreg;
	lvgl_res_string_t strtxt[1];
	lvgl_res_scene_t scene;
} clock_selector_view_data_t;

static void _clock_selector_view_unload_resource(clock_selector_view_data_t *data);

static int _clock_selector_view_load_resource(clock_selector_view_data_t *data)
{
	uint32_t res_id = STR_NAME;
	int ret;

	/* scene */
	ret = lvgl_res_load_scene(SCENE_CLOCK_SELECTOR_ID, &data->scene,
			DEF_STY_FILE, DEF_RES_FILE, DEF_STR_FILE);
	if (ret) {
		SYS_LOG_ERR("scene selector view not found");
		return -ENOENT;
	}

	ret = lvgl_res_load_picregion_from_scene(&data->scene, RES_THUMBNAIL, &data->picreg);
	if (ret || data->picreg.frames < 1) {
		SYS_LOG_ERR("cannot find picreg 0x%x\n", RES_THUMBNAIL);
		goto fail_exit;
	}

	/* preload pictures */
#ifdef PRELOAD_5_PICS
	lvgl_res_preload_pictures_from_picregion(0, &data->picreg,
			MAX(data->sel_idx - 2, 0), MIN(data->sel_idx + 2, data->picreg.frames - 1));
#else
	lvgl_res_preload_pictures_from_picregion(0, &data->picreg,
			MAX(data->sel_idx - 1, 0), MIN(data->sel_idx + 1, data->picreg.frames - 1));
#endif

	ret = lvgl_res_load_strings_from_scene(&data->scene, &res_id, data->strtxt, ARRAY_SIZE(data->strtxt));
	if (ret == 0) {
		ret = LVGL_FONT_OPEN_DEFAULT(&data->font, DEF_FONT_SIZE_NORMAL);
		if (ret) {
			goto fail_exit;
		}
	}

	return 0;
fail_exit:
	_clock_selector_view_unload_resource(data);
	return -ENOENT;
}

static void _clock_selector_view_unload_resource(clock_selector_view_data_t *data)
{
	lvgl_res_preload_cancel();

	if (data->strtxt[0].txt) {
		LVGL_FONT_CLOSE(&data->font);
		lvgl_res_unload_strings(data->strtxt, ARRAY_SIZE(data->strtxt));
	}

	lvgl_res_unload_pictures(data->src, ARRAY_SIZE(data->src));
	lvgl_res_unload_picregion(&data->picreg);
	lvgl_res_unload_scene(&data->scene);
	lvgl_res_unload_scene_compact(SCENE_CLOCK_SELECTOR_ID);
}

static void _load_element_resource(view_data_t *view_data, int elem_idx, int src_idx)
{
	const clock_selector_view_presenter_t *presenter = view_get_presenter(view_data);
	clock_selector_view_data_t *data = view_data->user_data;
	int res;

	if (elem_idx < 0 || elem_idx >= data->picreg.frames)
		return;

	if (data->txt) {
		const char *name = presenter->get_clock_name(elem_idx);
		lv_label_set_text_static(data->txt[elem_idx], name ? name : "unknown");
	}

	lvgl_res_unload_pictures(&data->src[src_idx], 1);

	res = lvgl_res_load_pictures_from_picregion(&data->picreg, elem_idx, elem_idx, &data->src[src_idx]);
	if (res) {
		SYS_LOG_ERR("fail to load pic %d\n", elem_idx);
		simple_img_set_src(data->img[elem_idx], NULL);
	} else {
		simple_img_set_src(data->img[elem_idx], &data->src[src_idx]);
	}
}

static int _clock_selector_view_preload(view_data_t *view_data)
{
	const clock_selector_view_presenter_t *presenter = view_get_presenter(view_data);
	clock_selector_view_data_t *data;

	data = app_mem_malloc(sizeof(*data));
	if (data == NULL) {
		return -ENOMEM;
	}

	memset(data, 0, sizeof(*data));
	if (presenter->get_clock_id) {
		data->sel_idx = presenter->get_clock_id();
	} else {
		data->sel_idx = 0;
	}

	if (data->sel_idx == 0) {
		lvgl_res_clear_regular_pictures(SCENE_CLOCK_VIEW, DEF_STY_FILE);
		lvgl_res_unload_scene_compact(SCENE_CLOCK_VIEW);
	} else {
		lvgl_res_clear_regular_pictures(SCENE_DIGITAL_CLOCK_VIEW, DEF_STY_FILE);
		lvgl_res_unload_scene_compact(SCENE_DIGITAL_CLOCK_VIEW);
	}

	if (_clock_selector_view_load_resource(data)) {
		app_mem_free(data);
		return -ENOENT;
	}

	data->img = app_mem_malloc(sizeof(lv_obj_t *) * data->picreg.frames);
	if (data->img == NULL) {
		goto fail_exit;
	}

	if (data->strtxt[0].txt) {
		data->txt = app_mem_malloc(sizeof(lv_obj_t *) * data->picreg.frames);
		if (data->txt == NULL) {
			goto fail_exit;
		}
	}

	view_data->user_data = data;

	ui_view_layout(CLOCK_SELECTOR_VIEW);
	return 0;
fail_exit:
	_clock_selector_view_unload_resource(data);
	app_mem_free(data->img);
	app_mem_free(data->txt);
	app_mem_free(data);
	return -ENOMEM;
}

static void _clock_selector_tileview_event_handler(lv_event_t * e)
{
	lv_obj_t *tileview = lv_event_get_current_target(e);
	view_data_t *view_data = lv_event_get_user_data(e);
	clock_selector_view_data_t *data = view_data->user_data;

	int8_t preload_idx = -1;
	int8_t src_idx;

	lv_obj_t * tile = lv_tileview_get_tile_act(tileview);
	int tile_x = (int)lv_obj_get_user_data(tile);

	if (tile_x < data->sel_idx) {
		preload_idx = tile_x - 2;
		src_idx = (data->sel_src < 2) ? (data->sel_src + 1) : 0;
		_load_element_resource(view_data, tile_x - 1, src_idx);

		if (--data->sel_src < 0)
			data->sel_src = 2;
	} else if (tile_x > data->sel_idx) {
		preload_idx = tile_x + 2;
		src_idx = (data->sel_src > 0) ? (data->sel_src - 1) : 2;
		_load_element_resource(view_data, tile_x + 1, src_idx);

		if (++data->sel_src > 2)
			data->sel_src = 0;
	}

	data->sel_idx = tile_x;
	SYS_LOG_INF("preview clock %d", data->sel_idx);

#ifdef PRELOAD_5_PICS
	if (preload_idx >= 0 && preload_idx < data->picreg.frames) {
		lvgl_res_preload_pictures_from_picregion(0, &data->picreg, preload_idx, preload_idx);
	}
#endif
}

static void _clock_selector_click_event_handler(lv_event_t * e)
{
	view_data_t *view_data = lv_event_get_user_data(e);

	const clock_selector_view_presenter_t *presenter = view_get_presenter(view_data);
	clock_selector_view_data_t *data = view_data->user_data;

	SYS_LOG_INF("select clock %d", data->sel_idx);

	if (presenter->set_clock_id) {
		presenter->set_clock_id(data->sel_idx);
	}
}

static int _clock_selector_view_layout(view_data_t *view_data)
{
	lv_obj_t *scr = lv_display_get_screen_active(view_data->display);
	clock_selector_view_data_t *data = view_data->user_data;
	int i;

	data->tileview = lv_tileview_create(scr);
	if (data->tileview == NULL) {
		return -ENOMEM;
	}

	lv_style_init(&data->sty_bg);
	lv_style_set_bg_color(&data->sty_bg, data->scene.background);
	lv_style_set_bg_opa(&data->sty_bg, LV_OPA_COVER);

	lv_obj_add_style(data->tileview, &data->sty_bg, LV_PART_MAIN);
	lv_obj_set_user_data(data->tileview, data);
	lv_obj_add_event_cb(data->tileview, _clock_selector_tileview_event_handler, LV_EVENT_SCROLL_END, view_data);

	if (data->txt) {
		lv_style_init(&data->sty_txt);
		lv_style_set_text_font(&data->sty_txt, &data->font);
		lv_style_set_text_color(&data->sty_txt, data->strtxt[0].color);
		lv_style_set_text_align(&data->sty_txt, LV_TEXT_ALIGN_CENTER);
		lv_style_set_bg_opa(&data->sty_txt, LV_OPA_TRANSP);
		lv_style_set_align(&data->sty_txt, LV_ALIGN_TOP_MID);
	}

	for (i = 0; i < data->picreg.frames; i++) {
		lv_obj_t *tile = lv_tileview_add_tile(data->tileview, i, 0, LV_DIR_HOR);
		if (tile == NULL) {
			return -ENOMEM;
		}

		lv_obj_set_user_data(tile, (void *)i);

		data->img[i] = simple_img_create(tile);
		if (data->img[i] == NULL) {
			return -ENOMEM;
		}

		lv_obj_add_style(data->img[i], &data->sty_bg, LV_PART_MAIN);
		lv_obj_add_flag(data->img[i], LV_OBJ_FLAG_CLICKABLE);
		lv_obj_set_pos(data->img[i], data->picreg.x, data->picreg.y);
		lv_obj_set_size(data->img[i], data->picreg.width, data->picreg.height);
		lv_obj_set_user_data(data->img[i], view_data);
		lv_obj_add_event_cb(data->img[i], _clock_selector_click_event_handler, LV_EVENT_SHORT_CLICKED, view_data);

		if (data->txt) {
			data->txt[i] = lv_label_create(tile);
			if (data->txt[i] == NULL) {
				return -ENOMEM;
			}

			lv_obj_add_style(data->txt[i], &data->sty_txt, LV_PART_MAIN);
		}
	}

	data->sel_src = 1;
	_load_element_resource(view_data, data->sel_idx, data->sel_src);
	_load_element_resource(view_data, data->sel_idx - 1, data->sel_src - 1);
	_load_element_resource(view_data, data->sel_idx + 1, data->sel_src + 1);

	lv_obj_update_layout(data->tileview);
	lv_obj_set_tile_id(data->tileview, data->sel_idx, 0, false);

	/* filter out all system gestures */
	ui_gesture_lock_scroll();

	SYS_LOG_INF("clock selector view inflated");
	return 0;
}

static int _clock_selector_view_delete(view_data_t *view_data)
{
	clock_selector_view_data_t *data = view_data->user_data;

	if (data->tileview) {
		lv_obj_delete(data->tileview);
		lv_style_reset(&data->sty_bg);
		if (data->txt) {
			lv_style_reset(&data->sty_txt);
		}

		/* recovery all system gestures */
		ui_gesture_unlock_scroll();
	}

	_clock_selector_view_unload_resource(data);
	app_mem_free(data->img);
	app_mem_free(data->txt);
	app_mem_free(data);
	return 0;
}

static int _clock_selector_view_proc_key(view_data_t *view_data, ui_key_msg_data_t * key_data)
{
	clock_selector_view_data_t *data = view_data->user_data;
	if (data == NULL)
		return 0;

	if (KEY_VALUE(key_data->event) == KEY_GESTURE_RIGHT)
		key_data->done = true;

	return 0;
}

int clock_selector_view_handler(uint16_t view_id, view_data_t *view_data, uint8_t msg_id, void *msg_data)
{
	assert(view_id == CLOCK_SELECTOR_VIEW);

	switch (msg_id) {
	case MSG_VIEW_PRELOAD:
		return _clock_selector_view_preload(view_data);
	case MSG_VIEW_LAYOUT:
		return _clock_selector_view_layout(view_data);
	case MSG_VIEW_DELETE:
		return _clock_selector_view_delete(view_data);
	case MSG_VIEW_KEY:
		return _clock_selector_view_proc_key(view_data, msg_data);
	case MSG_VIEW_PAINT:
	default:
		return 0;
	}
}

VIEW_DEFINE2(clock_selector, clock_selector_view_handler, NULL,
		NULL, CLOCK_SELECTOR_VIEW, NORMAL_ORDER, UI_VIEW_LVGL, DEF_UI_VIEW_WIDTH, DEF_UI_VIEW_HEIGHT);
