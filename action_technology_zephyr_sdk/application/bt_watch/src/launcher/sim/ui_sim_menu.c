#include "ui_sim.h"
#include <widgets/text_canvas.h>
#include <view_stack.h>

typedef struct sim_view_data {
	lv_font_t font;
    lv_obj_t *up_text[5];

    bool power_on_off;
} sim_view_data;

#if 1       //SIM_LOG_SET_VIEW
const char sim_log_set_text[][20] =
{
    "NONE",
    "ERROR",
    "WARNING",
    "INFO",
    "DEBUG",
};

static void _sim_log_set_up(sim_view_data *data)
{
    if(data)
    {
        alog_level_e log_level = sim_data.get_log_level();
        lv_coord_t text_num = sizeof(sim_log_set_text)/sizeof(sim_log_set_text[0]);
        for(uint32_t i = 0 ; i < text_num ; i++)
        {
            lv_obj_t *obj_icon = lv_obj_get_user_data(data->up_text[i]);
            if(log_level != i)
                lv_obj_set_style_bg_color(obj_icon,lv_color_hex(0x3B3B3B),LV_PART_MAIN);
            else
                lv_obj_set_style_bg_color(obj_icon,lv_color_hex(0x00ff00),LV_PART_MAIN);
        }
    }
}

static void _sim_log_set_btn_evt_handler(lv_event_t *e)
{
	int32_t btn_id = (int32_t)lv_obj_get_user_data(lv_event_get_current_target(e));
    sim_view_data *data = lv_event_get_user_data(e);
    sim_data.set_log_level(btn_id);
    _sim_log_set_up(data);
}

static int _ui_sim_log_set_layout(view_data_t *view_data)
{
	lv_obj_t *scr = lv_display_get_screen_active(view_data->display);
	sim_view_data *data = app_mem_malloc(sizeof(*data));
	if (!data) {
		return -ENOMEM;
	}
	memset(data, 0, sizeof(*data));

	view_data->user_data = data;

    LVGL_FONT_OPEN_DEFAULT(&data->font, 28);

    lv_coord_t layout_space = 110;
    lv_coord_t text_num = sizeof(sim_log_set_text)/sizeof(sim_log_set_text[0]);
    for(uint32_t i = 0 ; i < text_num ; i++)
    {
        lv_obj_t *obj_icon = lv_obj_create(scr);
		lv_obj_align(obj_icon,LV_ALIGN_TOP_MID,0,60 + layout_space * i);
		lv_obj_set_size(obj_icon,300,82);
		lv_obj_set_style_pad_all(obj_icon,0,LV_PART_MAIN);
		lv_obj_set_style_border_width(obj_icon,0,LV_PART_MAIN);
		lv_obj_set_style_radius(obj_icon,20,LV_PART_MAIN);
		lv_obj_set_style_bg_opa(obj_icon,LV_OPA_100,LV_PART_MAIN);
		lv_obj_add_event_cb(obj_icon, _sim_log_set_btn_evt_handler, LV_EVENT_SHORT_CLICKED, data);
		lv_obj_set_user_data(obj_icon,(void *)i);

        data->up_text[i] = text_canvas_create(obj_icon);
        lv_obj_align(data->up_text[i],LV_ALIGN_CENTER,0,0);
        lv_obj_set_width(data->up_text[i],300);
        lv_obj_set_style_text_align(data->up_text[i],LV_TEXT_ALIGN_CENTER,LV_PART_MAIN);
		lv_obj_set_style_text_color(data->up_text[i],lv_color_white(),LV_PART_MAIN);
		lv_obj_set_style_text_font(data->up_text[i],&data->font,LV_PART_MAIN);
        text_canvas_set_long_mode(data->up_text[i],TEXT_CANVAS_LONG_SCROLL_CIRCULAR);
        text_canvas_set_text(data->up_text[i],sim_log_set_text[i]);
        lv_obj_add_flag(data->up_text[i],LV_OBJ_FLAG_EVENT_BUBBLE);
        lv_obj_set_user_data(data->up_text[i],obj_icon);
    }
    _sim_log_set_up(data);

    lv_obj_t *obj_icon = lv_obj_create(scr);
	lv_obj_set_pos(obj_icon,0,60 + layout_space * text_num);
	lv_obj_set_size(obj_icon,4,4);
    return 0;
}

static int _ui_sim_log_set_delete(view_data_t *view_data)
{
    lv_obj_t *scr = lv_display_get_screen_active(view_data->display);
    lv_obj_clean(scr);
    sim_view_data *data = view_data->user_data;
	if (data) {
        LVGL_FONT_CLOSE(&data->font);
        app_mem_free(data);
    }
    view_data->user_data = NULL;
    return 0;
}

int _ui_sim_log_set_handler(uint16_t view_id, view_data_t *view_data, uint8_t msg_id, void * msg_data)
{
	view_data_t *view_data = view_get_data(view_id);
    int ret = 0;
	switch (msg_id) {
	case MSG_VIEW_PRELOAD:
        return ui_view_layout(SIM_LOG_SET_VIEW);
        break;
	case MSG_VIEW_LAYOUT:
        ret = _ui_sim_log_set_layout(view_data);
        break;
	case MSG_VIEW_DELETE:
        ret = _ui_sim_log_set_delete(view_data);
        break;
	default:
        break;
	}
	return ret;
}

VIEW_DEFINE2(ui_sim_log_set, _ui_sim_log_set_handler, NULL, \
		NULL, SIM_LOG_SET_VIEW, NORMAL_ORDER, UI_VIEW_LVGL, DEF_UI_VIEW_WIDTH, DEF_UI_VIEW_HEIGHT);
#endif

#if 1       //SIM_POWER_SET_VIEW
const char sim_power_set_text[][20] =
{
    "POWER ON",
    "POWER OFF",
    "重启",
    "Assert",
};

enum {
    SIM_POWER_ON,
    SIM_POWER_OFF,
    SIM_RESET,
    SIM_ASSERT,
};

static void _sim_power_set_btn_evt_handler(lv_event_t *e)
{
	int32_t btn_id = (int32_t)lv_obj_get_user_data(lv_event_get_current_target(e));
    switch (btn_id)
    {
    case SIM_POWER_ON:
        sim_data.ctrl_power_on(true);
        break;
    case SIM_POWER_OFF:
        sim_data.ctrl_power_on(false);
        break;
    case SIM_RESET:
        sim_data.ctrl_reset();
        break;
    case SIM_ASSERT:
        sim_data.ctrl_assert();
        break;
    default:
        break;
    }
}

static int _ui_sim_power_set_layout(view_data_t *view_data)
{
	lv_obj_t *scr = lv_display_get_screen_active(view_data->display);
	sim_view_data *data = app_mem_malloc(sizeof(*data));
	if (!data) {
		return -ENOMEM;
	}
	memset(data, 0, sizeof(*data));

	view_data->user_data = data;

    LVGL_FONT_OPEN_DEFAULT(&data->font, 28);

    lv_coord_t layout_space = 110;
    lv_coord_t text_num = sizeof(sim_power_set_text)/sizeof(sim_power_set_text[0]);
    for(uint32_t i = 0 ; i < text_num ; i++)
    {
        lv_obj_t *obj_icon = lv_obj_create(scr);
		lv_obj_set_pos(obj_icon,24,60 + layout_space * i);
		lv_obj_set_size(obj_icon,420,82);
		lv_obj_set_style_pad_all(obj_icon,0,LV_PART_MAIN);
		lv_obj_set_style_border_width(obj_icon,0,LV_PART_MAIN);
		lv_obj_set_style_radius(obj_icon,20,LV_PART_MAIN);
		lv_obj_set_style_bg_color(obj_icon,lv_color_hex(0x3B3B3B),LV_PART_MAIN);
		lv_obj_set_style_bg_opa(obj_icon,LV_OPA_100,LV_PART_MAIN);
		lv_obj_add_event_cb(obj_icon, _sim_power_set_btn_evt_handler, LV_EVENT_SHORT_CLICKED, data);
		lv_obj_set_user_data(obj_icon,(void *)i);

        data->up_text[i] = text_canvas_create(obj_icon);
        lv_obj_align(data->up_text[i],LV_ALIGN_CENTER,0,0);
        lv_obj_set_width(data->up_text[i],420);
        lv_obj_set_style_text_align(data->up_text[i],LV_TEXT_ALIGN_CENTER,LV_PART_MAIN);
		lv_obj_set_style_text_color(data->up_text[i],lv_color_white(),LV_PART_MAIN);
		lv_obj_set_style_text_font(data->up_text[i],&data->font,LV_PART_MAIN);
        text_canvas_set_long_mode(data->up_text[i],TEXT_CANVAS_LONG_SCROLL_CIRCULAR);
        text_canvas_set_text(data->up_text[i],sim_power_set_text[i]);
        lv_obj_add_flag(data->up_text[i],LV_OBJ_FLAG_EVENT_BUBBLE);
    }

    lv_obj_t *obj_icon = lv_obj_create(scr);
	lv_obj_set_pos(obj_icon,0,60 + layout_space * text_num);
	lv_obj_set_size(obj_icon,4,4);
    return 0;
}

static int _ui_sim_power_set_delete(view_data_t *view_data)
{
    lv_obj_t *scr = lv_display_get_screen_active(view_data->display);
    lv_obj_clean(scr);
    sim_view_data *data = view_data->user_data;
	if (data) {
        LVGL_FONT_CLOSE(&data->font);
        app_mem_free(data);
    }
    view_data->user_data = NULL;
    return 0;
}

int _ui_sim_power_set_handler(uint16_t view_id, view_data_t *view_data, uint8_t msg_id, void * msg_data)
{
    int ret = 0;
	switch (msg_id) {
	case MSG_VIEW_PRELOAD:
        return ui_view_layout(SIM_POWER_SET_VIEW);
        break;
	case MSG_VIEW_LAYOUT:
        ret = _ui_sim_power_set_layout(view_data);
        break;
	case MSG_VIEW_DELETE:
        ret = _ui_sim_power_set_delete(view_data);
        break;
	default:
        break;
	}
	return ret;
}

VIEW_DEFINE2(ui_sim_power_set, _ui_sim_power_set_handler, NULL, \
		NULL, SIM_POWER_SET_VIEW, NORMAL_ORDER, UI_VIEW_LVGL, DEF_UI_VIEW_WIDTH, DEF_UI_VIEW_HEIGHT);
#endif

#if 1       //SIM_DEVICE_INFO_VIEW
const char sim_device_info_text[][30] =
{
    "软件版本号:",
    "IMEI设备识别码:",
    "是否已校准:",
};

enum {
    SIM_SOFTWARE_VERSIONS,
    SIM_IMEI_CODE,
    SIM_ADJUSTING,
};
static void _ui_sim_device_info_composing(sim_view_data *data)
{
    lv_coord_t layout_space = 150;
    lv_coord_t offset_space = 0;
    lv_coord_t text_num = sizeof(sim_device_info_text)/sizeof(sim_device_info_text[0]);
    lv_point_t txt_size ={0};
    lv_text_get_size(&txt_size, "0", &data->font, 0, 0, 0, LV_TEXT_FLAG_EXPAND);
    lv_obj_t *obj_icon = NULL;

    for(uint32_t i = 0; i< text_num; i++) {
        obj_icon = lv_obj_get_parent(data->up_text[i]);
        lv_obj_set_pos(obj_icon, 24, 60 +layout_space * i +offset_space);
        lv_obj_update_layout(data->up_text[i]);
        lv_coord_t up_text_height = lv_obj_get_height(data->up_text[i]);
        if(up_text_height > txt_size.y) {
            lv_obj_set_height(obj_icon, 120 + up_text_height - txt_size.y);
            offset_space += up_text_height - txt_size.y;
        }
    }
    lv_obj_set_height(lv_obj_get_parent(obj_icon), 60 +layout_space *text_num +offset_space);
}
static void _sim_device_info_up(sim_view_data *data,uint8_t id)
{
    if(data)
    {
        char text[250] = {0};
        switch (id)
        {
        case SIM_SOFTWARE_VERSIONS:
            sim_data.get_version(text,sizeof(text));
            alog_info("[%s] version info: %s", __func__, text);
            break;
        case SIM_IMEI_CODE:
            sim_data.get_imei(0,text);
            break;
        case SIM_ADJUSTING:
            if(sim_data.radio_have_cali())
                strcpy(text,"是");
            else
                strcpy(text,"否");
            break;
        default:
            return;
        }
        text_canvas_set_text(data->up_text[id],text);
    }
}

static void _sim_device_info_btn_evt_handler(lv_event_t *e)
{
	int32_t btn_id = (int32_t)lv_obj_get_user_data(lv_event_get_current_target(e));
    sim_view_data *data = lv_event_get_user_data(e);
    _sim_device_info_up(data,btn_id);
}

static int _ui_sim_device_info_layout(view_data_t *view_data)
{
	lv_obj_t *scr = lv_display_get_screen_active(view_data->display);
	sim_view_data *data = app_mem_malloc(sizeof(*data));
	if (!data) {
		return -ENOMEM;
	}
	memset(data, 0, sizeof(*data));

	view_data->user_data = data;

    LVGL_FONT_OPEN_DEFAULT(&data->font, 28);

    lv_obj_t *host_icon = lv_obj_create(scr);
    lv_obj_set_pos(host_icon, 0, 0);
    lv_obj_set_size(host_icon, DEF_UI_WIDTH, DEF_UI_HEIGHT);
    lv_obj_set_style_pad_all(host_icon, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(host_icon, 0, LV_PART_MAIN);

    lv_coord_t text_num = sizeof(sim_device_info_text)/sizeof(sim_device_info_text[0]);
    for(uint32_t i = 0 ; i < text_num ; i++)
    {
        lv_obj_t *obj_icon = lv_obj_create(host_icon);
		lv_obj_set_width(obj_icon,420);
		lv_obj_set_style_pad_all(obj_icon,0,LV_PART_MAIN);
		lv_obj_set_style_border_width(obj_icon,0,LV_PART_MAIN);
		lv_obj_set_style_radius(obj_icon,20,LV_PART_MAIN);
		lv_obj_set_style_bg_color(obj_icon,lv_color_hex(0x3B3B3B),LV_PART_MAIN);
		lv_obj_set_style_bg_opa(obj_icon,LV_OPA_100,LV_PART_MAIN);
		lv_obj_add_event_cb(obj_icon, _sim_device_info_btn_evt_handler, LV_EVENT_SHORT_CLICKED, data);
		lv_obj_set_user_data(obj_icon,(void *)i);

        lv_obj_t *text_icon = text_canvas_create(obj_icon);
        lv_obj_align(text_icon,LV_ALIGN_TOP_MID,0,20);
        lv_obj_set_width(text_icon,420);
        lv_obj_set_style_text_align(text_icon,LV_TEXT_ALIGN_CENTER,LV_PART_MAIN);
		lv_obj_set_style_text_color(text_icon,lv_color_white(),LV_PART_MAIN);
		lv_obj_set_style_text_font(text_icon,&data->font,LV_PART_MAIN);
        text_canvas_set_long_mode(text_icon,TEXT_CANVAS_LONG_SCROLL_CIRCULAR);
        text_canvas_set_text(text_icon,sim_device_info_text[i]);
        lv_obj_add_flag(text_icon,LV_OBJ_FLAG_EVENT_BUBBLE);


        data->up_text[i] = text_canvas_create(obj_icon);
        lv_obj_align(data->up_text[i],LV_ALIGN_BOTTOM_MID,0,-20);
        lv_obj_set_width(data->up_text[i],420);
        lv_obj_set_style_text_align(data->up_text[i],LV_TEXT_ALIGN_CENTER,LV_PART_MAIN);
		lv_obj_set_style_text_color(data->up_text[i],lv_color_white(),LV_PART_MAIN);
		lv_obj_set_style_text_font(data->up_text[i],&data->font,LV_PART_MAIN);
        text_canvas_set_long_mode(data->up_text[i],TEXT_CANVAS_LONG_WRAP);
        lv_obj_add_flag(data->up_text[i],LV_OBJ_FLAG_EVENT_BUBBLE);
        _sim_device_info_up(data,i);
    }
    _ui_sim_device_info_composing(data);
    return 0;
}

static int _ui_sim_device_info_delete(view_data_t *view_data)
{
    lv_obj_t *scr = lv_display_get_screen_active(view_data->display);
    lv_obj_clean(scr);
    sim_view_data *data = view_data->user_data;
	if (data) {
        LVGL_FONT_CLOSE(&data->font);
        app_mem_free(data);
    }
    view_data->user_data = NULL;
    return 0;
}

int _ui_sim_device_info_handler(uint16_t view_id, view_data_t *view_data, uint8_t msg_id, void * msg_data)
{
	view_data_t *view_data = view_get_data(view_id);
    int ret = 0;
	switch (msg_id) {
	case MSG_VIEW_PRELOAD:
        return ui_view_layout(SIM_DEVICE_INFO_VIEW);
        break;
	case MSG_VIEW_LAYOUT:
        ret = _ui_sim_device_info_layout(view_data);
        break;
	case MSG_VIEW_DELETE:
        ret = _ui_sim_device_info_delete(view_data);
        break;
	default:
        break;
	}
	return ret;
}

VIEW_DEFINE2(ui_sim_device_info, _ui_sim_device_info_handler, NULL, \
		NULL, SIM_DEVICE_INFO_VIEW, NORMAL_ORDER, UI_VIEW_LVGL, DEF_UI_VIEW_WIDTH, DEF_UI_VIEW_HEIGHT);
#endif

#if 1       //SIM_MENU_VIEW
const char sim_menu_text[][20] =
{
    "设备信息",
    "Tele",
    "电源控制",
    "log设置",
};

enum {
    SIM_DEVICE_INFO,
    SIM_TELE,
    SIM_POWER_SET,
    SIM_LOG_SET,
};

static void _sim_menu_btn_evt_handler(lv_event_t *e)
{
	int32_t btn_id = (int32_t)lv_obj_get_user_data(lv_event_get_current_target(e));
    switch (btn_id)
    {
    case SIM_DEVICE_INFO:
        view_stack_push_view(SIM_DEVICE_INFO_VIEW, NULL);
        break;
    case SIM_TELE:
        view_stack_push_view(SIM_TELE_VIEW, NULL);
        break;
    case SIM_POWER_SET:
        view_stack_push_view(SIM_POWER_SET_VIEW, NULL);
        break;
    case SIM_LOG_SET:
        view_stack_push_view(SIM_LOG_SET_VIEW, NULL);
        break;
    default:
        break;
    }
}

static int _ui_sim_menu_layout(view_data_t *view_data)
{
	lv_obj_t *scr = lv_display_get_screen_active(view_data->display);
	sim_view_data *data = app_mem_malloc(sizeof(*data));
	if (!data) {
		return -ENOMEM;
	}
	memset(data, 0, sizeof(*data));

	view_data->user_data = data;

    LVGL_FONT_OPEN_DEFAULT(&data->font, 28);

    lv_coord_t layout_space = 110;
    lv_coord_t text_num = sizeof(sim_menu_text)/sizeof(sim_menu_text[0]);
    for(uint32_t i = 0 ; i < text_num ; i++)
    {
        lv_obj_t *obj_icon = lv_obj_create(scr);
		lv_obj_set_pos(obj_icon,24,60 + layout_space * i);
		lv_obj_set_size(obj_icon,420,82);
		lv_obj_set_style_pad_all(obj_icon,0,LV_PART_MAIN);
		lv_obj_set_style_border_width(obj_icon,0,LV_PART_MAIN);
		lv_obj_set_style_radius(obj_icon,20,LV_PART_MAIN);
		lv_obj_set_style_bg_color(obj_icon,lv_color_hex(0x3B3B3B),LV_PART_MAIN);
		lv_obj_set_style_bg_opa(obj_icon,LV_OPA_100,LV_PART_MAIN);
		lv_obj_add_event_cb(obj_icon, _sim_menu_btn_evt_handler, LV_EVENT_SHORT_CLICKED, data);
		lv_obj_set_user_data(obj_icon,(void *)i);

        lv_obj_t *text_icon = text_canvas_create(obj_icon);
        lv_obj_align(text_icon,LV_ALIGN_CENTER,0,0);
        lv_obj_set_width(text_icon,420);
        lv_obj_set_style_text_align(text_icon,LV_TEXT_ALIGN_CENTER,LV_PART_MAIN);
		lv_obj_set_style_text_color(text_icon,lv_color_white(),LV_PART_MAIN);
		lv_obj_set_style_text_font(text_icon,&data->font,LV_PART_MAIN);
        text_canvas_set_long_mode(text_icon,TEXT_CANVAS_LONG_SCROLL_CIRCULAR);
        text_canvas_set_text(text_icon,sim_menu_text[i]);
        lv_obj_add_flag(text_icon,LV_OBJ_FLAG_EVENT_BUBBLE);
    }

    lv_obj_t *obj_icon = lv_obj_create(scr);
	lv_obj_set_pos(obj_icon,0,60 + layout_space * text_num);
	lv_obj_set_size(obj_icon,4,4);
    return 0;
}

static int _ui_sim_menu_delete(view_data_t *view_data)
{
    lv_obj_t *scr = lv_display_get_screen_active(view_data->display);
    lv_obj_clean(scr);
    sim_view_data *data = view_data->user_data;
	if (data) {
        LVGL_FONT_CLOSE(&data->font);
        app_mem_free(data);
    }
    view_data->user_data = NULL;
    return 0;
}

int _ui_sim_menu_handler(uint16_t view_id, view_data_t *view_data, uint8_t msg_id, void * msg_data)
{
    int ret = 0;
	switch (msg_id) {
	case MSG_VIEW_PRELOAD:
        return ui_view_layout(SIM_MENU_VIEW);
        break;
	case MSG_VIEW_LAYOUT:
        ret = _ui_sim_menu_layout(view_data);
        break;
	case MSG_VIEW_DELETE:
        ret = _ui_sim_menu_delete(view_data);
        break;
	default:
        break;
	}
	return ret;
}

VIEW_DEFINE2(ui_sim_menu, _ui_sim_menu_handler, NULL, \
		NULL, SIM_MENU_VIEW, NORMAL_ORDER, UI_VIEW_LVGL, DEF_UI_VIEW_WIDTH, DEF_UI_VIEW_HEIGHT);
#endif
