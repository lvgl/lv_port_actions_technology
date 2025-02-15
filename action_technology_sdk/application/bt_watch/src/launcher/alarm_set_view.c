/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief bt call view
 */
#include <stddef.h>
#include <stdint.h>
#include <ui_manager.h>
#include <lvgl/lvgl_res_loader.h>
#include "launcher_app.h"
#include "app_ui.h"
#include "widgets/img_number.h"
#ifdef CONFIG_ALARM_MANAGER
#include <alarm_manager.h>
#endif
#ifdef CONFIG_PROPERTY
#include <property_manager.h>
#endif

#define ALARM_SET_CNT	3

/* alarm bg view */
enum {
	BMP_ALARM_S_ICON = 0,
	BMP_ALARM_S_NAME,

	NUM_ALARM_SET_BG_IMGS,
};
/* alarm main view */
enum {
	BTN_TM1_COLON_ON_OFF = 0,
	BTN_TM1_ON_OFF,
	BTN_TM2_COLON_ON_OFF,
	BTN_TM2_ON_OFF,
	BTN_TM3_COLON_ON_OFF,
	BTN_TM3_ON_OFF,
	BTN_ADD,

	NUM_ALARM_MAIN_BTNS,
};
/* alarm bg view */
static const uint32_t alarm_set_bg_bmp_ids[] = {
	PIC_ALARM_ICON,
	PIC_ALARM_NAME,
};
/* alarm main view */
static const uint32_t alarm_main_btn_def_ids[] = {
	PIC_COLON_OFF1,
	PIC_BTN_OFF1,
	PIC_COLON_OFF2,
	PIC_BTN_OFF2,
	PIC_COLON_OFF3,
	PIC_BTN_OFF3,
	PIC_BTN_ADD,
};
static const uint32_t alarm_main_btn_sel_ids[] = {
	PIC_COLON_ON1,
	PIC_BTN_ON1,
	PIC_COLON_ON2,
	PIC_BTN_ON2,
	PIC_COLON_ON3,
	PIC_BTN_ON3,
	PIC_BTN_ADD,
};
/* alarm sub view */
enum {
	BTN_H_UP = 0,
	BTN_H_DP,
	BTN_H_DW,
	BTN_M_UP,
	BTN_M_DP,
	BTN_M_DW,
	BTN_OK,

	NUM_ALARM_SUB_BTNS,
};
static const uint32_t alarm_sub_btn_ids[] = {
	PIC_BTN_H_UPP,
	PIC_H_BG,
	PIC_BTN_H_DOWN,
	PIC_BTN_M_UPP,
	PIC_M_BG,
	PIC_BTN_M_DOWN,
	PIC_BTN_OK,
};
static const uint32_t pic_tm_ch_ids[] = {
	PIC_NUM_L_0, PIC_NUM_L_1, PIC_NUM_L_2, PIC_NUM_L_3, PIC_NUM_L_4, PIC_NUM_L_5, PIC_NUM_L_6, PIC_NUM_L_7, PIC_NUM_L_8, PIC_NUM_L_9,
};
static const uint32_t pic_tm_unch_ids[] = {
	PIC_NUM_0, PIC_NUM_1, PIC_NUM_2, PIC_NUM_3, PIC_NUM_4, PIC_NUM_5, PIC_NUM_6, PIC_NUM_7, PIC_NUM_8, PIC_NUM_9,
};
static const uint32_t res_grp_h_chose_id[] = {
	RES_ALARM_H_CHOSE,
};
static const uint32_t res_grp_m_chose_id[] = {
	RES_ALARM_M_CHOSE,
};
static const uint32_t res_grp_h_unchose_id[] = {
	RES_ALARM_H_UNCHOSE,
};
static const uint32_t res_grp_m_unchose_id[] = {
	RES_ALARM_M_UNCHOSE,
};

/* alarm main view */

static const uint32_t pic_tm_off_ids[] = {
	PIC_H_OFF_0, PIC_H_OFF_1, PIC_H_OFF_2, PIC_H_OFF_3, PIC_H_OFF_4, PIC_H_OFF_5, PIC_H_OFF_6, PIC_H_OFF_7, PIC_H_OFF_8, PIC_H_OFF_9,
};
static const uint32_t pic_tm_on_ids[] = {
	PIC_H_ON_0, PIC_H_ON_1, PIC_H_ON_2, PIC_H_ON_3, PIC_H_ON_4, PIC_H_ON_5, PIC_H_ON_6, PIC_H_ON_7, PIC_H_ON_8, PIC_H_ON_9,
};
static const uint32_t res_grp_hour_off_id[] = {
	RES_ALARM_H_ONE_OFF,
	RES_ALARM_H_TWO_OFF,
	RES_ALARM_H_THR_OFF,
};
static const uint32_t res_grp_min_off_id[] = {
	RES_ALARM_M_ONE_OFF,
	RES_ALARM_M_TWO_OFF,
	RES_ALARM_M_THR_OFF,
};
static const uint32_t res_grp_hour_on_id[] = {
	RES_ALARM_H_ONE_ON,
	RES_ALARM_H_TWO_ON,
	RES_ALARM_H_THR_ON,
};
static const uint32_t res_grp_min_on_id[] = {
	RES_ALARM_M_ONE_ON,
	RES_ALARM_M_TWO_ON,
	RES_ALARM_M_THR_ON,
};

typedef struct alarm_list_view_tmp_res {
	lvgl_res_group_t grp_hour_off[ALARM_SET_CNT];
	lvgl_res_group_t grp_min_off[ALARM_SET_CNT];
	lvgl_res_group_t grp_hour_on[ALARM_SET_CNT];
	lvgl_res_group_t grp_min_on[ALARM_SET_CNT];

} alarm_list_view_tmp_res_t;

typedef struct alarm_tm_res {
	lvgl_res_group_t grp_h_ch;
	lvgl_res_group_t grp_m_ch;
	lvgl_res_group_t grp_h_unch;
	lvgl_res_group_t grp_m_unch;
} alarm_tm_res_t;

typedef struct alarm_main_view_data {
	lv_obj_t *obj_main_btn[NUM_ALARM_MAIN_BTNS];
	lv_obj_t *obj_tm_hour_on[ALARM_SET_CNT];
	lv_obj_t *obj_tm_hour_off[ALARM_SET_CNT];
	lv_obj_t *obj_tm_min_on[ALARM_SET_CNT];
	lv_obj_t *obj_tm_min_off[ALARM_SET_CNT];

	/* lvgl resource */
	lv_image_dsc_t img_dsc_def_main_btn[NUM_ALARM_MAIN_BTNS];
	lv_image_dsc_t img_dsc_sel_main_btn[NUM_ALARM_MAIN_BTNS];

	lv_image_dsc_t img_dsc_off_num[10];
	lv_image_dsc_t img_dsc_on_num[10];

	lv_point_t pt_main_btn[NUM_ALARM_MAIN_BTNS];
	alarm_list_view_tmp_res_t resource;
} alarm_main_view_data_t;

typedef struct alarm_sub_view_data {
	lv_obj_t *obj_sub_btn[NUM_ALARM_MAIN_BTNS];
	lv_obj_t *obj_sub_h_ch;
	lv_obj_t *obj_sub_h_unch;
	lv_obj_t *obj_sub_m_ch;
	lv_obj_t *obj_sub_m_unch;

	/* lvgl resource */
	lv_image_dsc_t img_dsc_sub_btn[NUM_ALARM_SUB_BTNS];
	lv_image_dsc_t img_dsc_ch_num[10];
	lv_image_dsc_t img_dsc_unch_num[10];

	lv_point_t pt_sub_btn[NUM_ALARM_SUB_BTNS];
	alarm_tm_res_t resource;
	uint8_t al_index;
} alarm_sub_view_data_t;

typedef struct alarm_set_view_data {
	lv_obj_t *obj_alarm_bg;
	lv_obj_t *obj_set_bg[NUM_ALARM_SET_BG_IMGS];
	/* lvgl resource */
	lv_image_dsc_t img_dsc_set_bg[NUM_ALARM_SET_BG_IMGS];
	lv_point_t pt_set_bg[NUM_ALARM_SET_BG_IMGS];
	lvgl_res_scene_t res_scene;

	/* user data */
	alarm_main_view_data_t *main_view;
	alarm_sub_view_data_t *sub_view;
	uint16_t tm_min[ALARM_SET_CNT];
	uint8_t tm_state[ALARM_SET_CNT];/* 0--null, 1--off, 2--on*/

} alarm_set_view_data_t;

static alarm_set_view_data_t *p_alarm_set_data = NULL;
static void _add_alarm(alarm_set_view_data_t *data);
static void _exit_alarm_main_view(alarm_set_view_data_t *data);
static void _display_alarm_sub_view(alarm_set_view_data_t *data, uint8_t i);
static void _display_alarm_list_one(alarm_set_view_data_t *data, lv_obj_t *par, uint8_t index);
static void _display_alarm_time(alarm_set_view_data_t *data, lv_obj_t *par, uint16_t min, bool h_is_ch, bool m_is_ch);
static void _display_alarm_main_view(alarm_set_view_data_t *data);
static void _exit_alarm_sub_view(alarm_set_view_data_t *data);
extern void alarm_event_cb(void);
static void alarm_set_btn_state_toggle(lv_obj_t * btn)
{
    if (lv_obj_has_state(btn, LV_STATE_CHECKED)) {
        lv_obj_clear_state(btn, LV_STATE_CHECKED);
    } else {
        lv_obj_add_state(btn, LV_STATE_CHECKED);
    }
}

static void _alarm_set_delete_obj_array(lv_obj_t **pobj, uint32_t num)
{
	for (int i = 0; i < num; i++) {
		if (pobj[i]) {
			lv_obj_delete(pobj[i]);
			pobj[i] = NULL;
		}
	}
}
static void _img_number_event_handler(lv_event_t *e)
{
	lv_event_code_t event = lv_event_get_code(e);
	lv_obj_t *obj = lv_event_get_current_target(e);

	if (!p_alarm_set_data || !p_alarm_set_data->main_view)
		return;

	if (event == LV_EVENT_CLICKED) {
		for (int i = 0; i < ALARM_SET_CNT; i++) {
			if (p_alarm_set_data->main_view->obj_tm_hour_off[i] == obj ||
				p_alarm_set_data->main_view->obj_tm_hour_on[i] == obj ||
				p_alarm_set_data->main_view->obj_tm_min_off[i] == obj ||
				p_alarm_set_data->main_view->obj_tm_hour_on[i] == obj) {
				_exit_alarm_main_view(p_alarm_set_data);
				_display_alarm_sub_view(p_alarm_set_data, i);
			}
		}
	}
}
static void _alarm_bg_event_handler(lv_event_t * e)
{
	lv_event_code_t event = lv_event_get_code(e);
	lv_obj_t *obj = lv_event_get_current_target(e);

	if (event == LV_EVENT_GESTURE) {
		SYS_LOG_INF("%p draw\n", obj);
		if (p_alarm_set_data && p_alarm_set_data->obj_alarm_bg == obj && p_alarm_set_data->sub_view) {
			_exit_alarm_sub_view(p_alarm_set_data);
			_display_alarm_main_view(p_alarm_set_data);
			lv_indev_wait_release(lv_indev_get_act());
		}
	} else if (event == LV_EVENT_VALUE_CHANGED) {
		SYS_LOG_INF("%p Toggled\n", obj);
	}
}

static void _alarm_set_btn_evt_handler(lv_event_t *e)
{
	lv_event_code_t event = lv_event_get_code(e);
	lv_obj_t *obj = lv_event_get_current_target(e);
	lv_state_t btn_state;
	uint32_t time_sec = 0;
	lv_obj_t *par;
	alarm_sub_view_data_t *sview;
	
	if (!p_alarm_set_data)
		return;

	par = p_alarm_set_data->obj_alarm_bg;
	if (!par)
		return;
	
	sview = p_alarm_set_data->sub_view;

	if (event == LV_EVENT_CLICKED) {
		if (p_alarm_set_data->main_view) {
			if (p_alarm_set_data->main_view->obj_main_btn[BTN_ADD] == obj) {
				_add_alarm(p_alarm_set_data);
			} else {
				for (int i = 0; i < ALARM_SET_CNT; i++) {
					if (p_alarm_set_data->main_view->obj_main_btn[i * 2] == obj) {
						_exit_alarm_main_view(p_alarm_set_data);
						_display_alarm_sub_view(p_alarm_set_data, i);
						break;
					} else if (p_alarm_set_data->main_view->obj_main_btn[i * 2 + 1] == obj) {
						btn_state = lv_obj_get_state(obj);
						if (btn_state == LV_STATE_PRESSED || ((btn_state & (LV_STATE_CHECKED | LV_STATE_PRESSED)) == LV_STATE_CHECKED)) {
							p_alarm_set_data->tm_state[i] = ALARM_STATE_FREE;
						} else {
							p_alarm_set_data->tm_state[i] = ALARM_STATE_OFF;
						}
						time_sec = p_alarm_set_data->tm_min[i] * 60;
					#ifdef CONFIG_ALARM_MANAGER
						alarm_manager_update_alarm(time_sec, i, p_alarm_set_data->tm_state[i]);
					#endif
						/* toggle time and : from on <> off */
						_display_alarm_list_one(p_alarm_set_data, par, i);
						break;
					}
				}
			}
		} else if (sview) {
			if (sview->obj_sub_btn[BTN_H_UP] == obj) {
				if (p_alarm_set_data->tm_min[sview->al_index] / 60 < 23) {/*22:XX*/
					p_alarm_set_data->tm_min[sview->al_index] += 60;
				} else {/*00:XX*/
					p_alarm_set_data->tm_min[sview->al_index] %= 60;
				}
				_display_alarm_time(p_alarm_set_data, par, p_alarm_set_data->tm_min[sview->al_index], true, false);
			} else if (sview->obj_sub_btn[BTN_H_DW] == obj) {
				if (p_alarm_set_data->tm_min[sview->al_index] / 60 > 0) {/*01:XX*/
					p_alarm_set_data->tm_min[sview->al_index] -= 60;
				} else {/*23:XX*/
					p_alarm_set_data->tm_min[sview->al_index] += 23 * 60;
				}
				_display_alarm_time(p_alarm_set_data, par, p_alarm_set_data->tm_min[sview->al_index], true, false);
			} else if (sview->obj_sub_btn[BTN_M_UP] == obj) {
				if (p_alarm_set_data->tm_min[sview->al_index] % 60 < 59) {/*XX:58*/
					p_alarm_set_data->tm_min[sview->al_index]++;
				} else {/*XX:00*/
					p_alarm_set_data->tm_min[sview->al_index] /= 60;
					p_alarm_set_data->tm_min[sview->al_index] *= 60;
				}
				_display_alarm_time(p_alarm_set_data, par, p_alarm_set_data->tm_min[sview->al_index], false, true);
			} else if (sview->obj_sub_btn[BTN_M_DW] == obj) {
				if (p_alarm_set_data->tm_min[sview->al_index] % 60 > 0) {/*XX:01*/
					p_alarm_set_data->tm_min[sview->al_index]--;
				} else {/*XX:59*/
					p_alarm_set_data->tm_min[sview->al_index] += 59;
				}
				_display_alarm_time(p_alarm_set_data, par, p_alarm_set_data->tm_min[sview->al_index], false, true);
			} else if (sview->obj_sub_btn[BTN_OK] == obj) {
				SYS_LOG_INF("btn ok\n");
			#ifdef CONFIG_ALARM_MANAGER
				alarm_manager_update_alarm(p_alarm_set_data->tm_min[sview->al_index] * 60, sview->al_index, p_alarm_set_data->tm_state[sview->al_index]);
			#endif
				_exit_alarm_sub_view(p_alarm_set_data);
				_display_alarm_main_view(p_alarm_set_data);
			}
		}
		SYS_LOG_INF("Clicked obj %p\n", obj);
	} else if (event == LV_EVENT_VALUE_CHANGED) {
		SYS_LOG_INF("Toggled\n");
	}
}
static void _alarm_set_create_btn(lv_obj_t *par, lv_obj_t **pobj, lv_point_t *pt,
										lv_image_dsc_t *def, lv_image_dsc_t *sel)
{
	*pobj = lv_imagebutton_create(par);
	lv_obj_set_pos(*pobj, pt->x, pt->y);
	lv_obj_set_size(*pobj, def->header.w, def->header.h);
	lv_obj_add_event_cb(*pobj, _alarm_set_btn_evt_handler, LV_EVENT_ALL, NULL);

	lv_imagebutton_set_src(*pobj, LV_IMAGEBUTTON_STATE_RELEASED, NULL, def, NULL);
	lv_imagebutton_set_src(*pobj, LV_IMAGEBUTTON_STATE_PRESSED, NULL, sel, NULL);
	lv_imagebutton_set_src(*pobj, LV_IMAGEBUTTON_STATE_CHECKED_RELEASED, NULL, sel, NULL);
	lv_imagebutton_set_src(*pobj, LV_IMAGEBUTTON_STATE_CHECKED_PRESSED, NULL, def, NULL);

	lv_obj_set_ext_click_area(*pobj, lv_obj_get_style_width(*pobj, LV_PART_MAIN));
}

static void _alarm_set_create_img_array(lv_obj_t *par, lv_obj_t **pobj, lv_point_t *pt,
										lv_image_dsc_t *img, uint32_t num)
{
	for (int i = 0; i < num; i++) {
		pobj[i] = lv_image_create(par);
		lv_image_set_src(pobj[i], &img[i]);
		lv_obj_set_pos(pobj[i], pt[i].x, pt[i].y);
	}
}
static void _add_alarm(alarm_set_view_data_t *bg_view_data)
{
	int i = 0;
	uint32_t time_sec = 0;
	alarm_main_view_data_t *data;

	if (!bg_view_data)
		return;

	data = bg_view_data->main_view;
	if (!data)
		return;

	for (i = 0; i < ALARM_SET_CNT; i++) {
		if (bg_view_data->tm_state[i] == ALARM_STATE_NULL)
			break;
	}
	if (i < ALARM_SET_CNT) {
		data->obj_tm_hour_off[i] = img_number_create(bg_view_data->obj_alarm_bg);
		lv_obj_set_pos(data->obj_tm_hour_off[i], data->resource.grp_hour_off[i].x, data->resource.grp_hour_off[i].y);
		lv_obj_set_size(data->obj_tm_hour_off[i], data->resource.grp_hour_off[i].width, data->resource.grp_hour_off[i].height);
		img_number_set_src(data->obj_tm_hour_off[i], data->img_dsc_off_num, 10);
		img_number_set_align(data->obj_tm_hour_off[i], LV_ALIGN_LEFT_MID);
		img_number_set_value(data->obj_tm_hour_off[i], bg_view_data->tm_min[i] / 60, 2);
		/* display min*/
		data->obj_tm_min_off[i] = img_number_create(bg_view_data->obj_alarm_bg);
		lv_obj_set_pos(data->obj_tm_min_off[i], data->resource.grp_min_off[i].x, data->resource.grp_min_off[i].y);
		lv_obj_set_size(data->obj_tm_min_off[i], data->resource.grp_min_off[i].width, data->resource.grp_min_off[i].height);
		img_number_set_src(data->obj_tm_min_off[i], data->img_dsc_off_num, 10);
		img_number_set_align(data->obj_tm_min_off[i], LV_ALIGN_LEFT_MID);
		img_number_set_value(data->obj_tm_min_off[i], bg_view_data->tm_min[i] % 60, 2);
		/* display :*/
		_alarm_set_create_btn(bg_view_data->obj_alarm_bg, &data->obj_main_btn[2 * i], &data->pt_main_btn[2 * i], &data->img_dsc_def_main_btn[2 * i], &data->img_dsc_sel_main_btn[2 * i]);
		/* display off btn*/
		_alarm_set_create_btn(bg_view_data->obj_alarm_bg, &data->obj_main_btn[2 * i + 1], &data->pt_main_btn[2 * i + 1], &data->img_dsc_def_main_btn[2 * i + 1], &data->img_dsc_sel_main_btn[2 * i + 1]);
		lv_obj_add_flag(data->obj_main_btn[2 * i + 1], LV_OBJ_FLAG_CHECKABLE);
		bg_view_data->tm_state[i] = ALARM_STATE_OFF;
		time_sec = bg_view_data->tm_min[i] * 60;
	#ifdef CONFIG_ALARM_MANAGER
		alarm_manager_update_alarm(time_sec, i, bg_view_data->tm_state[i]);
	#endif
	}
}

static void _alarm_sub_view_unload_group_from_scene(alarm_sub_view_data_t *sub_view)
{
	lvgl_res_unload_pictures(sub_view->img_dsc_ch_num, ARRAY_SIZE(sub_view->img_dsc_ch_num));
	lvgl_res_unload_pictures(sub_view->img_dsc_unch_num, ARRAY_SIZE(sub_view->img_dsc_unch_num));
}

static void _alarm_list_unload_group_from_scene(alarm_main_view_data_t *main_view)
{
	lvgl_res_unload_pictures(main_view->img_dsc_off_num, ARRAY_SIZE(main_view->img_dsc_off_num));
	lvgl_res_unload_pictures(main_view->img_dsc_on_num, ARRAY_SIZE(main_view->img_dsc_on_num));
}
static void _alarm_set_unload_resource(alarm_set_view_data_t *data)
{
	lvgl_res_unload_pictures(data->img_dsc_set_bg, NUM_ALARM_SET_BG_IMGS);

	lvgl_res_unload_scene(&data->res_scene);
}

static int _alarm_set_load_resource(alarm_set_view_data_t *data)
{
	int ret;

	/* scene */
	ret = lvgl_res_load_scene(SCENE_ALARM_SET_VIEW, &data->res_scene,
			DEF_STY_FILE, DEF_RES_FILE, DEF_STR_FILE);
	if (ret < 0) {
		SYS_LOG_ERR("SCENE_ALARM_SET_VIEW not found");
		return -ENOENT;
	}

	/* background picture */
	ret = lvgl_res_load_pictures_from_scene(&data->res_scene, alarm_set_bg_bmp_ids, data->img_dsc_set_bg, data->pt_set_bg, NUM_ALARM_SET_BG_IMGS);
	if (ret < 0) {
		goto out_exit;
	}

out_exit:
	if (ret < 0) {
		_alarm_set_unload_resource(data);
	}

	return ret;
}
static int _alarm_sub_view_load_group_from_scene(alarm_set_view_data_t *data, alarm_tm_res_t *tmp_res)
{
	int ret = 0;

	ret = lvgl_res_load_group_from_scene(&data->res_scene, res_grp_h_chose_id[0], &tmp_res->grp_h_ch);
	if (ret < 0) {
		goto fail_exit;
	}
	ret = lvgl_res_load_pictures_from_group(&tmp_res->grp_h_ch, pic_tm_ch_ids,
			data->sub_view->img_dsc_ch_num, NULL, ARRAY_SIZE(pic_tm_ch_ids));
	lvgl_res_unload_group(&tmp_res->grp_h_ch);

	ret = lvgl_res_load_group_from_scene(&data->res_scene, res_grp_m_chose_id[0], &tmp_res->grp_m_ch);
	if (ret < 0) {
		goto fail_exit;
	}
	lvgl_res_unload_group(&tmp_res->grp_m_ch);

	ret = lvgl_res_load_group_from_scene(&data->res_scene, res_grp_h_unchose_id[0], &tmp_res->grp_h_unch);
	if (ret < 0) {
		goto fail_exit;
	}
	ret = lvgl_res_load_pictures_from_group(&tmp_res->grp_h_unch, pic_tm_unch_ids,
			data->sub_view->img_dsc_unch_num, NULL, ARRAY_SIZE(pic_tm_unch_ids));
	lvgl_res_unload_group(&tmp_res->grp_h_unch);

	ret = lvgl_res_load_group_from_scene(&data->res_scene, res_grp_m_unchose_id[0], &tmp_res->grp_m_unch);
	if (ret < 0) {
		goto fail_exit;
	}

	lvgl_res_unload_group(&tmp_res->grp_m_unch);
	return ret;
fail_exit:
	_alarm_list_unload_group_from_scene(data->main_view);
	return ret;
}

static int _alarm_list_load_group_from_scene(alarm_set_view_data_t *data, alarm_list_view_tmp_res_t *tmp_res)
{
	int ret = 0;

	for (int i = 0; i < ALARM_SET_CNT; i++) {
		ret = lvgl_res_load_group_from_scene(&data->res_scene, res_grp_hour_off_id[i], &tmp_res->grp_hour_off[i]);
		if (ret < 0) {
			goto fail_exit;
		}
		ret = lvgl_res_load_pictures_from_group(&tmp_res->grp_hour_off[i], pic_tm_off_ids,
				data->main_view->img_dsc_off_num, NULL, ARRAY_SIZE(pic_tm_off_ids));
		lvgl_res_unload_group(&tmp_res->grp_hour_off[i]);

		ret = lvgl_res_load_group_from_scene(&data->res_scene, res_grp_min_off_id[i], &tmp_res->grp_min_off[i]);
		if (ret < 0) {
			goto fail_exit;
		}

		lvgl_res_unload_group(&tmp_res->grp_min_off[i]);

		ret = lvgl_res_load_group_from_scene(&data->res_scene, res_grp_hour_on_id[i], &tmp_res->grp_hour_on[i]);
		if (ret < 0) {
			goto fail_exit;
		}
		ret = lvgl_res_load_pictures_from_group(&tmp_res->grp_hour_on[i], pic_tm_on_ids,
				data->main_view->img_dsc_on_num, NULL, ARRAY_SIZE(pic_tm_on_ids));
		lvgl_res_unload_group(&tmp_res->grp_hour_on[i]);

		ret = lvgl_res_load_group_from_scene(&data->res_scene, res_grp_min_on_id[i], &tmp_res->grp_min_on[i]);
		if (ret < 0) {
			goto fail_exit;
		}

		lvgl_res_unload_group(&tmp_res->grp_min_on[i]);
	}
	return ret;
fail_exit:
	_alarm_list_unload_group_from_scene(data->main_view);
	return ret;
}
static void _exit_alarm_sub_view_btn(alarm_sub_view_data_t *view)
{
	if (!view)
		return;
	lvgl_res_unload_pictures(view->img_dsc_sub_btn, NUM_ALARM_SUB_BTNS);
	for (int i = 0; i < NUM_ALARM_SUB_BTNS; i++) {
		if (view->obj_sub_btn[i])
			lv_obj_delete(view->obj_sub_btn[i]);
		view->obj_sub_btn[i] = NULL;
	}
}
static void _exit_alarm_sub_view_time(alarm_sub_view_data_t *view)
{
	if (!view)
		return;
	_alarm_sub_view_unload_group_from_scene(view);

	if (view->obj_sub_h_ch) {
		lv_obj_delete(view->obj_sub_h_ch);
		view->obj_sub_h_ch = NULL;
	}
	if (view->obj_sub_h_unch) {
		lv_obj_delete(view->obj_sub_h_unch);
		view->obj_sub_h_unch = NULL;
	}
	if (view->obj_sub_m_ch) {
		lv_obj_delete(view->obj_sub_m_ch);
		view->obj_sub_m_ch = NULL;
	}
	if (view->obj_sub_m_unch) {
		lv_obj_delete(view->obj_sub_m_unch);
		view->obj_sub_m_unch = NULL;
	}
}

static void _exit_alarm_sub_view(alarm_set_view_data_t *data)
{
	_exit_alarm_sub_view_btn(data->sub_view);
	_exit_alarm_sub_view_time(data->sub_view);
	if (data->sub_view)
		app_mem_free(data->sub_view);
	data->sub_view = NULL;

	ui_gesture_wait_release();
	ui_gesture_unlock_scroll();
}

static void _exit_alarm_main_view_btn(alarm_main_view_data_t *main_view)
{
	if (!main_view)
		return;
	lvgl_res_unload_pictures(main_view->img_dsc_def_main_btn, NUM_ALARM_MAIN_BTNS);
	lvgl_res_unload_pictures(main_view->img_dsc_sel_main_btn, NUM_ALARM_MAIN_BTNS);
	for (int i = 0; i < NUM_ALARM_MAIN_BTNS; i++) {
		if (main_view->obj_main_btn[i])
			lv_obj_delete(main_view->obj_main_btn[i]);
		main_view->obj_main_btn[i] = NULL;
	}
}
static void _exit_alarm_main_view_list(alarm_main_view_data_t *view)
{
	if (!view)
		return;
	_alarm_list_unload_group_from_scene(view);

	for (int i = 0; i < ALARM_SET_CNT; i++) {
		if (view->obj_tm_hour_off[i]) {
			lv_obj_delete(view->obj_tm_hour_off[i]);
			view->obj_tm_hour_off[i] = NULL;
		}
		if (view->obj_tm_min_off[i]) {
			lv_obj_delete(view->obj_tm_min_off[i]);
			view->obj_tm_min_off[i] = NULL;
		}
		if (view->obj_tm_hour_on[i]) {
			lv_obj_delete(view->obj_tm_hour_on[i]);
			view->obj_tm_hour_on[i] = NULL;
		}

		if (view->obj_tm_min_on[i]) {
			lv_obj_delete(view->obj_tm_min_on[i]);
			view->obj_tm_min_on[i] = NULL;
		}
		/*del colon*/
		if (view->obj_main_btn[2 * i]) {
			lv_obj_delete(view->obj_main_btn[2 * i]);
			view->obj_main_btn[2 * i] = NULL;
		}
		/*del alarm btn*/
		if (view->obj_main_btn[2 * i + 1]) {
			lv_obj_delete(view->obj_main_btn[2 * i + 1]);
			view->obj_main_btn[2 * i + 1] = NULL;
		}
	}

}

static void _exit_alarm_main_view(alarm_set_view_data_t *data)
{
	_exit_alarm_main_view_btn(data->main_view);
	_exit_alarm_main_view_list(data->main_view);
	if (data->main_view)
		app_mem_free(data->main_view);
	data->main_view = NULL;
}
static void _display_sub_view_btn(alarm_set_view_data_t *data)
{
	int ret = 0;
	alarm_sub_view_data_t *sub_view = NULL;

	if (!data)
		return;

	sub_view = data->sub_view;

	if(!sub_view)
		return;

	/* btn picture */
	ret = lvgl_res_load_pictures_from_scene(&data->res_scene, alarm_sub_btn_ids, sub_view->img_dsc_sub_btn, sub_view->pt_sub_btn, NUM_ALARM_SUB_BTNS);
	if (ret < 0) {
		return;
	}
	ret = lvgl_res_load_pictures_from_scene(&data->res_scene, alarm_sub_btn_ids, sub_view->img_dsc_sub_btn, sub_view->pt_sub_btn, NUM_ALARM_SUB_BTNS);
	if (ret < 0) {
		lvgl_res_unload_pictures(sub_view->img_dsc_sub_btn, NUM_ALARM_SUB_BTNS);
		return;
	}
	for (int i = 0; i < NUM_ALARM_SUB_BTNS; i++)
		_alarm_set_create_btn(data->obj_alarm_bg, &sub_view->obj_sub_btn[i], &sub_view->pt_sub_btn[i], &sub_view->img_dsc_sub_btn[i], &sub_view->img_dsc_sub_btn[i]);
	lv_obj_set_ext_click_area(sub_view->obj_sub_btn[BTN_H_DP], 0);
	lv_obj_set_ext_click_area(sub_view->obj_sub_btn[BTN_M_DP], 0);
}
static void _display_alarm_time(alarm_set_view_data_t *data, lv_obj_t *par, uint16_t min, bool h_is_ch, bool m_is_ch)
{
	if (!data)
		return;
	
	alarm_sub_view_data_t *sub_view = data->sub_view;
	if (!sub_view)
		return;
	
	SYS_LOG_INF("min %d h_is_ch %d m_is_ch %d", min, h_is_ch, m_is_ch);
	/* display hour*/
	if (h_is_ch) {
		if (sub_view->obj_sub_h_unch) {
			lv_obj_delete(sub_view->obj_sub_h_unch);
			sub_view->obj_sub_h_unch = NULL;
		}

		if (sub_view->obj_sub_h_ch) {
			img_number_set_value(sub_view->obj_sub_h_ch, min / 60, 2);
		} else {
			sub_view->obj_sub_h_ch = img_number_create(par);
			lv_obj_set_pos(sub_view->obj_sub_h_ch, sub_view->resource.grp_h_ch.x, sub_view->resource.grp_h_ch.y);
			lv_obj_set_size(sub_view->obj_sub_h_ch, sub_view->resource.grp_h_ch.width, sub_view->resource.grp_h_ch.height);
			img_number_set_src(sub_view->obj_sub_h_ch, sub_view->img_dsc_ch_num, 10);
			img_number_set_align(sub_view->obj_sub_h_ch, LV_ALIGN_LEFT_MID);
			img_number_set_value(sub_view->obj_sub_h_ch, min / 60, 2);
		}
	} else {
		if (sub_view->obj_sub_h_ch) {
			lv_obj_delete(sub_view->obj_sub_h_ch);
			sub_view->obj_sub_h_ch = NULL;
		}

		if (sub_view->obj_sub_h_unch) {
			img_number_set_value(sub_view->obj_sub_h_unch, min / 60, 2);
		} else {
			sub_view->obj_sub_h_unch = img_number_create(par);
			lv_obj_set_pos(sub_view->obj_sub_h_unch, sub_view->resource.grp_h_unch.x, sub_view->resource.grp_h_unch.y);
			lv_obj_set_size(sub_view->obj_sub_h_unch, sub_view->resource.grp_h_unch.width, sub_view->resource.grp_h_unch.height);
			img_number_set_src(sub_view->obj_sub_h_unch, sub_view->img_dsc_unch_num, 10);
			img_number_set_align(sub_view->obj_sub_h_unch, LV_ALIGN_LEFT_MID);
			img_number_set_value(sub_view->obj_sub_h_unch, min / 60, 2);
		}
	}
	/* display min*/
	if (m_is_ch) {
		if (sub_view->obj_sub_m_unch) {
			lv_obj_delete(sub_view->obj_sub_m_unch);
			sub_view->obj_sub_m_unch = NULL;
		}

		if (sub_view->obj_sub_m_ch) {
			img_number_set_value(sub_view->obj_sub_m_ch, min % 60, 2);
		} else {
			sub_view->obj_sub_m_ch = img_number_create(par);
			lv_obj_set_pos(sub_view->obj_sub_m_ch, sub_view->resource.grp_m_ch.x, sub_view->resource.grp_m_ch.y);
			lv_obj_set_size(sub_view->obj_sub_m_ch, sub_view->resource.grp_m_ch.width, sub_view->resource.grp_m_ch.height);
			img_number_set_src(sub_view->obj_sub_m_ch, sub_view->img_dsc_ch_num, 10);
			img_number_set_align(sub_view->obj_sub_m_ch, LV_ALIGN_LEFT_MID);
			img_number_set_value(sub_view->obj_sub_m_ch, min % 60, 2);
		}
	} else {
		if (sub_view->obj_sub_m_ch) {
			lv_obj_delete(sub_view->obj_sub_m_ch);
			sub_view->obj_sub_m_ch = NULL;
		}

		if (sub_view->obj_sub_m_unch) {
			img_number_set_value(sub_view->obj_sub_m_unch, min % 60, 2);
 		} else {
			sub_view->obj_sub_m_unch = img_number_create(par);
			lv_obj_set_pos(sub_view->obj_sub_m_unch, sub_view->resource.grp_m_unch.x, sub_view->resource.grp_m_unch.y);
			lv_obj_set_size(sub_view->obj_sub_m_unch, sub_view->resource.grp_m_unch.width, sub_view->resource.grp_m_unch.height);
			img_number_set_src(sub_view->obj_sub_m_unch, sub_view->img_dsc_unch_num, 10);
			img_number_set_align(sub_view->obj_sub_m_unch, LV_ALIGN_LEFT_MID);
			img_number_set_value(sub_view->obj_sub_m_unch, min % 60, 2);
 		}
	}
}

static void _display_sub_view_time(alarm_set_view_data_t *data, uint16_t min)
{
	if (_alarm_sub_view_load_group_from_scene(data, &data->sub_view->resource)) {
		SYS_LOG_ERR("load group fail\n");
		return;
	}
	_display_alarm_time(data, data->obj_alarm_bg, min, false, false);
}

static void _display_alarm_sub_view(alarm_set_view_data_t *data, uint8_t index)
{
	alarm_sub_view_data_t *sub_data = NULL;
	if (data->sub_view) {
		SYS_LOG_INF("%s %d \n",__FUNCTION__,__LINE__);
		return;
	}

	sub_data = app_mem_malloc(sizeof(*sub_data));
	if (!sub_data) {
		SYS_LOG_INF("%s %d \n",__FUNCTION__,__LINE__);
		return;
	}
	sub_data->al_index = index;
	data->sub_view = sub_data;

	_display_sub_view_btn(data);
	_display_sub_view_time(data, data->tm_min[index]);
	/* hidden left and right view to receive callback function when alarm sub view was draw */
	ui_gesture_lock_scroll();

}

static void _display_add_btn_view(alarm_set_view_data_t *data, lv_obj_t *par)
{
	int ret = 0;
	alarm_main_view_data_t *main_view = NULL;

	if (!data )
		return;

	main_view = data->main_view;

	if (!main_view)
		return;

	/* btn picture */
	ret = lvgl_res_load_pictures_from_scene(&data->res_scene, alarm_main_btn_def_ids, main_view->img_dsc_def_main_btn, main_view->pt_main_btn, NUM_ALARM_MAIN_BTNS);
	if (ret < 0) {
		return;
	}
	ret = lvgl_res_load_pictures_from_scene(&data->res_scene, alarm_main_btn_sel_ids, main_view->img_dsc_sel_main_btn, main_view->pt_main_btn, NUM_ALARM_MAIN_BTNS);
	if (ret < 0) {
		lvgl_res_unload_pictures(main_view->img_dsc_def_main_btn, NUM_ALARM_MAIN_BTNS);
		return;
	}
	_alarm_set_create_btn(par, &main_view->obj_main_btn[BTN_ADD], &main_view->pt_main_btn[BTN_ADD], &main_view->img_dsc_def_main_btn[BTN_ADD], &main_view->img_dsc_sel_main_btn[BTN_ADD]);
	lv_obj_add_flag(main_view->obj_main_btn[BTN_ADD], LV_OBJ_FLAG_CLICKABLE);
	SYS_LOG_INF("BTN_ADD %p\n", main_view->obj_main_btn[BTN_ADD]);

}
static void _display_alarm_list_one(alarm_set_view_data_t *data, lv_obj_t *par, uint8_t index)
{
	alarm_main_view_data_t *main_view = NULL;
	if (!data)
		return;

	main_view = data->main_view;
	if (!main_view || index >= ALARM_SET_CNT)
		return;

	if (data->tm_state[index] == ALARM_STATE_OFF) {
		/* display hour*/
		if (main_view->obj_tm_hour_on[index]) {
			lv_obj_delete(main_view->obj_tm_hour_on[index]);
			main_view->obj_tm_hour_on[index] = NULL;
		}
		if (main_view->obj_tm_hour_off[index])
			lv_obj_delete(main_view->obj_tm_hour_off[index]);
		main_view->obj_tm_hour_off[index] = img_number_create(par);
		lv_obj_set_pos(main_view->obj_tm_hour_off[index], main_view->resource.grp_hour_off[index].x, main_view->resource.grp_hour_off[index].y);
		lv_obj_set_size(main_view->obj_tm_hour_off[index], main_view->resource.grp_hour_off[index].width, main_view->resource.grp_hour_off[index].height);
		img_number_set_src(main_view->obj_tm_hour_off[index], main_view->img_dsc_off_num, 10);
		img_number_set_align(main_view->obj_tm_hour_off[index], LV_ALIGN_LEFT_MID);
		img_number_set_value(main_view->obj_tm_hour_off[index], data->tm_min[index] / 60, 2);

		lv_obj_add_event_cb(main_view->obj_tm_hour_off[index], _img_number_event_handler, LV_EVENT_ALL, NULL);
		/*set current background clickable to cut out click event send to front bg*/
		lv_obj_add_flag(main_view->obj_tm_hour_off[index], LV_OBJ_FLAG_CLICKABLE);
		lv_obj_set_ext_click_area(main_view->obj_tm_hour_off[index], lv_obj_get_style_width(main_view->obj_tm_hour_off[index], LV_PART_MAIN));

		/* display min*/
		if (main_view->obj_tm_min_on[index]) {
			lv_obj_delete(main_view->obj_tm_min_on[index]);
			main_view->obj_tm_min_on[index] = NULL;
		}
		if (main_view->obj_tm_min_off[index])
			lv_obj_delete(main_view->obj_tm_min_off[index]);

		main_view->obj_tm_min_off[index] = img_number_create(par);
		lv_obj_set_pos(main_view->obj_tm_min_off[index], main_view->resource.grp_min_off[index].x, main_view->resource.grp_min_off[index].y);
		lv_obj_set_size(main_view->obj_tm_min_off[index], main_view->resource.grp_min_off[index].width, main_view->resource.grp_min_off[index].height);
		img_number_set_src(main_view->obj_tm_min_off[index], main_view->img_dsc_off_num, 10);
		img_number_set_align(main_view->obj_tm_min_off[index], LV_ALIGN_LEFT_MID);
		img_number_set_value(main_view->obj_tm_min_off[index], data->tm_min[index] % 60, 2);
		lv_obj_add_event_cb(main_view->obj_tm_min_off[index], _img_number_event_handler, LV_EVENT_ALL, NULL);
		/*set current background clickable to cut out click event send to front bg*/
		lv_obj_add_flag(main_view->obj_tm_min_off[index], LV_OBJ_FLAG_CLICKABLE);
		lv_obj_set_ext_click_area(main_view->obj_tm_min_off[index], lv_obj_get_style_width(main_view->obj_tm_min_off[index], LV_PART_MAIN));

		/* display :*/
		if (main_view->obj_main_btn[2 * index])
			lv_obj_delete(main_view->obj_main_btn[2 * index]);

		_alarm_set_create_btn(par, &main_view->obj_main_btn[2 * index], &main_view->pt_main_btn[2 * index], &main_view->img_dsc_def_main_btn[2 * index], &main_view->img_dsc_sel_main_btn[2 * index]);
		lv_obj_add_flag(main_view->obj_main_btn[2 * index], LV_OBJ_FLAG_CHECKABLE);
	} else if (data->tm_state[index] > ALARM_STATE_OFF) {
		/* display hour*/
		if (main_view->obj_tm_hour_off[index]) {
			lv_obj_delete(main_view->obj_tm_hour_off[index]);
			main_view->obj_tm_hour_off[index] = NULL;
		}

		if (main_view->obj_tm_hour_on[index])
			lv_obj_delete(main_view->obj_tm_hour_on[index]);

		main_view->obj_tm_hour_on[index] = img_number_create(par);
		lv_obj_set_pos(main_view->obj_tm_hour_on[index], main_view->resource.grp_hour_on[index].x, main_view->resource.grp_hour_on[index].y);
		lv_obj_set_size(main_view->obj_tm_hour_on[index], main_view->resource.grp_hour_on[index].width, main_view->resource.grp_hour_on[index].height);
		img_number_set_src(main_view->obj_tm_hour_on[index], main_view->img_dsc_on_num, 10);
		img_number_set_align(main_view->obj_tm_hour_on[index], LV_ALIGN_LEFT_MID);
		img_number_set_value(main_view->obj_tm_hour_on[index], data->tm_min[index] / 60, 2);
		lv_obj_add_event_cb(main_view->obj_tm_hour_on[index], _img_number_event_handler, LV_EVENT_ALL, NULL);
		/*set current background clickable to cut out click event send to front bg*/
		lv_obj_add_flag(main_view->obj_tm_hour_on[index], LV_OBJ_FLAG_CLICKABLE);
		lv_obj_set_ext_click_area(main_view->obj_tm_hour_on[index], lv_obj_get_style_width(main_view->obj_tm_hour_on[index], LV_PART_MAIN));

		/* display min*/
		if (main_view->obj_tm_min_off[index]) {
			lv_obj_delete(main_view->obj_tm_min_off[index]);
			main_view->obj_tm_min_off[index] = NULL;
		}

		if (main_view->obj_tm_min_on[index])
			lv_obj_delete(main_view->obj_tm_min_on[index]);

		main_view->obj_tm_min_on[index] = img_number_create(par);
		lv_obj_set_pos(main_view->obj_tm_min_on[index], main_view->resource.grp_min_on[index].x, main_view->resource.grp_min_on[index].y);
		lv_obj_set_size(main_view->obj_tm_min_on[index], main_view->resource.grp_min_on[index].width, main_view->resource.grp_min_on[index].height);
		img_number_set_src(main_view->obj_tm_min_on[index], main_view->img_dsc_on_num, 10);
		img_number_set_align(main_view->obj_tm_min_on[index], LV_ALIGN_LEFT_MID);
		img_number_set_value(main_view->obj_tm_min_on[index], data->tm_min[index] % 60, 2);
		lv_obj_add_event_cb(main_view->obj_tm_min_on[index], _img_number_event_handler, LV_EVENT_ALL, NULL);
		/*set current background clickable to cut out click event send to front bg*/
		lv_obj_add_flag(main_view->obj_tm_min_on[index], LV_OBJ_FLAG_CLICKABLE);
		lv_obj_set_ext_click_area(main_view->obj_tm_min_on[index], lv_obj_get_style_width(main_view->obj_tm_min_on[index], LV_PART_MAIN));

		/* display :*/
		if (main_view->obj_main_btn[2 * index])
			lv_obj_delete(main_view->obj_main_btn[2 * index]);

		_alarm_set_create_btn(par, &main_view->obj_main_btn[2 * index], &main_view->pt_main_btn[2 * index], &main_view->img_dsc_def_main_btn[2 * index], &main_view->img_dsc_sel_main_btn[2 * index]);
		lv_obj_add_flag(main_view->obj_main_btn[2 * index], LV_OBJ_FLAG_CHECKABLE);
		/*default state is off,need to toggle*/
		alarm_set_btn_state_toggle(main_view->obj_main_btn[2 * index]);
		lv_obj_invalidate(main_view->obj_main_btn[2 * index]);
	}
}
static void _display_alarm_list(alarm_set_view_data_t *data, lv_obj_t *par)
{
	if (!data)
		return;
	
	alarm_main_view_data_t *main_view = data->main_view;
	if (!main_view)
		return;
	
	for (int i = 0; i < ALARM_SET_CNT; i++) {
		_display_alarm_list_one(data, par, i);
		if (data->tm_state[i] == ALARM_STATE_OFF) {
			/* display off btn*/
			_alarm_set_create_btn(par, &main_view->obj_main_btn[2 * i + 1], &main_view->pt_main_btn[2 * i + 1], &main_view->img_dsc_def_main_btn[2 * i + 1], &main_view->img_dsc_sel_main_btn[2 * i + 1]);
			lv_obj_add_flag(data->main_view->obj_main_btn[2 * i + 1], LV_OBJ_FLAG_CHECKABLE);

		} else if (data->tm_state[i] > ALARM_STATE_OFF) {
			/* display off btn*/
			_alarm_set_create_btn(par, &main_view->obj_main_btn[2 * i + 1], &main_view->pt_main_btn[2 * i + 1], &main_view->img_dsc_def_main_btn[2 * i + 1], &main_view->img_dsc_sel_main_btn[2 * i + 1]);
			lv_obj_add_flag(main_view->obj_main_btn[2 * i + 1], LV_OBJ_FLAG_CHECKABLE);
			/*default state is off,need to toggle*/
			alarm_set_btn_state_toggle(main_view->obj_main_btn[2 * i + 1]);
			lv_obj_invalidate(main_view->obj_main_btn[2 * i + 1]);
		}
	}
}
static void _display_exist_alarm_list(alarm_set_view_data_t *data, lv_obj_t *par)
{
	int alarm_cnt = 0;

	if (_alarm_list_load_group_from_scene(data, &data->main_view->resource)) {
		SYS_LOG_ERR("load group fail\n");
		return;
	}
#ifdef CONFIG_ALARM_MANAGER
	struct alarm_manager *alarm_list = NULL;

	system_registry_alarm_callback(alarm_event_cb);
	alarm_manager_init();
	alarm_list = alarm_manager_get_exist_alarm(&alarm_cnt);
	if (alarm_cnt > ALARM_SET_CNT)
		alarm_cnt = ALARM_SET_CNT;

	for (int i = 0; i < alarm_cnt; i++) {
		data->tm_min[i]  = alarm_list->alarm[i].alarm_time / 60;
		data->tm_state[i] = alarm_list->alarm[i].state;
	}
#endif
	if (alarm_cnt) {
		_display_alarm_list(data, par);
	} else {
		SYS_LOG_INF("alarm null\n");
	}
}
static void _display_alarm_main_view(alarm_set_view_data_t *data)
{
	alarm_main_view_data_t *main_data = NULL;

	main_data = app_mem_malloc(sizeof(*main_data));
	if (!main_data) {
		SYS_LOG_INF("%s %d \n",__FUNCTION__,__LINE__);
		return;
	}
	data->main_view = main_data;

	/* create add button */
	_display_add_btn_view(data, data->obj_alarm_bg);
	/* create exist alarm list */
	_display_exist_alarm_list(data, data->obj_alarm_bg);
}

static int _alarm_set_view_preload(view_data_t *view_data)
{
	return lvgl_res_preload_scene_compact(SCENE_ALARM_SET_VIEW, NULL, 0, lvgl_res_scene_preload_default_cb_for_view, (void *)ALARM_SET_VIEW,
			DEF_STY_FILE, DEF_RES_FILE, DEF_STR_FILE);
}

static int _alarm_set_view_layout(view_data_t *view_data)
{
	lv_obj_t *scr = lv_display_get_screen_active(view_data->display);
	alarm_set_view_data_t *data = NULL;

	SYS_LOG_INF("enter data size %d\n", sizeof(*data));

	data = app_mem_malloc(sizeof(*data));
	if (!data) {
		return -ENOMEM;
	}

	view_data->user_data = data;
	memset(data, 0, sizeof(*data));

	if (_alarm_set_load_resource(data)) {
		app_mem_free(data);
		view_data->user_data = NULL;
		return -ENOENT;
	}
	/* create bg image */
	data->obj_alarm_bg = lv_image_create(scr);
	lv_obj_set_pos(data->obj_alarm_bg, 0, 0);
	lv_obj_set_size(data->obj_alarm_bg, DEF_UI_WIDTH, DEF_UI_HEIGHT);

	lv_obj_set_style_bg_color(data->obj_alarm_bg, lv_color_make(0x3b, 0x3b, 0x3b), LV_PART_MAIN);
	lv_obj_set_style_bg_opa(data->obj_alarm_bg, LV_OPA_COVER, LV_PART_MAIN);

	/*set current background clickable to cut out click event send to front bg*/
	lv_obj_add_flag(data->obj_alarm_bg, LV_OBJ_FLAG_CLICKABLE);

	_alarm_set_create_img_array(data->obj_alarm_bg, data->obj_set_bg, data->pt_set_bg, data->img_dsc_set_bg, NUM_ALARM_SET_BG_IMGS);
	/*set current background clickable to cut out click event send to front bg*/
	lv_obj_add_flag(data->obj_alarm_bg, LV_OBJ_FLAG_CLICKABLE);
	lv_obj_remove_flag(data->obj_alarm_bg, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_GESTURE_BUBBLE);
	lv_obj_add_event_cb(data->obj_alarm_bg, _alarm_bg_event_handler, LV_EVENT_ALL, NULL);

	_display_alarm_main_view(data);

	p_alarm_set_data = data;

	return 0;
}

static int _alarm_set_view_paint(view_data_t *view_data)
{
	return 0;
}

static int _alarm_set_view_delete(view_data_t *view_data)
{
	alarm_set_view_data_t *data = view_data->user_data;

	if (data) {
		if (data->sub_view)
			_exit_alarm_sub_view(data);

		if (data->main_view)
			_exit_alarm_main_view(data);

		_alarm_set_delete_obj_array(data->obj_set_bg, NUM_ALARM_SET_BG_IMGS);
		if (data->obj_alarm_bg) {
			lv_obj_delete(data->obj_alarm_bg);
			data->obj_alarm_bg = NULL;
		}

		_alarm_set_unload_resource(data);
		app_mem_free(data);
		view_data->user_data = NULL;
		SYS_LOG_INF("ok\n");
	}
	else
	{
		lvgl_res_preload_cancel_scene(SCENE_ALARM_SET_VIEW);
	}
	p_alarm_set_data = NULL;
	lvgl_res_unload_scene_compact(SCENE_ALARM_SET_VIEW);
	lvgl_res_cache_clear(0);
#ifdef CONFIG_PROPERTY
	property_flush(NULL);
#endif
	return 0;
}

static int _alarm_set_view_handler(uint16_t view_id, view_data_t *view_data, uint8_t msg_id, void * msg_data)
{
	switch (msg_id) {
	case MSG_VIEW_PRELOAD:
		return _alarm_set_view_preload(view_data);
	case MSG_VIEW_LAYOUT:
		return _alarm_set_view_layout(view_data);
	case MSG_VIEW_DELETE:
		return _alarm_set_view_delete(view_data);
	case MSG_VIEW_PAINT:
		return _alarm_set_view_paint(view_data);
	default:
		return 0;
	}
	return 0;
}

VIEW_DEFINE2(alarm_set_view, _alarm_set_view_handler, NULL, \
		NULL, ALARM_SET_VIEW, NORMAL_ORDER, UI_VIEW_LVGL, DEF_UI_VIEW_WIDTH, DEF_UI_VIEW_HEIGHT);


