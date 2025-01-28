#include "ui_sim.h"
#include <widgets/text_canvas.h>
#include <view_stack.h>
#include "aic_call_player.h"

typedef struct sim_tele_view_data {
	lv_font_t font;
    lv_font_t font_1;
    lv_obj_t *up_text[8];

    char network_dl_speed[50];
    char network_ul_speed[50];
    char sim_housing_info[200];
    char sim_neighbour_info[1800];
    char operator_info[400];
    char dial_number[18];
    char sms_smsc[22];
} sim_tele_view_data;

#if 1       //SIM_SMS_SET_VIEW
static sim_tele_view_data * sim_sms_data= NULL;
const char sim_sms_set_text[][30] =
{
    "收单条短信",
    "发单条短信",
    "加载短信列表",
    "短信中心号码",
};


enum {
    SIM_SMS_RECEIVE,
    SIM_SMS_SEND,
    SIM_SMS_LOAD_LIST,
    SIM_SMS_GET_SMSC,
};
#define SMS_TEXT_BASE_SIZE 60

static int _ui_sim_sms_load_list_layout(view_data_t *view_data)
{
    lv_obj_t *scr = lv_display_get_screen_active(view_data->display);
    sim_tele_view_data *data = app_mem_malloc(sizeof(*data));
    if (!data) {
        return -ENOMEM;
    }
    memset(data, 0, sizeof(*data));

    view_data->user_data = data;

    LVGL_FONT_OPEN_DEFAULT(&data->font, 28);

    lv_obj_t *obj_icon = lv_obj_create(scr);
    lv_obj_set_pos(obj_icon,24,20);
    lv_obj_set_size(obj_icon,420,60);
    lv_obj_set_style_pad_all(obj_icon,0,LV_PART_MAIN);
    lv_obj_set_style_border_width(obj_icon,0,LV_PART_MAIN);
    lv_obj_set_style_radius(obj_icon,20,LV_PART_MAIN);
    lv_obj_set_style_bg_color(obj_icon,lv_color_hex(0x3B3B3B),LV_PART_MAIN);
    lv_obj_set_style_bg_opa(obj_icon,LV_OPA_100,LV_PART_MAIN);

    lv_obj_t *title_icon = text_canvas_create(obj_icon);
    lv_obj_align(title_icon,LV_ALIGN_TOP_MID,0,20);
    lv_obj_set_width(title_icon,420);
    lv_obj_set_style_text_align(title_icon,LV_TEXT_ALIGN_CENTER,LV_PART_MAIN);
    lv_obj_set_style_text_color(title_icon,lv_color_white(),LV_PART_MAIN);
    lv_obj_set_style_text_font(title_icon,&data->font,LV_PART_MAIN);
    text_canvas_set_long_mode(title_icon,TEXT_CANVAS_LONG_SCROLL_CIRCULAR);
    text_canvas_set_text(title_icon,"短信列表");

    return 0;
}
static int _ui_sim_sms_load_list_delete(view_data_t *view_data)
{
    lv_obj_t *scr = lv_display_get_screen_active(view_data->display);
    lv_obj_clean(scr);
    sim_tele_view_data *data = view_data->user_data;
    if (data) {
        LVGL_FONT_CLOSE(&data->font);
        app_mem_free(data);
    }
    view_data->user_data = NULL;

    return 0;
}
static void _ui_sim_sms_load_list_composing(view_data_t *view_data)
{
    sim_tele_view_data *data = view_data->user_data;
    lv_obj_t *scr = lv_display_get_screen_active(view_data->display);
    lv_coord_t layout_space = 150;
    lv_coord_t offset_space = 0;
    uint32_t text_num = lv_obj_get_child_cnt(scr) -1;
    lv_point_t txt_size ={0};
    lv_text_get_size(&txt_size, "0", &data->font, 0, 0, 0, LV_TEXT_FLAG_EXPAND);
    lv_obj_t *obj_icon = NULL;

    for(uint32_t i = 0; i< text_num; i++) {
        obj_icon = lv_obj_get_parent(data->up_text[i]);
        lv_obj_set_pos(obj_icon, 24, 110 +layout_space * i +offset_space);
        lv_obj_update_layout(data->up_text[i]);
        lv_coord_t up_text_height = lv_obj_get_height(data->up_text[i]);
        if(up_text_height > txt_size.y) {
            lv_obj_set_height(obj_icon, 120 + up_text_height - txt_size.y);
            offset_space += up_text_height - txt_size.y;
        }
    }
}
static int _ui_sim_sms_load_list_paint(view_data_t *view_data, view_user_msg_data_t * msg_data)
{
    sim_tele_view_data *data = view_data->user_data;
    lv_obj_t *scr = lv_display_get_screen_active(view_data->display);
    char * sim_sms_text = (char *)(msg_data->data);
    SYS_LOG_INF("[%s] sms text: %s; the length of sms is %d", __func__, sim_sms_text, strlen(sim_sms_text));
    uint32_t icon_id = lv_obj_get_child_cnt(scr);
    lv_obj_t *obj_icon = lv_obj_create(scr);
    lv_obj_set_width(obj_icon,420);
    lv_obj_set_style_pad_all(obj_icon,0,LV_PART_MAIN);
    lv_obj_set_style_border_width(obj_icon,0,LV_PART_MAIN);
    lv_obj_set_style_radius(obj_icon,20,LV_PART_MAIN);
    lv_obj_set_style_bg_color(obj_icon,lv_color_hex(0x3B3B3B),LV_PART_MAIN);
    lv_obj_set_style_bg_opa(obj_icon,LV_OPA_100,LV_PART_MAIN);

    data->up_text[icon_id-1] = text_canvas_create(obj_icon);
    lv_obj_align(data->up_text[icon_id-1],LV_ALIGN_TOP_MID,0,20);
    lv_obj_set_width(data->up_text[icon_id-1],420);
    lv_obj_set_style_text_align(data->up_text[icon_id-1],LV_TEXT_ALIGN_CENTER,LV_PART_MAIN);
    lv_obj_set_style_text_color(data->up_text[icon_id-1],lv_color_white(),LV_PART_MAIN);
    lv_obj_set_style_text_font(data->up_text[icon_id-1],&data->font,LV_PART_MAIN);
    text_canvas_set_long_mode(data->up_text[icon_id-1],TEXT_CANVAS_LONG_WRAP);
    text_canvas_set_text_fmt(data->up_text[icon_id-1], "序号:[%d]\n%s", icon_id, sim_sms_text);

    _ui_sim_sms_load_list_composing(view_data);
    free(msg_data->data);
    return 0;
}
int _ui_sim_sms_load_list_handler(uint16_t view_id, view_data_t * view_data, uint8_t msg_id, void * msg_data)
{
    int ret = 0;
    switch (msg_id) {
    case MSG_VIEW_PRELOAD:
        ret = ui_view_layout(SIM_SMS_LOAD_LIST_VIEW);
        break;
    case MSG_VIEW_LAYOUT:
        ret = _ui_sim_sms_load_list_layout(view_data);
        break;
    case MSG_VIEW_DELETE:
        ret = _ui_sim_sms_load_list_delete(view_data);
        break;
    case MSG_VIEW_PAINT:
        ret = _ui_sim_sms_load_list_paint(view_data, msg_data);
        break;
    default:
        break;
    }
    return ret;
}
VIEW_DEFINE2(ui_sim_sms_load_list, _ui_sim_sms_load_list_handler, NULL, \
		NULL, SIM_SMS_LOAD_LIST_VIEW, NORMAL_ORDER, UI_VIEW_LVGL, DEF_UI_VIEW_WIDTH, DEF_UI_VIEW_HEIGHT);

static int32_t handleSmsCallback(void *p_param, uint32_t size)
{
    int32_t ret = -1;
    uint32_t event = TS_EVENT_SMS_MAX;

    if(NULL == p_param) {
        alog_error("[%s][Error]p_param is NULL.", __func__);
        return -1;
    }

    event = *(uint32_t *)p_param;
    alog_info("[%s][Info]event id is 0x%x.", __func__, event);
    switch(event) {
        case TS_EVENT_SMS_RECEIVE_NEW: /* new sms */
        {
            ts_sms_receive_new_t *p_ind = (ts_sms_receive_new_t *)p_param;

            alog_info("[%s][Info]TS_EVENT_SMS_RECEIVE_NEW, index:%d, total_count:%d, p_ind->is_full_in_sim = %d.\n", __func__, p_ind->index, p_ind->total_count, p_ind->is_full_in_sim);
            alog_info("[%s][Info]TS_EVENT_SMS_RECEIVE_NEW, sms_class(%d), ymd(%d-%d-%d), hms(%d:%d:%d), number(%s), content(%s).",
                      __func__,
                      p_ind->sms_class,
                      p_ind->time_stamp.year,
                      p_ind->time_stamp.month,
                      p_ind->time_stamp.day,
                      p_ind->time_stamp.hour,
                      p_ind->time_stamp.min,
                      p_ind->time_stamp.sec,
                      p_ind->number,
                      (char *)&p_ind->p_content);

            view_stack_push_view(SIM_SMS_LOAD_LIST_VIEW, NULL);
            int32_t text_length = SMS_TEXT_BASE_SIZE+strlen(p_ind->number) + strlen((char *)&p_ind->p_content);
            char * sim_sms_text = calloc(1, text_length);
            if(sim_sms_text != NULL) {
                snprintf(sim_sms_text, text_length-1, "发件人:%s\n时间:%d-%02d-%02d %02d:%02d:%02d\n短信内容:%s",
                p_ind->number,
                p_ind->time_stamp.year,
                p_ind->time_stamp.month,
                p_ind->time_stamp.day,
                p_ind->time_stamp.hour,
                p_ind->time_stamp.min,
                p_ind->time_stamp.sec,
                (char *)&p_ind->p_content);
                alog_debug("[%s] text_length = %d, the length of sim_sms_text = %d", __func__, text_length, strlen(sim_sms_text));
                int ret = ui_message_send_async(SIM_SMS_LOAD_LIST_VIEW, MSG_VIEW_PAINT, (uint32_t)sim_sms_text);
                if(ret != 0)
                    free(sim_sms_text);
            }
            break;
        }

        case TS_EVENT_SMS_READ_IN_SIM_RESP:
            {
                ts_sms_read_in_sim_resp_t *p_sim = (ts_sms_read_in_sim_resp_t *)p_param;

                alog_info("[%s][Info]TS_EVENT_SMS_READ_IN_SIM_RESP, index:%d, textstart:0x%x \n", __func__, p_sim->index, (uint32_t)p_sim->p_content);
                view_stack_push_view(SIM_SMS_LOAD_LIST_VIEW, NULL);
                if (p_sim->base_info.err_code == 0) {
                    alog_info("[%s][Info]index(%d), sms_class(%d), ymd(%d-%d-%d), hms(%d:%d:%d), number(%s), content(%s)",
                              __func__,
                              p_sim->index,
                              p_sim->sms_class,
                              p_sim->time_stamp.year,
                              p_sim->time_stamp.month,
                              p_sim->time_stamp.day,
                              p_sim->time_stamp.hour,
                              p_sim->time_stamp.min,
                              p_sim->time_stamp.sec,
                              p_sim->number,
                              (char *)&p_sim->p_content);
                    int32_t text_length = SMS_TEXT_BASE_SIZE+strlen(p_sim->number) + strlen((char *)&p_sim->p_content);
                    char * sim_sms_text = calloc(1, text_length);
                    if(sim_sms_text != NULL) {
                        snprintf(sim_sms_text, text_length-1, "发件人:%s\n时间:%d-%02d-%02d %02d:%02d:%02d\n短信内容:%s",
                        p_sim->number,
                        p_sim->time_stamp.year,
                        p_sim->time_stamp.month,
                        p_sim->time_stamp.day,
                        p_sim->time_stamp.hour,
                        p_sim->time_stamp.min,
                        p_sim->time_stamp.sec,
                        (char *)&p_sim->p_content);
                        alog_debug("[%s] text_length = %d, the length of sim_sms_text = %d", __func__, text_length, strlen(sim_sms_text));
                        int ret = ui_message_send_async(SIM_SMS_LOAD_LIST_VIEW, MSG_VIEW_PAINT, (uint32_t)sim_sms_text);
                        if(ret != 0)
                            free(sim_sms_text);
                    }

                }else {
                    char * sim_sms_text = calloc(1, SMS_TEXT_BASE_SIZE);
                    if(sim_sms_text != NULL) {
                        strcpy(sim_sms_text, "读取短信失败");
                        int ret = ui_message_send_async(SIM_SMS_LOAD_LIST_VIEW, MSG_VIEW_PAINT, (uint32_t)sim_sms_text);
                        if(ret != 0)
                            free(sim_sms_text);
                    }
                }
            }
            break;

        case TS_EVENT_SMS_GET_SMSC_RESP:
            {
                ts_sms_get_smsc_resp_t* p_sms_smsc = (ts_sms_get_smsc_resp_t*)(p_param);

                alog_info("[%s][Info]TS_EVENT_SMS_GET_SMSC_RESP, p_sms_smsc->smsc is %s. \n", __func__, p_sms_smsc->smsc);
                if(sim_sms_data) {
                    strcpy(sim_sms_data->sms_smsc,p_sms_smsc->smsc);
                    ui_message_send_async(SIM_SMS_SET_VIEW, MSG_VIEW_PAINT, 0);
                }
            }
            break;

        case TS_EVENT_SMS_SEND_RESP:
            {
                ts_sms_send_resp_t* p_sms_send = (ts_sms_send_resp_t*)(p_param);

                alog_info("[%s][Info]TS_EVENT_SMS_SEND_RESP, p_sms_send->base_info.err_code = %d.", __func__, p_sms_send->base_info.err_code);
                if (p_sms_send->base_info.err_code == 0){/* sms send successfully */
                    alog_info("[%s][Info]TS_EVENT_SMS_SEND_RESP, p_sms_send->index = %d.", __func__, p_sms_send->index);
                }
            }
            break;

        case TS_EVENT_SMS_DEL_IN_SIM_RESP:
            {
                ts_sms_del_in_sim_resp_t *p_sms_del = (ts_sms_del_in_sim_resp_t *)p_param;

                alog_info("[%s][Info]TS_EVENT_SMS_DEL_IN_SIM_RESP, p_sms_del->base_info.err_code = %d.", __func__, p_sms_del->base_info.err_code);
            }
            break;

        case TS_EVENT_SMS_LOAD_IN_SIM_RESP: {
            ts_sms_load_in_sim_resp_t *p_load_sms = (ts_sms_load_in_sim_resp_t *)p_param;
            uint16_t index = 0;

            alog_info("[%s]TS_EVENT_SMS_LOAD_IN_SIM_RESP, card_id = %d, err_code = %d, is_full_in_sim = %d, total_count = %d, count_in_sim = %d, have_reported_count = %d, cur_reported_count = %d, index_start = %d.",
                      __func__,
                      p_load_sms->base_info.card_id,
                      p_load_sms->base_info.err_code,
                      p_load_sms->is_full_in_sim,
                      p_load_sms->total_count,
                      p_load_sms->count_in_sim,
                      p_load_sms->have_reported_count,
                      p_load_sms->cur_reported_count,
                      p_load_sms->index_start);
            if (0 == p_load_sms->base_info.err_code) {
                /* load sim sms success. */
                if (true == p_load_sms->is_full_in_sim) {
                    /* If the sms in sim is full, user could do something here.(such as pop up some UI tips or delete some messages.) */
                }
                /* when user receive this response event, call this api to get sms content */
                for (index = p_load_sms->index_start; index <= p_load_sms->total_count; index++) {
                    ret = aic_srv_tele_sms_read_in_sim(p_load_sms->base_info.card_id, index, handleSmsCallback);
                    alog_info("[%s]aic_srv_tele_sms_read_in_sim[index = %d] return %d.", __func__, index, ret);
                }
            } else {
                /* load sim sms failed, and user could pop up some UI tips. */
            }
            break;
        }

        default:
            break;
    }

    return 0;
}

static void _sim_sms_set_btn_evt_handler(lv_event_t *e)
{
    int32_t btn_id = (int32_t)lv_obj_get_user_data(lv_event_get_current_target(e));
    //sim_tele_view_data *data = lv_event_get_user_data(e);
    switch (btn_id)
    {
    case SIM_SMS_RECEIVE:
        {
            aic_srv_bus_info_t bus_info = {0};
            bus_info.p_client_name = "sms_app";
            aic_srv_tele_sms_register(&bus_info, handleSmsCallback);
            break;
        }
    case SIM_SMS_SEND:
        sim_ui_data.dial_num_type = CALL_MESSAGES_TYPE;
        view_stack_push_view(SIM_CALL_DIAL_VIEW, NULL);
        break;
    case SIM_SMS_LOAD_LIST:
        sim_data.tele_sms_load_list(0, handleSmsCallback);
        break;
    default:
        break;
    }
}

static int _ui_sim_sms_set_layout(view_data_t *view_data)
{
	lv_obj_t *scr = lv_display_get_screen_active(view_data->display);
	sim_tele_view_data *data = app_mem_malloc(sizeof(*data));
	if (!data) {
		return -ENOMEM;
	}
	memset(data, 0, sizeof(*data));

    sim_sms_data = data;
	view_data->user_data = data;

    LVGL_FONT_OPEN_DEFAULT(&data->font, 28);

    lv_coord_t layout_space = 150;
    lv_coord_t text_num = sizeof(sim_sms_set_text)/sizeof(sim_sms_set_text[0]);
    for(uint32_t i = 0 ; i < text_num ; i++)
    {
        lv_obj_t *obj_icon = lv_obj_create(scr);
		lv_obj_set_pos(obj_icon,24,60 + layout_space * i);
		lv_obj_set_size(obj_icon,420,120);
		lv_obj_set_style_pad_all(obj_icon,0,LV_PART_MAIN);
		lv_obj_set_style_border_width(obj_icon,0,LV_PART_MAIN);
		lv_obj_set_style_radius(obj_icon,20,LV_PART_MAIN);
		lv_obj_set_style_bg_color(obj_icon,lv_color_hex(0x3B3B3B),LV_PART_MAIN);
		lv_obj_set_style_bg_opa(obj_icon,LV_OPA_100,LV_PART_MAIN);
		lv_obj_add_event_cb(obj_icon, _sim_sms_set_btn_evt_handler, LV_EVENT_SHORT_CLICKED, data);
		lv_obj_set_user_data(obj_icon,(void *)i);

        lv_obj_t *text_icon = text_canvas_create(obj_icon);
        lv_obj_align(text_icon,LV_ALIGN_TOP_MID,0,20);
        lv_obj_set_width(text_icon,420);
        lv_obj_set_style_text_align(text_icon,LV_TEXT_ALIGN_CENTER,LV_PART_MAIN);
		lv_obj_set_style_text_color(text_icon,lv_color_white(),LV_PART_MAIN);
		lv_obj_set_style_text_font(text_icon,&data->font,LV_PART_MAIN);
        text_canvas_set_long_mode(text_icon,TEXT_CANVAS_LONG_SCROLL_CIRCULAR);
        text_canvas_set_text(text_icon,sim_sms_set_text[i]);
        lv_obj_add_flag(text_icon,LV_OBJ_FLAG_EVENT_BUBBLE);

        if(i == SIM_SMS_GET_SMSC)
        {
            data->up_text[i] = text_canvas_create(obj_icon);
            lv_obj_align(data->up_text[i],LV_ALIGN_BOTTOM_MID,0,-20);
            lv_obj_set_width(data->up_text[i],420);
            lv_obj_set_style_text_align(data->up_text[i],LV_TEXT_ALIGN_CENTER,LV_PART_MAIN);
            lv_obj_set_style_text_color(data->up_text[i],lv_color_white(),LV_PART_MAIN);
            lv_obj_set_style_text_font(data->up_text[i],&data->font,LV_PART_MAIN);
            lv_obj_add_flag(data->up_text[i],LV_OBJ_FLAG_EVENT_BUBBLE);
            sim_data.tele_sms_get_smsc(0,handleSmsCallback);
        }
        else
            lv_obj_align(text_icon,LV_ALIGN_CENTER,0,0);
    }

    lv_obj_t *obj_icon = lv_obj_create(scr);
	lv_obj_set_pos(obj_icon,0,60 + layout_space * text_num);
	lv_obj_set_size(obj_icon,4,4);
    return 0;
}

static int _ui_sim_sms_set_delete(view_data_t *view_data)
{
    lv_obj_t *scr = lv_display_get_screen_active(view_data->display);
    lv_obj_clean(scr);
    sim_tele_view_data *data = view_data->user_data;
	if (data) {
        LVGL_FONT_CLOSE(&data->font);
        app_mem_free(data);
    }
    sim_sms_data = NULL;
    view_data->user_data = NULL;
    return 0;
}

static int _ui_sim_sms_set_paint(view_data_t *view_data)
{
    text_canvas_set_text(sim_sms_data->up_text[SIM_SMS_GET_SMSC],sim_sms_data->sms_smsc);
    return 0;
}

int _ui_sim_sms_set_handler(uint16_t view_id, view_data_t *view_data, uint8_t msg_id, void * msg_data)
{
    int ret = 0;
	switch (msg_id) {
	case MSG_VIEW_PRELOAD:
        ret = ui_view_layout(SIM_SMS_SET_VIEW);
        break;
	case MSG_VIEW_LAYOUT:
        ret = _ui_sim_sms_set_layout(view_data);
        break;
	case MSG_VIEW_DELETE:
        ret = _ui_sim_sms_set_delete(view_data);
        break;
    case MSG_VIEW_PAINT:
        ret = _ui_sim_sms_set_paint(view_data);
	default:
        break;
	}
	return ret;
}

VIEW_DEFINE2(ui_sim_sms_set, _ui_sim_sms_set_handler, NULL, \
		NULL, SIM_SMS_SET_VIEW, NORMAL_ORDER, UI_VIEW_LVGL, DEF_UI_VIEW_WIDTH, DEF_UI_VIEW_HEIGHT);
#endif

#if 1       //SIM_CALL_DIAL_VIEW
enum {
    CALL_DIAL_NUM_1,
    CALL_DIAL_NUM_2,
    CALL_DIAL_NUM_3,
    CALL_DIAL_NUM_4,
    CALL_DIAL_NUM_5,
    CALL_DIAL_NUM_6,
    CALL_DIAL_NUM_7,
    CALL_DIAL_NUM_8,
    CALL_DIAL_NUM_9,
    CALL_DIAL_NUM_0,
    CALL_DIAL_NUM_J,    //+
    CALL_DIAL_NUM_N,    //#
    CALL_DIAL_NUM_CALL, //拨号
    CALL_DIAL_NUM_X,    //*
    CALL_DIAL_NUM_DEL,  //删除
};

const static char call_text[][8] = {
    "1","2","3","4","5","6","7","8","9","0","+","#","拨号","*","删除"
};

const static char messages_text[] = {"发送"};

int32_t call_dial_cb(void *p_param, uint32_t size)
{
    return 0;
}

static void _sim_call_dial_btn_evt_handler(lv_event_t *e)
{
	int32_t btn_id = (int32_t)lv_obj_get_user_data(lv_event_get_current_target(e));
    sim_tele_view_data *data = lv_event_get_user_data(e);
    if(btn_id < CALL_DIAL_NUM_CALL || btn_id == CALL_DIAL_NUM_X)
    {
        if(strlen(data->dial_number) < sizeof(data->dial_number) - 1)
        {
            strcat(data->dial_number,call_text[btn_id]);
            text_canvas_set_text(data->up_text[0],data->dial_number);
        }
    }
    else if(btn_id == CALL_DIAL_NUM_DEL)
    {
        uint32_t num = strlen(data->dial_number);
        if(num)
            data->dial_number[num - 1] = 0;
        text_canvas_set_text(data->up_text[0],data->dial_number);
    }
    else
    {
        if(sim_ui_data.dial_num_type == CALL_MESSAGES_TYPE)
        {
            const static char text[20] = "hf";
            const ts_sms_send_info_t p_sms_info = {
                .p_smsc = NULL,
                .p_number = data->dial_number,
                .p_text = text,
                .is_write_to_sim = false,
            };
            sim_data.tele_sms_send(0,&p_sms_info,handleSmsCallback);
        }
        else
        {
            ts_call_dial_info_t p_dial_info = {0};
            lv_memcpy(p_dial_info.phone_number,data->dial_number,sizeof(data->dial_number));
            sim_data.tele_call_dail(&p_dial_info,call_dial_cb);
        }
    }
}

static void call_dial_btn_create(lv_obj_t *obj ,sim_tele_view_data *data, lv_coord_t id
    ,lv_coord_t x ,lv_coord_t y,lv_coord_t w,lv_coord_t h,lv_font_t *font)
{
    lv_obj_t *obj_icon = lv_obj_create(obj);
    lv_obj_set_pos(obj_icon,x,y);
    lv_obj_set_size(obj_icon,w,h);
    lv_obj_set_style_pad_all(obj_icon,0,LV_PART_MAIN);
    lv_obj_set_style_border_width(obj_icon,0,LV_PART_MAIN);
    lv_obj_add_event_cb(obj_icon, _sim_call_dial_btn_evt_handler, LV_EVENT_SHORT_CLICKED, data);
    lv_obj_set_user_data(obj_icon,(void *)id);
    lv_obj_t *text_icon = text_canvas_create(obj_icon);
    lv_obj_align(text_icon,LV_ALIGN_CENTER,0,0);
    lv_obj_set_width(text_icon,w);
    lv_obj_set_style_text_align(text_icon,LV_TEXT_ALIGN_CENTER,LV_PART_MAIN);
    if(id == CALL_DIAL_NUM_CALL)
        lv_obj_set_style_text_color(text_icon,lv_color_hex(0x00ff00),LV_PART_MAIN);
    else if(id == CALL_DIAL_NUM_DEL)
        lv_obj_set_style_text_color(text_icon,lv_color_hex(0xff0000),LV_PART_MAIN);
    else
        lv_obj_set_style_text_color(text_icon,lv_color_white(),LV_PART_MAIN);
    lv_obj_set_style_text_font(text_icon,font,LV_PART_MAIN);
    text_canvas_set_long_mode(text_icon,TEXT_CANVAS_LONG_SCROLL_CIRCULAR);
    if(id == CALL_DIAL_NUM_CALL && sim_ui_data.dial_num_type == CALL_MESSAGES_TYPE)
        text_canvas_set_text(text_icon,messages_text);
    else
        text_canvas_set_text(text_icon,call_text[id]);
    lv_obj_add_flag(text_icon,LV_OBJ_FLAG_EVENT_BUBBLE);
}

static int _ui_sim_call_dial_layout(view_data_t *view_data)
{
	lv_obj_t *scr = lv_display_get_screen_active(view_data->display);
	sim_tele_view_data *data = app_mem_malloc(sizeof(*data));
	if (!data) {
		return -ENOMEM;
	}
	memset(data, 0, sizeof(*data));

	view_data->user_data = data;

    LVGL_FONT_OPEN_DEFAULT(&data->font, 46);
    LVGL_FONT_OPEN_DEFAULT(&data->font_1, 32);

    lv_coord_t st_x = 30,st_y = 150;
    for(uint32_t i = 0 ; i < CALL_DIAL_NUM_CALL ; i++)
    {
        call_dial_btn_create(scr,data,i,st_x,st_y,100,70,&data->font);
        st_x += 100;
        if(st_x >= 400)
        {
            st_x = 30;
            st_y += 70;
        }
    }

    call_dial_btn_create(scr,data,CALL_DIAL_NUM_CALL,30,st_y,136,50,&data->font_1);
    call_dial_btn_create(scr,data,CALL_DIAL_NUM_X,166,st_y,100,50,&data->font);
    call_dial_btn_create(scr,data,CALL_DIAL_NUM_DEL,266,st_y,136,50,&data->font_1);

    data->up_text[0] = text_canvas_create(scr);
    lv_obj_align(data->up_text[0],LV_ALIGN_TOP_MID,0,60);
    lv_obj_set_width(data->up_text[0],420);
    lv_obj_set_style_text_align(data->up_text[0],LV_TEXT_ALIGN_CENTER,LV_PART_MAIN);
    lv_obj_set_style_text_color(data->up_text[0],lv_color_white(),LV_PART_MAIN);
    lv_obj_set_style_text_font(data->up_text[0],&data->font_1,LV_PART_MAIN);
    text_canvas_set_long_mode(data->up_text[0],TEXT_CANVAS_LONG_SCROLL_CIRCULAR);
    return 0;
}

static int _ui_sim_call_dial_delete(view_data_t *view_data)
{
    lv_obj_t *scr = lv_display_get_screen_active(view_data->display);
    lv_obj_clean(scr);
    sim_tele_view_data *data = view_data->user_data;
	if (data) {
        LVGL_FONT_CLOSE(&data->font);
        LVGL_FONT_CLOSE(&data->font_1);
        app_mem_free(data);
    }
    view_data->user_data = NULL;
    return 0;
}

int _ui_sim_call_dial_handler(uint16_t view_id, view_data_t *view_data, uint8_t msg_id, void * msg_data)
{
    int ret = 0;
    SYS_LOG_ERR("_ui_sim_call_dial_handler %d",msg_id);
	switch (msg_id) {
	case MSG_VIEW_PRELOAD:
        ret = ui_view_layout(SIM_CALL_DIAL_VIEW);
        break;
	case MSG_VIEW_LAYOUT:
        ret = _ui_sim_call_dial_layout(view_data);
        break;
	case MSG_VIEW_DELETE:
        ret = _ui_sim_call_dial_delete(view_data);
        break;
	default:
        break;
	}
	return ret;
}

VIEW_DEFINE2(ui_sim_call_dial, _ui_sim_call_dial_handler, NULL, \
		NULL, SIM_CALL_DIAL_VIEW, NORMAL_ORDER, UI_VIEW_LVGL, DEF_UI_VIEW_WIDTH, DEF_UI_VIEW_HEIGHT);
#endif

#if 1       //SIM_CALL_SET_VIEW
const char sim_call_set_text[][30] =
{
    "自动接听:",
    "电话语音LOOPBACK:",
    "VoLTE开关:",
    "VoLTE状态:",
    "拨打电话:",
};

enum {
    SIM_AUTO_ANSWER,
    SIM_LOOPBACK_ON_OFF,
    SIM_VOLTE_SWITCH,
    SIM_CALL_VOLTE,
    SIM_DIAL,
};

static void _sim_call_set_up(sim_tele_view_data *data,uint8_t id)
{
    if(data)
    {
        char text[100] = {0};
        switch (id)
        {
        case SIM_AUTO_ANSWER:
            if(sim_data.get_auto_answer())
                strcpy(text,"开");
            else
                strcpy(text,"关");
            break;
        case SIM_LOOPBACK_ON_OFF:
            if(sim_data.get_voice_enable_loopback())
                strcpy(text,"开");
            else
                strcpy(text,"关");
            break;
        case SIM_VOLTE_SWITCH:
            {
                bool is_enable = false;
                is_enable =sim_data.get_enable_volte(0);
                if(true == is_enable)
                    strcpy(text,"开");
                else
                    strcpy(text,"关");
            }
            break;
        case SIM_CALL_VOLTE:
            if(sim_data.call_have_volte_reg(0))
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

static void _sim_call_set_btn_evt_handler(lv_event_t *e)
{
	int32_t btn_id = (int32_t)lv_obj_get_user_data(lv_event_get_current_target(e));
    sim_tele_view_data *data = lv_event_get_user_data(e);
    switch (btn_id)
    {
    case SIM_AUTO_ANSWER:
        if(sim_data.get_auto_answer())
            sim_data.set_auto_answer(false);
        else
            sim_data.set_auto_answer(true);
        break;
    case SIM_LOOPBACK_ON_OFF:
        if(sim_data.get_voice_enable_loopback())
            sim_data.set_voice_enable_loopback(false);
        else
            sim_data.set_voice_enable_loopback(true);
        break;
    case SIM_VOLTE_SWITCH:
        {
            bool is_enable = false;
            is_enable = sim_data.get_enable_volte(0);
            if(is_enable)
                sim_data.set_enable_volte(0, false, NULL);
            else
                sim_data.set_enable_volte(0, true, NULL);
        }
        return;
    case SIM_DIAL:
        sim_ui_data.dial_num_type = CALL_DIALING_TYPE;
        view_stack_push_view(SIM_CALL_DIAL_VIEW, NULL);
        return;
    default:
        return;
    }
    _sim_call_set_up(data,btn_id);
}

static int _ui_sim_call_set_layout(view_data_t *view_data)
{
	lv_obj_t *scr = lv_display_get_screen_active(view_data->display);
	sim_tele_view_data *data = app_mem_malloc(sizeof(*data));
	if (!data) {
		return -ENOMEM;
	}
	memset(data, 0, sizeof(*data));

	view_data->user_data = data;

    LVGL_FONT_OPEN_DEFAULT(&data->font, 28);

    lv_coord_t layout_space = 150;
    lv_coord_t text_num = sizeof(sim_call_set_text)/sizeof(sim_call_set_text[0]);
    for(uint32_t i = 0 ; i < text_num ; i++)
    {
        lv_obj_t *obj_icon = lv_obj_create(scr);
		lv_obj_set_pos(obj_icon,24,60 + layout_space * i);
		lv_obj_set_size(obj_icon,420,120);
		lv_obj_set_style_pad_all(obj_icon,0,LV_PART_MAIN);
		lv_obj_set_style_border_width(obj_icon,0,LV_PART_MAIN);
		lv_obj_set_style_radius(obj_icon,20,LV_PART_MAIN);
		lv_obj_set_style_bg_color(obj_icon,lv_color_hex(0x3B3B3B),LV_PART_MAIN);
		lv_obj_set_style_bg_opa(obj_icon,LV_OPA_100,LV_PART_MAIN);
		lv_obj_add_event_cb(obj_icon, _sim_call_set_btn_evt_handler, LV_EVENT_SHORT_CLICKED, data);
		lv_obj_set_user_data(obj_icon,(void *)i);

        lv_obj_t *text_icon = text_canvas_create(obj_icon);
        lv_obj_align(text_icon,LV_ALIGN_TOP_MID,0,20);
        lv_obj_set_width(text_icon,420);
        lv_obj_set_style_text_align(text_icon,LV_TEXT_ALIGN_CENTER,LV_PART_MAIN);
		lv_obj_set_style_text_color(text_icon,lv_color_white(),LV_PART_MAIN);
		lv_obj_set_style_text_font(text_icon,&data->font,LV_PART_MAIN);
        text_canvas_set_long_mode(text_icon,TEXT_CANVAS_LONG_SCROLL_CIRCULAR);
        text_canvas_set_text(text_icon,sim_call_set_text[i]);
        lv_obj_add_flag(text_icon,LV_OBJ_FLAG_EVENT_BUBBLE);

        if(i != SIM_DIAL)
        {
            data->up_text[i] = text_canvas_create(obj_icon);
            lv_obj_align(data->up_text[i],LV_ALIGN_BOTTOM_MID,0,-20);
            lv_obj_set_width(data->up_text[i],420);
            lv_obj_set_style_text_align(data->up_text[i],LV_TEXT_ALIGN_CENTER,LV_PART_MAIN);
            lv_obj_set_style_text_color(data->up_text[i],lv_color_white(),LV_PART_MAIN);
            lv_obj_set_style_text_font(data->up_text[i],&data->font,LV_PART_MAIN);
            text_canvas_set_long_mode(data->up_text[i],TEXT_CANVAS_LONG_SCROLL_CIRCULAR);
            lv_obj_add_flag(data->up_text[i],LV_OBJ_FLAG_EVENT_BUBBLE);
            _sim_call_set_up(data,i);
        }
        else
            lv_obj_align(text_icon,LV_ALIGN_CENTER,0,0);
    }

    lv_obj_t *obj_icon = lv_obj_create(scr);
	lv_obj_set_pos(obj_icon,0,60 + layout_space * text_num);
	lv_obj_set_size(obj_icon,4,4);
    return 0;
}

static int _ui_sim_call_set_delete(view_data_t *view_data)
{
    lv_obj_t *scr = lv_display_get_screen_active(view_data->display);
    lv_obj_clean(scr);
    sim_tele_view_data *data = view_data->user_data;
	if (data) {
        LVGL_FONT_CLOSE(&data->font);
        app_mem_free(data);
    }
    view_data->user_data = NULL;
    return 0;
}

static int _ui_sim_call_set_paint(view_data_t * view_data)
{
    sim_tele_view_data *data = view_data->user_data;
    _sim_call_set_up(data, SIM_VOLTE_SWITCH);
    _sim_call_set_up(data, SIM_CALL_VOLTE);
    return 0;
}

int _ui_sim_call_set_handler(uint16_t view_id, view_data_t *view_data,  uint8_t msg_id, void * msg_data)
{
    int ret = 0;
	switch (msg_id) {
	case MSG_VIEW_PRELOAD:
        ret = ui_view_layout(SIM_CALL_SET_VIEW);
        break;
	case MSG_VIEW_LAYOUT:
        ret = _ui_sim_call_set_layout(view_data);
        break;
	case MSG_VIEW_DELETE:
        ret = _ui_sim_call_set_delete(view_data);
        break;
    case MSG_VIEW_PAINT:
        ret = _ui_sim_call_set_paint(view_data);
	default:
        break;
	}
	return ret;
}

VIEW_DEFINE2(ui_sim_call_set, _ui_sim_call_set_handler, NULL, \
		NULL, SIM_CALL_SET_VIEW, NORMAL_ORDER, UI_VIEW_LVGL, DEF_UI_VIEW_WIDTH, DEF_UI_VIEW_HEIGHT);
#endif

#if 1       //SIM_NETWORK_STATE_VIEW
static sim_tele_view_data *network_state_data = NULL;
const char sim_network_state_text[][30] =
{
    "是否驻网:",
    "运营商名称:",
    "IMS(VoLTE)驻网:",
    "PDP驻网:",
    "可选网络",
    "网络下行测速",
    "网络上行测速"
};

enum {
    SIM_NETWORK,
    SIM_NETWORK_OPERATOR,
    SIM_NETWORK_IMS,
    SIM_NETWORK_PDP,
    SIM_NETWORK_SELECT,
    SIM_NETWORK_DL_SPEED,
    SIM_NETWORK_UL_SPEED,
};
static void _ui_sim_network_composing(sim_tele_view_data *data)
{
    lv_coord_t layout_space = 150;
    lv_coord_t offset_space = 0;
    lv_coord_t text_num = sizeof(sim_network_state_text)/sizeof(sim_network_state_text[0]);
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
int32_t radio_start_search_cb(void *p_param, uint32_t size)
{
        ts_event_radio_t *p_radio_event = NULL;

        if(NULL == p_param) {
            alog_error("%s,p_param is null",__func__);
            return -1;
        }

        p_radio_event = (ts_event_radio_t *)p_param;

        switch(p_radio_event->event_id) {

        case TS_EVENT_RADIO_START_SEARCH_RESP: {
            uint32_t index = 0;
            ts_radio_start_search_resp_t *p_resp = NULL;
            ts_radio_operator_info_t *p_operator_info = NULL;
            char operator_list[200] = {0};
            char text[50] = {0};
            p_resp = (ts_radio_start_search_resp_t *)p_param;
            alog_info("[%s]event_id = 0x%x, ret_value = %d, operator_num = %d.",
                   __func__,
                   p_resp->event_id,
                   p_resp->ret_value,
                   p_resp->operator_num);
            if(network_state_data) {
                strcpy(network_state_data->operator_info,"");
            }
            for (index = 0; index < p_resp->operator_num; index++) {
                p_operator_info = (ts_radio_operator_info_t *)(&(p_resp->data_header)) + index;
                alog_info("[%s][%d]operator_short_name = %s, operator_long_name = %s, plmn_mcc = %s, plmn_mnc = %s, operator_id = %d, network_type = %d, status = %d.",
                       __func__,
                       index,
                       p_operator_info->operator_short_name,
                       p_operator_info->operator_long_name,
                       p_operator_info->plmn_mcc,
                       p_operator_info->plmn_mnc,
                       p_operator_info->operator_id,
                       p_operator_info->network_type,
                       p_operator_info->status);
                if(TS_RADIO_OPERATOR_ID_CMCC == p_operator_info->operator_id)
                    sprintf(text,"中国移动 %s", p_operator_info->operator_short_name);
                else if(TS_RADIO_OPERATOR_ID_CUCC == p_operator_info->operator_id)
                    sprintf(text,"中国联通 %s", p_operator_info->operator_short_name);
                else if(TS_RADIO_OPERATOR_ID_CTCC == p_operator_info->operator_id)
                    sprintf(text,"中国电信 %s", p_operator_info->operator_short_name);
                else
                    strcpy(text, p_operator_info->operator_short_name);

                snprintf(operator_list, sizeof(operator_list),"[%d] %s status:%d type:%d\nplmn:%s%s\n", index+1,text,
                    p_operator_info->status,p_operator_info->network_type,
                    p_operator_info->plmn_mcc, p_operator_info->plmn_mnc);

                if(network_state_data) {
                    strncat(network_state_data->operator_info, operator_list, sizeof(network_state_data->operator_info));
                }
            }
            if(network_state_data) {
                if(p_resp->operator_num == 0) {
                    strcpy(network_state_data->operator_info, "无可用网络");
                }
                ui_message_send_async(SIM_NETWORK_STATE_VIEW, MSG_VIEW_PAINT, 0);
            }
            break;
        }

        default:
            break;
    }
    return 0;
}

static void _sim_network_dl_speed_update(int errcode, int dlspeed)
{
    alog_info("[%s] dlspeed:%d\n", __func__, dlspeed);
    if(network_state_data) {
        if(errcode == SPEED_NO_ERROR) {
            snprintf(network_state_data->network_dl_speed, 50, "下行速度:%d字节/秒", dlspeed);
        } else {
            strncpy(network_state_data->network_dl_speed, "测速异常", 50);
        }
        alog_info("[%s] network_speed: %s\n",__func__, network_state_data->network_dl_speed);
        ui_message_send_async(SIM_NETWORK_STATE_VIEW, MSG_VIEW_PAINT, 0);
    }
}
static void _sim_network_ul_speed_update(int errcode, int ulspeed)
{
    alog_info("[%s] ulspeed:%d\n", __func__, ulspeed);
    if(network_state_data) {
        if(errcode == SPEED_NO_ERROR) {
            snprintf(network_state_data->network_ul_speed, 50, "上行速度:%d字节/秒", ulspeed);
        } else {
            strncpy(network_state_data->network_ul_speed, "测速异常", 50);
        }
        alog_info("[%s] network_speed: %s\n",__func__, network_state_data->network_ul_speed);
        ui_message_send_async(SIM_NETWORK_STATE_VIEW, MSG_VIEW_PAINT, 0);
    }
}

static void _sim_network_state_up(sim_tele_view_data *data,uint8_t id)
{
    if(data)
    {
        char text[600] = {0};
        switch (id)
        {
        case SIM_NETWORK:
            {
                ts_radio_reg_status_e p_reg_status = TS_RADIO_REG_STATUS_NO_SERVICE;
                sim_data.get_reg_status(0,&p_reg_status);
                if(p_reg_status == TS_RADIO_REG_STATUS_IN_SERVICE)
                    strcpy(text,"驻网成功");
                else
                    strcpy(text,"驻网失败");
            }
            break;
        case SIM_NETWORK_OPERATOR:
            {
                ts_radio_operator_info_t p_operator_info = {0};
                sim_data.get_operator_info(0,&p_operator_info);
                if(TS_RADIO_OPERATOR_ID_CMCC == p_operator_info.operator_id)
                    sprintf(text,"中国移动 %s", p_operator_info.operator_short_name);
                else if(TS_RADIO_OPERATOR_ID_CUCC == p_operator_info.operator_id)
                    sprintf(text,"中国联通 %s", p_operator_info.operator_short_name);
                else if(TS_RADIO_OPERATOR_ID_CTCC == p_operator_info.operator_id)
                    sprintf(text,"中国电信 %s", p_operator_info.operator_short_name);
                else
                    strcpy(text, p_operator_info.operator_short_name);
            }
            break;
        case SIM_NETWORK_IMS:
            if(sim_data.have_ims_reg(0))
                strcpy(text,"是");
            else
                strcpy(text,"否");
            break;
        case SIM_NETWORK_PDP:
            {
                ts_data_pdp_info_t p_pdp_info = {0};
                sim_data.get_internet_pdp_info(0,&p_pdp_info);
                if(p_pdp_info.pdp_status == TS_DATA_PDP_STATUS_DEACTIVED) {
                    strcpy(text,"否");
                } else {
                    snprintf(text, sizeof(text),"type: %s\n apn_name: %s\n gatewat :%s\nIPV4 addr: %s\n IPV4 prim_dns: %s\n IPV4 sec_dns: %s\nIPV6 addr: %s\n IPV6 prim_dns: %s\n IPV6 sec_dns: %s",
                       p_pdp_info.link_info.type,
                       p_pdp_info.link_info.apn_name,
                       p_pdp_info.link_info.gateway,
                       p_pdp_info.link_info.ipv4_addr,
                       p_pdp_info.link_info.ipv4_prim_dns,
                       p_pdp_info.link_info.ipv4_sec_dns,
                       p_pdp_info.link_info.ipv6_addr,
                       p_pdp_info.link_info.ipv6_prim_dns,
                       p_pdp_info.link_info.ipv6_sec_dns);
                }
            }
            break;
        case SIM_NETWORK_SELECT:
            if(data->operator_info[0] != '\0')
                strcpy(text,data->operator_info);
            else
                strcpy(text,"点击开始搜索网络");
            break;
        case SIM_NETWORK_DL_SPEED:
            if(data->network_dl_speed[0] != '\0')
                strcpy(text, data->network_dl_speed);
            else
                strcpy(text,"点击开始测速");
            break;
        case SIM_NETWORK_UL_SPEED:
            if(data->network_ul_speed[0] != '\0')
                strcpy(text, data->network_ul_speed);
            else
                strcpy(text,"点击开始测速");
            break;
        default:
            return;
        }
        text_canvas_set_text(data->up_text[id],text);
    }
}

static void _sim_network_state_btn_evt_handler(lv_event_t *e)
{
	int32_t btn_id = (int32_t)lv_obj_get_user_data(lv_event_get_current_target(e));
    sim_tele_view_data *data = lv_event_get_user_data(e);
    switch(btn_id)
    {
    case SIM_NETWORK_SELECT:
        sim_data.radio_start_search(0,radio_start_search_cb);
        strcpy(data->operator_info, "搜索中......");
        break;
    case SIM_NETWORK_DL_SPEED:
        sim_data.get_network_dl_speed( _sim_network_dl_speed_update);
        strcpy(data->network_dl_speed, "正在测速，请稍等......");
        break;
    case SIM_NETWORK_UL_SPEED:
        sim_data.get_network_ul_speed( _sim_network_ul_speed_update);
        strcpy(data->network_ul_speed, "正在测速，请稍等......");
        break;
    default:
        return;
    }
    _sim_network_state_up(data,btn_id);
}

static int _ui_sim_network_state_layout(view_data_t *view_data)
{
	lv_obj_t *scr = lv_display_get_screen_active(view_data->display);
	sim_tele_view_data *data = app_mem_malloc(sizeof(*data));
	if (!data) {
		return -ENOMEM;
	}
	memset(data, 0, sizeof(*data));

	view_data->user_data = data;
    network_state_data = data;

    LVGL_FONT_OPEN_DEFAULT(&data->font, 28);

    lv_obj_t *host_icon = lv_obj_create(scr);
    lv_obj_set_pos(host_icon, 0, 0);
    lv_obj_set_size(host_icon, DEF_UI_WIDTH, DEF_UI_HEIGHT);
    lv_obj_set_style_pad_all(host_icon, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(host_icon, 0, LV_PART_MAIN);

    lv_coord_t text_num = sizeof(sim_network_state_text)/sizeof(sim_network_state_text[0]);
    for(uint32_t i = 0 ; i < text_num ; i++)
    {
        lv_obj_t *obj_icon = lv_obj_create(host_icon);
		lv_obj_set_width(obj_icon,420);
		lv_obj_set_style_pad_all(obj_icon,0,LV_PART_MAIN);
		lv_obj_set_style_border_width(obj_icon,0,LV_PART_MAIN);
		lv_obj_set_style_radius(obj_icon,20,LV_PART_MAIN);
		lv_obj_set_style_bg_color(obj_icon,lv_color_hex(0x3B3B3B),LV_PART_MAIN);
		lv_obj_set_style_bg_opa(obj_icon,LV_OPA_100,LV_PART_MAIN);
		lv_obj_add_event_cb(obj_icon, _sim_network_state_btn_evt_handler, LV_EVENT_SHORT_CLICKED, data);
		lv_obj_set_user_data(obj_icon,(void *)i);

        lv_obj_t *text_icon = text_canvas_create(obj_icon);
        lv_obj_align(text_icon,LV_ALIGN_TOP_MID,0,20);
        lv_obj_set_width(text_icon,420);
        lv_obj_set_style_text_align(text_icon,LV_TEXT_ALIGN_CENTER,LV_PART_MAIN);
		lv_obj_set_style_text_color(text_icon,lv_color_white(),LV_PART_MAIN);
		lv_obj_set_style_text_font(text_icon,&data->font,LV_PART_MAIN);
        text_canvas_set_long_mode(text_icon,TEXT_CANVAS_LONG_SCROLL_CIRCULAR);
        text_canvas_set_text(text_icon,sim_network_state_text[i]);
        lv_obj_add_flag(text_icon,LV_OBJ_FLAG_EVENT_BUBBLE);


        data->up_text[i] = text_canvas_create(obj_icon);
        lv_obj_align(data->up_text[i],LV_ALIGN_BOTTOM_MID,0,-20);
        lv_obj_set_width(data->up_text[i],420);
        lv_obj_set_style_text_align(data->up_text[i],LV_TEXT_ALIGN_CENTER,LV_PART_MAIN);
		lv_obj_set_style_text_color(data->up_text[i],lv_color_white(),LV_PART_MAIN);
		lv_obj_set_style_text_font(data->up_text[i],&data->font,LV_PART_MAIN);
        text_canvas_set_long_mode(data->up_text[i],TEXT_CANVAS_LONG_WRAP);
        lv_obj_add_flag(data->up_text[i],LV_OBJ_FLAG_EVENT_BUBBLE);
        _sim_network_state_up(data,i);
    }
    _ui_sim_network_composing(data);
    return 0;
}

static int _ui_sim_network_state_delete(view_data_t *view_data)
{
    sim_tele_view_data *data = view_data->user_data;
    if (data) {
        lv_obj_t *scr = lv_display_get_screen_active(view_data->display);
        lv_obj_clean(scr);
        LVGL_FONT_CLOSE(&data->font);
        app_mem_free(data);
    }
    view_data->user_data = NULL;
    network_state_data = NULL;
    return 0;
}

static int _ui_sim_network_state_paint(view_data_t *view_data)
{
    if(network_state_data->operator_info[0] != '\0')
        text_canvas_set_text(network_state_data->up_text[SIM_NETWORK_SELECT],network_state_data->operator_info);
    if(network_state_data->network_dl_speed[0] != '\0')
        text_canvas_set_text(network_state_data->up_text[SIM_NETWORK_DL_SPEED],network_state_data->network_dl_speed);
    if(network_state_data->network_ul_speed[0] != '\0')
        text_canvas_set_text(network_state_data->up_text[SIM_NETWORK_UL_SPEED],network_state_data->network_ul_speed);
    _ui_sim_network_composing(view_data->user_data);
    return 0;
}

int _ui_sim_network_state_handler(uint16_t view_id, view_data_t *view_data, uint8_t msg_id, void * msg_data)
{
    int ret = 0;
	switch (msg_id) {
	case MSG_VIEW_PRELOAD:
        ret = ui_view_layout(SIM_NETWORK_STATE_VIEW);
        break;
	case MSG_VIEW_LAYOUT:
        ret = _ui_sim_network_state_layout(view_data);
        break;
	case MSG_VIEW_DELETE:
        ret = _ui_sim_network_state_delete(view_data);
        break;
    case MSG_VIEW_PAINT:
        ret = _ui_sim_network_state_paint(view_data);
	default:
        break;
	}
	return ret;
}

VIEW_DEFINE2(ui_sim_network_state, _ui_sim_network_state_handler, NULL, \
		NULL, SIM_NETWORK_STATE_VIEW, NORMAL_ORDER, UI_VIEW_LVGL, DEF_UI_VIEW_WIDTH, DEF_UI_VIEW_HEIGHT);
#endif

#if 1       //SIM_HOUSING_INFORMATION_VIEW
static sim_tele_view_data *housing_information_data = NULL;
const char sim_housing_information_text[][30] =
{
    "当区信息:",
    "邻区信息:",
};

enum {
    SIM_AT_HOUSING_INFORMATION,
    SIM_NEIGHBOUR_INFORMATION,
};

static int32_t srv_housing_func_cb(void *p_param, uint32_t size)
{
    ts_event_radio_t *p_radio_event = NULL;

    if(NULL == p_param) {
        alog_error("%s,p_param is null",__func__);
        return -1;
    }

    p_radio_event = (ts_event_radio_t *)p_param;

    switch(p_radio_event->event_id) {
    case TS_EVENT_RADIO_GET_CELL_INFO_RESP: {
        ts_radio_get_cell_info_resp_t *p_cell_info = NULL;
        p_cell_info = (ts_radio_get_cell_info_resp_t *)p_param;
        if(housing_information_data) {
            sprintf(housing_information_data->sim_housing_info, "rat type:%d plmn:%s%s\ntac:%d earfcn:%d\n"
                            "cellid:%d rsrp:%d\nrsrq:%d pcid:%d\n is_roaming:%d band:%d\n"
                            "ul_bandwidth:%d dl_bandwidth:%d\nsinr:%d srxlev:%d rssi:%d",
                p_cell_info->cell_info.rat_type,
                p_cell_info->cell_info.plmn_mcc,
                p_cell_info->cell_info.plmn_mnc,
                p_cell_info->cell_info.tac,
                p_cell_info->cell_info.earfcn,
                p_cell_info->cell_info.cellid,
                p_cell_info->cell_info.rsrp,
                p_cell_info->cell_info.rsrq,
                p_cell_info->cell_info.pcid,
                p_cell_info->cell_info.is_roaming,
                p_cell_info->cell_info.band,
                p_cell_info->cell_info.ul_bandwidth,
                p_cell_info->cell_info.dl_bandwidth,
                p_cell_info->cell_info.sinr,
                p_cell_info->cell_info.srxlev,
                p_cell_info->cell_info.rssi);
            alog_info("[%s] sim_housing_info = %s\n", __func__, housing_information_data->sim_housing_info);
            ui_message_send_async(SIM_HOUSING_INFORMATION_VIEW, MSG_VIEW_PAINT, 0);
        }
        break;
    }
    case TS_EVENT_RADIO_GET_NEIGHBOR_CELL_INFO_LIST_RESP: {
        ts_radio_get_neighbor_cell_info_list_resp_t *p_nb_cell_info_list = NULL;
        uint8_t num = 0;
        char ncell_info_list[300] = {0};
        p_nb_cell_info_list = (ts_radio_get_neighbor_cell_info_list_resp_t *)p_param;
        alog_info("[%s]ncell_num = %d.", __func__, p_nb_cell_info_list->ncell_info_list.ncell_num);

        if(housing_information_data) {
            snprintf(housing_information_data->sim_neighbour_info, sizeof(housing_information_data->sim_neighbour_info),
                        "num:%d\n", p_nb_cell_info_list->ncell_info_list.ncell_num);
        }
        for (num = 0; num < p_nb_cell_info_list->ncell_info_list.ncell_num; num++) {

            snprintf(ncell_info_list, sizeof(ncell_info_list),"[index:%d] rat type:%d\nplmn:%s%s\ntac:%d earfcn:%d\n"
                            "cellid:%d rsrp:%d\nrsrq:%d pcid:%d\nsinr:%d srxlev:%d rssi:%d\n",
                num,
                p_nb_cell_info_list->ncell_info_list.ncell_info[num].rat_type,
                p_nb_cell_info_list->ncell_info_list.ncell_info[num].plmn_mcc,
                p_nb_cell_info_list->ncell_info_list.ncell_info[num].plmn_mnc,
                p_nb_cell_info_list->ncell_info_list.ncell_info[num].tac,
                p_nb_cell_info_list->ncell_info_list.ncell_info[num].earfcn,
                p_nb_cell_info_list->ncell_info_list.ncell_info[num].cellid,
                p_nb_cell_info_list->ncell_info_list.ncell_info[num].rsrp,
                p_nb_cell_info_list->ncell_info_list.ncell_info[num].rsrq,
                p_nb_cell_info_list->ncell_info_list.ncell_info[num].pcid,
                p_nb_cell_info_list->ncell_info_list.ncell_info[num].sinr,
                p_nb_cell_info_list->ncell_info_list.ncell_info[num].srxlev,
                p_nb_cell_info_list->ncell_info_list.ncell_info[num].rssi);

            if(housing_information_data) {
                strncat(housing_information_data->sim_neighbour_info, ncell_info_list, sizeof(housing_information_data->sim_neighbour_info));
            }

        }

        if(housing_information_data) {
            alog_info("[%s] sim_neighbour_info = %s\n", __func__, housing_information_data->sim_neighbour_info);
            ui_message_send_async(SIM_HOUSING_INFORMATION_VIEW, MSG_VIEW_PAINT, 0);
        }
        break;
    }
    default:
        break;
    }
    return 0;
}

static void _ui_sim_housing_infomation_composing(sim_tele_view_data *data)
{
    lv_coord_t layout_space = 150;
    lv_coord_t offset_space = 0;
    lv_coord_t text_num = sizeof(sim_housing_information_text)/sizeof(sim_housing_information_text[0]);
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

static void _sim_housing_information_up(sim_tele_view_data *data,uint8_t id)
{
    if(data)
    {
        char text[100] = {0};
        switch (id)
        {
        case SIM_AT_HOUSING_INFORMATION:
            sim_data.get_cell_info(0,srv_housing_func_cb);
            strcpy(text,"搜索中");
            break;
        case SIM_NEIGHBOUR_INFORMATION:
            sim_data.get_neighbor_cell_info_list(0,srv_housing_func_cb);
            strcpy(text,"搜索中");
            break;
        default:
            return;
        }
        text_canvas_set_text(data->up_text[id],text);
    }
}

static void _sim_housing_information_btn_evt_handler(lv_event_t *e)
{
	//int32_t btn_id = (int32_t)lv_obj_get_user_data(lv_event_get_current_target(e));
    //sim_tele_view_data *data = lv_event_get_user_data(e);
    //_sim_housing_information_up(data,btn_id);
}

static int _ui_sim_housing_information_layout(view_data_t *view_data)
{
	lv_obj_t *scr = lv_display_get_screen_active(view_data->display);
	sim_tele_view_data *data = app_mem_malloc(sizeof(*data));
	if (!data) {
		return -ENOMEM;
	}
	memset(data, 0, sizeof(*data));
    housing_information_data = data;
	view_data->user_data = data;

    LVGL_FONT_OPEN_DEFAULT(&data->font, 28);

    lv_obj_t *host_icon = lv_obj_create(scr);
    lv_obj_set_pos(host_icon, 0, 0);
    lv_obj_set_size(host_icon, DEF_UI_WIDTH, DEF_UI_HEIGHT);
    lv_obj_set_style_pad_all(host_icon, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(host_icon, 0, LV_PART_MAIN);
    lv_coord_t text_num = sizeof(sim_housing_information_text)/sizeof(sim_housing_information_text[0]);
    for(uint32_t i = 0 ; i < text_num ; i++)
    {
        lv_obj_t *obj_icon = lv_obj_create(host_icon);
		lv_obj_set_width(obj_icon,420);
		lv_obj_set_style_pad_all(obj_icon,0,LV_PART_MAIN);
		lv_obj_set_style_border_width(obj_icon,0,LV_PART_MAIN);
		lv_obj_set_style_radius(obj_icon,20,LV_PART_MAIN);
		lv_obj_set_style_bg_color(obj_icon,lv_color_hex(0x3B3B3B),LV_PART_MAIN);
		lv_obj_set_style_bg_opa(obj_icon,LV_OPA_100,LV_PART_MAIN);
		lv_obj_add_event_cb(obj_icon, _sim_housing_information_btn_evt_handler, LV_EVENT_SHORT_CLICKED, data);
		lv_obj_set_user_data(obj_icon,(void *)i);

        lv_obj_t *text_icon = text_canvas_create(obj_icon);
        lv_obj_align(text_icon,LV_ALIGN_TOP_MID,0,20);
        lv_obj_set_width(text_icon,420);
        lv_obj_set_style_text_align(text_icon,LV_TEXT_ALIGN_CENTER,LV_PART_MAIN);
		lv_obj_set_style_text_color(text_icon,lv_color_white(),LV_PART_MAIN);
		lv_obj_set_style_text_font(text_icon,&data->font,LV_PART_MAIN);
        text_canvas_set_long_mode(text_icon,TEXT_CANVAS_LONG_SCROLL_CIRCULAR);
        text_canvas_set_text(text_icon,sim_housing_information_text[i]);
        lv_obj_add_flag(text_icon,LV_OBJ_FLAG_EVENT_BUBBLE);


        data->up_text[i] = text_canvas_create(obj_icon);
        lv_obj_align(data->up_text[i],LV_ALIGN_BOTTOM_MID,0,-20);
        lv_obj_set_width(data->up_text[i],420);
        lv_obj_set_style_text_align(data->up_text[i],LV_TEXT_ALIGN_CENTER,LV_PART_MAIN);
		lv_obj_set_style_text_color(data->up_text[i],lv_color_white(),LV_PART_MAIN);
		lv_obj_set_style_text_font(data->up_text[i],&data->font,LV_PART_MAIN);
        text_canvas_set_long_mode(data->up_text[i],TEXT_CANVAS_LONG_WRAP);
        lv_obj_add_flag(data->up_text[i],LV_OBJ_FLAG_EVENT_BUBBLE);
        _sim_housing_information_up(data,i);
    }

    _ui_sim_housing_infomation_composing(data);
    return 0;
}

static int _ui_sim_housing_information_delete(view_data_t *view_data)
{
    lv_obj_t *scr = lv_display_get_screen_active(view_data->display);
    housing_information_data = NULL;
    lv_obj_clean(scr);
    sim_tele_view_data *data = view_data->user_data;
	if (data) {
        LVGL_FONT_CLOSE(&data->font);
        app_mem_free(data);
    }
    view_data->user_data = NULL;
    return 0;
}

static int _ui_sim_housing_information_paint(view_data_t * view_data)
{
    text_canvas_set_text(housing_information_data->up_text[SIM_AT_HOUSING_INFORMATION],housing_information_data->sim_housing_info);

    text_canvas_set_text(housing_information_data->up_text[SIM_NEIGHBOUR_INFORMATION],housing_information_data->sim_neighbour_info);

    _ui_sim_housing_infomation_composing(view_data->user_data);

    return 0;
}

int _ui_sim_housing_information_handler(uint16_t view_id, view_data_t *view_data, uint8_t msg_id, void * msg_data)
{
    int ret = 0;
	switch (msg_id) {
	case MSG_VIEW_PRELOAD:
        ret = ui_view_layout(SIM_HOUSING_INFORMATION_VIEW);
        break;
	case MSG_VIEW_LAYOUT:
        ret = _ui_sim_housing_information_layout(view_data);
        break;
	case MSG_VIEW_DELETE:
        ret = _ui_sim_housing_information_delete(view_data);
        break;
    case MSG_VIEW_PAINT:
        ret = _ui_sim_housing_information_paint(view_data);
	default:
        break;
	}
	return ret;
}

VIEW_DEFINE2(ui_sim_housing_information, _ui_sim_housing_information_handler, NULL, \
		NULL, SIM_HOUSING_INFORMATION_VIEW, NORMAL_ORDER, UI_VIEW_LVGL, DEF_UI_VIEW_WIDTH, DEF_UI_VIEW_HEIGHT);
#endif

#if 1       //SIM_SIGNAL_STATE_VIEW
const char sim_signal_state_text[][30] =
{
    "网络类型:",
    "信号等级:",
    "信号值:",
    "信号强度:",
};

static void _sim_signal_state_up(sim_tele_view_data *data)
{
    if(data)
    {
        ts_radio_signal_info_t p_signal_info = {0};
        sim_data.get_signal_info(0,&p_signal_info);

        if(p_signal_info.signal_type == TS_RADIO_NETWORK_TYPE_GSM)
            text_canvas_set_text(data->up_text[0],"GMS");
        else if(p_signal_info.signal_type == TS_RADIO_NETWORK_TYPE_CDMA
            || p_signal_info.signal_type == TS_RADIO_NETWORK_TYPE_WCDMA
            || p_signal_info.signal_type == TS_RADIO_NETWORK_TYPE_TDSCDMA)
            text_canvas_set_text(data->up_text[0],"CDMA");
        else if(p_signal_info.signal_type == TS_RADIO_NETWORK_TYPE_LTE)
            text_canvas_set_text(data->up_text[0],"4G");
        else if(p_signal_info.signal_type == TS_RADIO_NETWORK_TYPE_NR)
            text_canvas_set_text(data->up_text[0],"5G");
        else
            text_canvas_set_text(data->up_text[0],"未知");

        if(p_signal_info.signal_level == 4)
            text_canvas_set_text(data->up_text[1],"4");
        else if(p_signal_info.signal_level == 3)
            text_canvas_set_text(data->up_text[1],"3");
        else if(p_signal_info.signal_level == 2)
            text_canvas_set_text(data->up_text[1],"2");
        else if(p_signal_info.signal_level == 1)
            text_canvas_set_text(data->up_text[1],"1");
        else
            text_canvas_set_text(data->up_text[1],"0");

        text_canvas_set_text_fmt(data->up_text[2],"%ddB",p_signal_info.signal_level_val);

        text_canvas_set_text_fmt(data->up_text[3],"%ddB",p_signal_info.signal_strength);
    }
}

static void _sim_signal_state_btn_evt_handler(lv_event_t *e)
{
    sim_tele_view_data *data = lv_event_get_user_data(e);
    _sim_signal_state_up(data);
}

static int _ui_sim_signal_state_layout(view_data_t *view_data)
{
	lv_obj_t *scr = lv_display_get_screen_active(view_data->display);
	sim_tele_view_data *data = app_mem_malloc(sizeof(*data));
	if (!data) {
		return -ENOMEM;
	}
	memset(data, 0, sizeof(*data));

	view_data->user_data = data;

    LVGL_FONT_OPEN_DEFAULT(&data->font, 28);

    lv_coord_t layout_space = 150;
    lv_coord_t text_num = sizeof(sim_signal_state_text)/sizeof(sim_signal_state_text[0]);
    for(uint32_t i = 0 ; i < text_num ; i++)
    {
        lv_obj_t *obj_icon = lv_obj_create(scr);
		lv_obj_set_pos(obj_icon,24,60 + layout_space * i);
		lv_obj_set_size(obj_icon,420,120);
		lv_obj_set_style_pad_all(obj_icon,0,LV_PART_MAIN);
		lv_obj_set_style_border_width(obj_icon,0,LV_PART_MAIN);
		lv_obj_set_style_radius(obj_icon,20,LV_PART_MAIN);
		lv_obj_set_style_bg_color(obj_icon,lv_color_hex(0x3B3B3B),LV_PART_MAIN);
		lv_obj_set_style_bg_opa(obj_icon,LV_OPA_100,LV_PART_MAIN);
		lv_obj_add_event_cb(obj_icon, _sim_signal_state_btn_evt_handler, LV_EVENT_SHORT_CLICKED, data);
		lv_obj_set_user_data(obj_icon,(void *)i);

        lv_obj_t *text_icon = text_canvas_create(obj_icon);
        lv_obj_align(text_icon,LV_ALIGN_TOP_MID,0,20);
        lv_obj_set_width(text_icon,420);
        lv_obj_set_style_text_align(text_icon,LV_TEXT_ALIGN_CENTER,LV_PART_MAIN);
		lv_obj_set_style_text_color(text_icon,lv_color_white(),LV_PART_MAIN);
		lv_obj_set_style_text_font(text_icon,&data->font,LV_PART_MAIN);
        text_canvas_set_long_mode(text_icon,TEXT_CANVAS_LONG_SCROLL_CIRCULAR);
        text_canvas_set_text(text_icon,sim_signal_state_text[i]);
        lv_obj_add_flag(text_icon,LV_OBJ_FLAG_EVENT_BUBBLE);


        data->up_text[i] = text_canvas_create(obj_icon);
        lv_obj_align(data->up_text[i],LV_ALIGN_BOTTOM_MID,0,-20);
        lv_obj_set_width(data->up_text[i],420);
        lv_obj_set_style_text_align(data->up_text[i],LV_TEXT_ALIGN_CENTER,LV_PART_MAIN);
		lv_obj_set_style_text_color(data->up_text[i],lv_color_white(),LV_PART_MAIN);
		lv_obj_set_style_text_font(data->up_text[i],&data->font,LV_PART_MAIN);
        text_canvas_set_long_mode(data->up_text[i],TEXT_CANVAS_LONG_SCROLL_CIRCULAR);
        lv_obj_add_flag(data->up_text[i],LV_OBJ_FLAG_EVENT_BUBBLE);
    }
    _sim_signal_state_up(data);

    lv_obj_t *obj_icon = lv_obj_create(scr);
	lv_obj_set_pos(obj_icon,0,60 + layout_space * text_num);
	lv_obj_set_size(obj_icon,4,4);
    return 0;
}

static int _ui_sim_signal_state_delete(view_data_t *view_data)
{
    lv_obj_t *scr = lv_display_get_screen_active(view_data->display);
    lv_obj_clean(scr);
    sim_tele_view_data *data = view_data->user_data;
	if (data) {
        LVGL_FONT_CLOSE(&data->font);
        app_mem_free(data);
    }
    view_data->user_data = NULL;
    return 0;
}

int _ui_sim_signal_state_handler(uint16_t view_id, view_data_t *view_data, uint8_t msg_id, void * msg_data)
{
    int ret = 0;
	switch (msg_id) {
	case MSG_VIEW_PRELOAD:
        ret = ui_view_layout(SIM_SIGNAL_STATE_VIEW);
        break;
	case MSG_VIEW_LAYOUT:
        ret = _ui_sim_signal_state_layout(view_data);
        break;
	case MSG_VIEW_DELETE:
        ret = _ui_sim_signal_state_delete(view_data);
        break;
	default:
        break;
	}
	return ret;
}

VIEW_DEFINE21(ui_sim_signal_state, _ui_sim_signal_state_handler, NULL, \
		NULL, SIM_SIGNAL_STATE_VIEW, NORMAL_ORDER, UI_VIEW_LVGL, DEF_UI_VIEW_WIDTH, DEF_UI_VIEW_HEIGHT);
#endif

#if 1       //SIM_CARD_INFORMATION_VIEW
const char sim_card_information_text[][30] =
{
    "Sim卡状态:",
    "IMSI:",
    "本机号码:",
};

enum {
    SIM_EXISTS,
    SIM_IMSI,
    SIM_NUMBER,
};

static void _sim_card_information_up(sim_tele_view_data *data,uint8_t id)
{
    if(data)
    {
        char text[100] = {0};
        switch (id)
        {
        case SIM_EXISTS:
            {
                ts_sim_status_e p_sim_status = TS_SIM_STATUS_NOT_READY;
                sim_data.sim_get_status(0,&p_sim_status);
                 if(p_sim_status == TS_SIM_STATUS_NOT_PRESENT)
                    sprintf(text, "%d 卡已拔出",p_sim_status);
                 else if(p_sim_status == TS_SIM_STATUS_READY)
                    sprintf(text, "%d 卡存在",p_sim_status);
                else if(p_sim_status == TS_SIM_STATUS_PIN || p_sim_status == TS_SIM_STATUS_PIN2)
                    sprintf(text, "%d PIN锁",p_sim_status);
                else if(p_sim_status == TS_SIM_STATUS_PUK || p_sim_status == TS_SIM_STATUS_PUK2)
                    sprintf(text, "%d PUK锁",p_sim_status);
                else
                    sprintf(text, "%d 未知状态",p_sim_status);
            }
            break;
        case SIM_IMSI:
            sim_data.sim_get_imsi(0,text);
            break;
        case SIM_NUMBER:
            sim_data.sim_get_msisdn(0,text);
            break;
        default:
            return;
        }
        text_canvas_set_text(data->up_text[id],text);
    }
}

static void _sim_card_information_btn_evt_handler(lv_event_t *e)
{
	int32_t btn_id = (int32_t)lv_obj_get_user_data(lv_event_get_current_target(e));
    sim_tele_view_data *data = lv_event_get_user_data(e);
    _sim_card_information_up(data,btn_id);
}

static int _ui_sim_card_information_layout(view_data_t *view_data)
{
	lv_obj_t *scr = lv_display_get_screen_active(view_data->display);
	sim_tele_view_data *data = app_mem_malloc(sizeof(*data));
	if (!data) {
		return -ENOMEM;
	}
	memset(data, 0, sizeof(*data));

	view_data->user_data = data;

    LVGL_FONT_OPEN_DEFAULT(&data->font, 28);

    lv_coord_t layout_space = 150;
    lv_coord_t text_num = sizeof(sim_card_information_text)/sizeof(sim_card_information_text[0]);
    for(uint32_t i = 0 ; i < text_num ; i++)
    {
        lv_obj_t *obj_icon = lv_obj_create(scr);
		lv_obj_set_pos(obj_icon,24,60 + layout_space * i);
		lv_obj_set_size(obj_icon,420,120);
		lv_obj_set_style_pad_all(obj_icon,0,LV_PART_MAIN);
		lv_obj_set_style_border_width(obj_icon,0,LV_PART_MAIN);
		lv_obj_set_style_radius(obj_icon,20,LV_PART_MAIN);
		lv_obj_set_style_bg_color(obj_icon,lv_color_hex(0x3B3B3B),LV_PART_MAIN);
		lv_obj_set_style_bg_opa(obj_icon,LV_OPA_100,LV_PART_MAIN);
		lv_obj_add_event_cb(obj_icon, _sim_card_information_btn_evt_handler, LV_EVENT_SHORT_CLICKED, data);
		lv_obj_set_user_data(obj_icon,(void *)i);

        lv_obj_t *text_icon = text_canvas_create(obj_icon);
        lv_obj_align(text_icon,LV_ALIGN_TOP_MID,0,20);
        lv_obj_set_width(text_icon,420);
        lv_obj_set_style_text_align(text_icon,LV_TEXT_ALIGN_CENTER,LV_PART_MAIN);
		lv_obj_set_style_text_color(text_icon,lv_color_white(),LV_PART_MAIN);
		lv_obj_set_style_text_font(text_icon,&data->font,LV_PART_MAIN);
        text_canvas_set_long_mode(text_icon,TEXT_CANVAS_LONG_SCROLL_CIRCULAR);
        text_canvas_set_text(text_icon,sim_card_information_text[i]);
        lv_obj_add_flag(text_icon,LV_OBJ_FLAG_EVENT_BUBBLE);


        data->up_text[i] = text_canvas_create(obj_icon);
        lv_obj_align(data->up_text[i],LV_ALIGN_BOTTOM_MID,0,-20);
        lv_obj_set_width(data->up_text[i],420);
        lv_obj_set_style_text_align(data->up_text[i],LV_TEXT_ALIGN_CENTER,LV_PART_MAIN);
		lv_obj_set_style_text_color(data->up_text[i],lv_color_white(),LV_PART_MAIN);
		lv_obj_set_style_text_font(data->up_text[i],&data->font,LV_PART_MAIN);
        text_canvas_set_long_mode(data->up_text[i],TEXT_CANVAS_LONG_SCROLL_CIRCULAR);
        lv_obj_add_flag(data->up_text[i],LV_OBJ_FLAG_EVENT_BUBBLE);
        _sim_card_information_up(data,i);
    }

    lv_obj_t *obj_icon = lv_obj_create(scr);
	lv_obj_set_pos(obj_icon,0,60 + layout_space * text_num);
	lv_obj_set_size(obj_icon,4,4);
    return 0;
}

static int _ui_sim_card_information_delete(view_data_t *view_data)
{
    lv_obj_t *scr = lv_display_get_screen_active(view_data->display);
    lv_obj_clean(scr);
    sim_tele_view_data *data = view_data->user_data;
	if (data) {
        LVGL_FONT_CLOSE(&data->font);
        app_mem_free(data);
    }
    view_data->user_data = NULL;
    return 0;
}

int _ui_sim_card_information_handler(uint16_t view_id, view_data_t *view_data, uint8_t msg_id, void * msg_data)
{
    int ret = 0;
	switch (msg_id) {
	case MSG_VIEW_PRELOAD:
        ret = ui_view_layout(SIM_CARD_INFORMATION_VIEW);
        break;
	case MSG_VIEW_LAYOUT:
        ret = _ui_sim_card_information_layout(view_data);
        break;
	case MSG_VIEW_DELETE:
        ret = _ui_sim_card_information_delete(view_data);
        break;
	default:
        break;
	}
	return ret;
}

VIEW_DEFINE2(ui_sim_card_information, _ui_sim_card_information_handler, NULL, \
		NULL, SIM_CARD_INFORMATION_VIEW, NORMAL_ORDER, UI_VIEW_LVGL, DEF_UI_VIEW_WIDTH, DEF_UI_VIEW_HEIGHT);
#endif

#if 1       //SIM_TELE_VIEW
const char sim_tele_text[][30] =
{
    "Sim卡信息",
    "信号状态",
    "小区信息",
    "驻网开关",
    "驻网状态",
    "Call设置",
    "SMS设置"
};
static ts_call_info_list_t s_call_info_list = {0};

enum {
    SIM_CARD_INFORMATION,
    SIM_SIGNAL_STATE,
    SIM_HOUSING_INFORMATION,
    SIM_RADIO_SET,
    SIM_NETWORK_STATE,
    SIM_CALL_SET,
    SIM_CARD_SET,
};

static int32_t handleRadioCallback(void *p_param, uint32_t size)
{
    ts_event_radio_t *p_radio_event = NULL;

    if(NULL == p_param) {
        alog_error("%s,p_param is null",__func__);
        return -1;
    }

    p_radio_event = (ts_event_radio_t *)p_param;

    switch(p_radio_event->event_id) {
        case TS_EVENT_RADIO_SIGNAL_INFO_UPDATED: /* signal strenth&level reported per 10s */
        {
            ts_radio_signal_info_updated_t *p_signal_info = NULL;
            alog_info("[%s],TS_EVENT_RADIO_SIGNAL_INFO_UPDATED",__func__);

            p_signal_info = (ts_radio_signal_info_updated_t *)p_param;
            alog_info("[%s], signal_strength = %d, signal_level = %d.\r\n",
                     __func__,
                     p_signal_info->signal_strength,
                     p_signal_info->signal_level);

            /* signal level0: signal strenth <=-120dBm */
            /* signal level1: signal strenth -111~-119dBm */
            /* signal level2: signal strenth -106~-110dBm */
            /* signal level3: signal strenth -91~-105dBm */
            /* signal level4: signal strenth >=-90dBm */

            /* update signal
               to ui task: update signal level */
            break;
        }
        case TS_EVENT_RADIO_REG_STATUS_UPDATED: /* register net status reported */
        {
            ts_radio_reg_status_updated_t *p_reg_status = NULL;
            alog_info("[%s],TS_EVENT_RADIO_REG_STATUS_UPDATED",__func__);

            p_reg_status = (ts_radio_reg_status_updated_t *)p_param;
            alog_info("[%s],reg status = %d.\r\n",__func__,p_reg_status->reg_status);
            if(TS_RADIO_REG_STATUS_IN_SERVICE == p_reg_status->reg_status) {
                /* register successfual
                  to ui task: update reg status icon */
            } else {
                /* No register
                  to ui task: update reg status icon */
            }
            break;
        }
        case TS_EVENT_RADIO_IMS_REG_STATUS_UPDATED: /* ims status reported */
        {
            ts_radio_ims_reg_status_updated_t *p_ims_status = NULL;
            alog_info("[%s],TS_EVENT_RADIO_IMS_REG_STATUS_UPDATED",__func__);

            p_ims_status = (ts_radio_ims_reg_status_updated_t *)p_param;
            alog_info("[%s],ims status = %d.\r\n",__func__,p_ims_status->is_ims_on);
            if(true == p_ims_status->is_ims_on) {
                /* ims open
                  to ui task: update ims icon */
            } else {
                /* ims close
                  to ui task: update ims icon */
            }
            break;
        }
        case TS_EVENT_RADIO_OPERATOR_INFO_UPDATED: /* openator name(eg,cmcc,cucc,ctcc) reported*/
        {
            ts_radio_operator_info_updated_t *p_operator_name = NULL;
            alog_info("[%s],TS_EVENT_RADIO_OPERATOR_INFO_UPDATED",__func__);
            p_operator_name = (ts_radio_operator_info_updated_t *)p_param;
            alog_info("[%s],operator short name = %s, long name = %s,",__func__,
                p_operator_name->operator_info.operator_short_name,/*cmcc/cucc/ctcc*/
                p_operator_name->operator_info.operator_long_name);/*China Mobile/China Unicom/china Telecom*/

            /* operator name
               to ui task: display operator name */
            break;
        }

        case TS_EVENT_RADIO_GET_CELL_INFO_RESP: {
            ts_radio_get_cell_info_resp_t *p_cell_info = NULL;

            alog_info("[%s],TS_EVENT_RADIO_GET_CELL_INFO_RESP",__func__);
            p_cell_info = (ts_radio_get_cell_info_resp_t *)p_param;
            alog_info("[%s]ret_value = %d, rat_type = %d, plmn_mcc = %s, plmn_mnc = %s, tac = %d, earfcn = %d, cellid = %d, rsrp = %d, rsrq = %d, pcid = %d, is_roaming = %d, band = %d, ul_bandwidth = %d, dl_bandwidth = %d, sinr = %d, srxlev = %d, rssi = %d.",
                      __func__,
                      p_cell_info->ret_value,
                      p_cell_info->cell_info.rat_type,
                      p_cell_info->cell_info.plmn_mcc,
                      p_cell_info->cell_info.plmn_mnc,
                      p_cell_info->cell_info.tac,
                      p_cell_info->cell_info.earfcn,
                      p_cell_info->cell_info.cellid,
                      p_cell_info->cell_info.rsrp,
                      p_cell_info->cell_info.rsrq,
                      p_cell_info->cell_info.pcid,
                      p_cell_info->cell_info.is_roaming,
                      p_cell_info->cell_info.band,
                      p_cell_info->cell_info.ul_bandwidth,
                      p_cell_info->cell_info.dl_bandwidth,
                      p_cell_info->cell_info.sinr,
                      p_cell_info->cell_info.srxlev,
                      p_cell_info->cell_info.rssi);
            break;
        }

        case TS_EVENT_RADIO_GET_NEIGHBOR_CELL_INFO_LIST_RESP: {
            ts_radio_get_neighbor_cell_info_list_resp_t *p_nb_cell_info_list = NULL;
            uint8_t num = 0;

            alog_info("[%s],TS_EVENT_RADIO_GET_NEIGHBOR_CELL_INFO_LIST_RESP",__func__);
            p_nb_cell_info_list = (ts_radio_get_neighbor_cell_info_list_resp_t *)p_param;
            alog_info("[%s]ret_value = %d.", __func__, p_nb_cell_info_list->ret_value);
            alog_info("[%s]ncell_num = %d.", __func__, p_nb_cell_info_list->ncell_info_list.ncell_num);
            for (num = 0; num < p_nb_cell_info_list->ncell_info_list.ncell_num; num++) {
                alog_info("[%s]index = %d, rat_type = %d, plmn_mcc = %s, plmn_mnc = %s, tac = %d, cellid = %d, earfcn = %d, rsrp = %d, rsrq = %d, pcid = %d, sinr = %d, srxlev = %d, rssi = %d.",
                       __func__,
                       num,
                       p_nb_cell_info_list->ncell_info_list.ncell_info[num].rat_type,
                       p_nb_cell_info_list->ncell_info_list.ncell_info[num].plmn_mcc,
                       p_nb_cell_info_list->ncell_info_list.ncell_info[num].plmn_mnc,
                       p_nb_cell_info_list->ncell_info_list.ncell_info[num].tac,
                       p_nb_cell_info_list->ncell_info_list.ncell_info[num].cellid,
                       p_nb_cell_info_list->ncell_info_list.ncell_info[num].earfcn,
                       p_nb_cell_info_list->ncell_info_list.ncell_info[num].rsrp,
                       p_nb_cell_info_list->ncell_info_list.ncell_info[num].rsrq,
                       p_nb_cell_info_list->ncell_info_list.ncell_info[num].pcid,
                       p_nb_cell_info_list->ncell_info_list.ncell_info[num].sinr,
                       p_nb_cell_info_list->ncell_info_list.ncell_info[num].srxlev,
                       p_nb_cell_info_list->ncell_info_list.ncell_info[num].rssi);
            }
            break;
        }

        case TS_EVENT_RADIO_SET_IMEI_RESP: {
            ts_radio_set_imei_resp_t *p_set_imei = NULL;

            p_set_imei = (ts_radio_set_imei_resp_t *)p_param;
            alog_info("[%s]ret_value = %d.", __func__, p_set_imei->ret_value);
            break;
        }

        case TS_EVENT_RADIO_START_SEARCH_RESP: {
            uint32_t index = 0;
            ts_radio_start_search_resp_t *p_resp = NULL;
            ts_radio_operator_info_t *p_operator_info = NULL;

            p_resp = (ts_radio_start_search_resp_t *)p_param;
            alog_info("[%s]Receive TS_EVENT_RADIO_START_SEARCH_RESP.", __func__);
            alog_info("[%s]event_id = 0x%x, ret_value = %d, operator_num = %d.",
                   __func__,
                   p_resp->event_id,
                   p_resp->ret_value,
                   p_resp->operator_num);
            for (index = 0; index < p_resp->operator_num; index++) {
                p_operator_info = (ts_radio_operator_info_t *)(&(p_resp->data_header)) + index;
                alog_info("[%s][%d]operator_short_name = %s, operator_long_name = %s, plmn_mcc = %s, plmn_mnc = %s, operator_id = %d, network_type = %d, status = %d.",
                       __func__,
                       index,
                       p_operator_info->operator_short_name,
                       p_operator_info->operator_long_name,
                       p_operator_info->plmn_mcc,
                       p_operator_info->plmn_mnc,
                       p_operator_info->operator_id,
                       p_operator_info->network_type,
                       p_operator_info->status);

            }
            break;
        }

        default:
            break;
    }
    return 0;
}
static int32_t handleCallCallback(void *p_param, uint32_t size)
{
    ts_event_call_t *p_call_event = NULL;
    //int ret_audio = 0;

    if(NULL == p_param) {
        alog_error("handleCallCallback,p_param is null");
        return -1;
    }

    p_call_event = (ts_event_call_t *)p_param;

    switch (p_call_event->event_id) {
        case TS_EVENT_CALL_STATUS_UPDATED: {
            ts_call_status_updated_t *p_call_status = {0};
            ts_call_info_list_t call_info_list = {0};

            p_call_status = (ts_call_status_updated_t *)p_param;
            //alog_info("handleCallCallback,p_call_status status = %d",  p_call_status->call_status);
            aic_srv_tele_call_get_call_info_list(0, &call_info_list);
            memcpy(&s_call_info_list, &call_info_list, sizeof(ts_call_info_list_t));

            switch(p_call_status->call_status) {
                case TS_CALL_STATUS_ACTIVE:/* remote accept call,setup call success,display connected call win(include,phone number and call duration) */
                    /* 1.stop play local ring */

                    /* 2.stop play local ringback */

                    /* 3.start voice */
                    aic_call_voice_start();

                    /* 4.enter connected win and display call time*/
                    break;

                case TS_CALL_STATUS_INCOMING: {/* incoming a call,phone recieve a call */
                    alog_info("handleCallCallback,call_active: = %d call_number = %d", p_call_status->call_status,call_info_list.call_number);
                    if (1 < call_info_list.call_number) {
                        int ret = -1;
                        uint8_t cur_card_id = TS_CARD_ID_MAX;
                        aic_srv_tele_sim_get_current_card(&cur_card_id);
                        ret = aic_srv_tele_call_release(cur_card_id, p_call_status->call_idx, NULL);
                        alog_info("handleCallCallback,aic_srv_tele_call_release return,ret = %d.\r\n", ret);
                        return -1;
                    } else {
                        /* play local ring */
                    }

                    /* 2. to ui task,display incoming call win */
                    break;
                }

                case TS_CALL_STATUS_RELEASED: {/*hung up call,user hungup and remote hungup,this message is coming */
                    /* if there is other call index, do not need to stop voice */
                    alog_info("handleCallCallback,TS_CALL_STATUS_RELEASED call_number = %d.\r\n", call_info_list.call_number);
                    if (0 == call_info_list.call_number) {
                        /* 1. stop voice */
                        aic_call_voice_stop();

                    } else {
                        alog_info("handleCallCallback,Release the second call, do nothing and return only.\r\n");
                        return -1;
                    }

                    /* 2.release call��
                       ui task:close call win */
                    break;
                }

                default:
                    break;
            }
            break;
        }
        case TS_EVENT_CALL_VOLTE_REG_STATUS_UPDATED:/* ims register state*/
        {
            ts_call_volte_reg_status_updated_t *p_volte_status = NULL;
            alog_info("handleCallCallback,TS_EVENT_CALL_VOLTE_REG_STATUS_UPDATED");
            p_volte_status = (ts_call_volte_reg_status_updated_t *)p_param;

            alog_info("handleCallCallback,volte status = %d", p_volte_status->volte_reg_status);
            ui_message_send_async(SIM_CALL_SET_VIEW, MSG_VIEW_PAINT, 0);
            break;
        }

        case TS_EVENT_CALL_RINGBACK_VOICE: {/* play tone,maybe customed ringback by operator from network,maybe play local ringback settinged by user*/
            ts_call_ringback_voice_t *p_ringback_voice = NULL;

            p_ringback_voice = (ts_call_ringback_voice_t *)p_param;
            /* voice_type: 2-network ringback voice, 0-local ringback voice */
            /* voice_state: 1-start play, 0-stop play */
            alog_info("handleCallCallback,TS_EVENT_CALL_RINGBACK_VOICE arrived(voice_type = %d, voice_state = %d).\r\n",
                     p_ringback_voice->voice_type,
                     p_ringback_voice->voice_state);

            if (1 == p_ringback_voice->voice_state) {/* if voice_state is 1, start play ringback voice */
                /* TODO: open audio codec and start transform audio data to play ringback voice */
                aic_call_voice_start();

            } else {/* if voice_state is 0, no need to do things currently */
                alog_info("handleCallCallback,currently no need to do things for stop play ringback voice.\r\n");
            }
            break;
        }

        case TS_EVENT_CALL_SEND_DTMF_RESP: {
            ts_call_send_dtmf_resp_t *p_send_dtmf_resp = NULL;

            p_send_dtmf_resp = (ts_call_send_dtmf_resp_t *)p_param;
            alog_info("[%s][Info]p_send_dtmf_resp->ret_value = %d.", __func__, p_send_dtmf_resp->ret_value);
            break;
        }

        default:
            break;
    }
    return 0;
}

static void _sim_tele_btn_evt_handler(lv_event_t *e)
{
	int32_t btn_id = (int32_t)lv_obj_get_user_data(lv_event_get_current_target(e));
    switch (btn_id)
    {
    case SIM_CARD_INFORMATION:
        view_stack_push_view(SIM_CARD_INFORMATION_VIEW, NULL);
        break;
    case SIM_SIGNAL_STATE:
        view_stack_push_view(SIM_SIGNAL_STATE_VIEW, NULL);
        break;
    case SIM_HOUSING_INFORMATION:
        view_stack_push_view(SIM_HOUSING_INFORMATION_VIEW, NULL);
        break;
    case SIM_RADIO_SET:
    {
        ts_radio_reg_status_e p_reg_status = TS_RADIO_REG_STATUS_NO_SERVICE;
        static int32_t radio_handle = 0;
        static int32_t call_handle = 0;
        sim_data.get_reg_status(0,&p_reg_status);
        if(p_reg_status == TS_RADIO_REG_STATUS_IN_SERVICE) {
            sim_data.radio_on(0, false, NULL);
        } else {
            sim_data.radio_on(0, true, NULL);
            if(radio_handle == 0)
                radio_handle = sim_data.radio_register(NULL, handleRadioCallback);
            if(call_handle == 0)
                call_handle = sim_data.call_register(NULL, handleCallCallback);
        }
        break;
    }
    case SIM_NETWORK_STATE:
        view_stack_push_view(SIM_NETWORK_STATE_VIEW, NULL);
        break;
    case SIM_CALL_SET:
        view_stack_push_view(SIM_CALL_SET_VIEW, NULL);
        break;
    case SIM_CARD_SET:
        view_stack_push_view(SIM_SMS_SET_VIEW, NULL);
        break;
    default:
        break;
    }
}

static int _ui_sim_tele_layout(view_data_t *view_data)
{
	lv_obj_t *scr = lv_display_get_screen_active(view_data->display);
	sim_tele_view_data *data = app_mem_malloc(sizeof(*data));
	if (!data) {
		return -ENOMEM;
	}
	memset(data, 0, sizeof(*data));

	view_data->user_data = data;

    LVGL_FONT_OPEN_DEFAULT(&data->font, 28);

    lv_coord_t layout_space = 110;
    lv_coord_t text_num = sizeof(sim_tele_text)/sizeof(sim_tele_text[0]);
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
		lv_obj_add_event_cb(obj_icon, _sim_tele_btn_evt_handler, LV_EVENT_SHORT_CLICKED, data);
		lv_obj_set_user_data(obj_icon,(void *)i);

        lv_obj_t *text_icon = text_canvas_create(obj_icon);
        lv_obj_align(text_icon,LV_ALIGN_CENTER,0,0);
        lv_obj_set_width(text_icon,420);
        lv_obj_set_style_text_align(text_icon,LV_TEXT_ALIGN_CENTER,LV_PART_MAIN);
		lv_obj_set_style_text_color(text_icon,lv_color_white(),LV_PART_MAIN);
		lv_obj_set_style_text_font(text_icon,&data->font,LV_PART_MAIN);
        text_canvas_set_long_mode(text_icon,TEXT_CANVAS_LONG_SCROLL_CIRCULAR);
        text_canvas_set_text(text_icon,sim_tele_text[i]);
        lv_obj_add_flag(text_icon,LV_OBJ_FLAG_EVENT_BUBBLE);
    }

    lv_obj_t *obj_icon = lv_obj_create(scr);
	lv_obj_set_pos(obj_icon,0,60 + layout_space * text_num);
	lv_obj_set_size(obj_icon,4,4);
    return 0;
}

static int _ui_sim_tele_delete(view_data_t *view_data)
{
    lv_obj_t *scr = lv_display_get_screen_active(view_data->display);
    lv_obj_clean(scr);
    sim_tele_view_data *data = view_data->user_data;
	if (data) {
        LVGL_FONT_CLOSE(&data->font);
        app_mem_free(data);
    }
    view_data->user_data = NULL;
    return 0;
}

int _ui_sim_tele_handler(uint16_t view_id, view_data_t *view_data, uint8_t msg_id, void * msg_data)
{
    int ret = 0;
	switch (msg_id) {
	case MSG_VIEW_PRELOAD:
        ret = ui_view_layout(SIM_TELE_VIEW);
        break;
	case MSG_VIEW_LAYOUT:
        ret = _ui_sim_tele_layout(view_data);
        break;
	case MSG_VIEW_DELETE:
        ret = _ui_sim_tele_delete(view_data);
        break;
	default:
        break;
	}
	return ret;
}

VIEW_DEFINE2(ui_sim_tele, _ui_sim_tele_handler, NULL, \
		NULL, SIM_TELE_VIEW, NORMAL_ORDER, UI_VIEW_LVGL, DEF_UI_VIEW_WIDTH, DEF_UI_VIEW_HEIGHT);
#endif
