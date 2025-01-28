#include "ui_coder.h"
#include <app_ui.h>
#include <view_stack.h>
#ifndef CONFIG_SIMULATOR
#include <drivers/input/input_dev.h>
#include <input_manager.h>
#endif

#ifdef CONFIG_INPUT_DEV_ACTS_KNOB
#define CODER_END_TIME 300
#define CODER_COMMON_ROLL_X 50
#define CODER_COMMON_ROLL_Y 50

static ui_coder_data_t coder_data = {0};
static coder_to_tp_t coder_tp = {0};

typedef struct {
    ui_coder_trigger_t *trigger;
    lv_timer_t *timer;
    uint8_t trigger_num;
}ui_coder_manage_t;

static ui_coder_manage_t coder_manage = {0};

static bool input_filter_tp(void *input_val)
{
#if (defined(CONFIG_INPUT_DEV_ACTS_KNOB) && !defined(CONFIG_SIMULATOR))
    struct input_value *val = (struct input_value *)input_val;
    bool ret = false;
    int16_t scroll_value = 0;
    switch (coder_tp.state_ing)
    {
    case 0:
        if(val->point.pessure_value)
            coder_tp.direction = CODER_DIRECTION_NULL;
        else
        {
            if(coder_tp.direction != CODER_DIRECTION_NULL)
            {
                coder_tp.state_ing++;
                scroll_value = coder_tp.tp_value;
                val->point.pessure_value = true;
                ret = true;
            }
        }
        break;
    case 1:
        coder_tp.state_ing++;
        if(coder_tp.direction == CODER_DIRECTION_FORWARD)
            scroll_value = coder_tp.tp_value + coder_tp.tp_value/2;
        else if(coder_tp.direction == CODER_DIRECTION_INVERSION)
            scroll_value = coder_tp.tp_value - coder_tp.tp_value/2;
        val->point.pessure_value = true;
        ret = true;
        break;
    case 2:
        coder_tp.state_ing++;
        if(coder_tp.direction == CODER_DIRECTION_FORWARD)
            scroll_value = coder_tp.tp_value * 2;
        else if(coder_tp.direction == CODER_DIRECTION_INVERSION)
            scroll_value = 0;
        val->point.pessure_value = true;
        ret = true;
        break;
    case 3:
        coder_tp.state_ing = 0;
        if(coder_tp.direction == CODER_DIRECTION_FORWARD)
            scroll_value = coder_tp.tp_value * 2;
        else if(coder_tp.direction == CODER_DIRECTION_INVERSION)
            scroll_value = 0;
        coder_tp.direction = CODER_DIRECTION_NULL;
        val->point.pessure_value = false;
        ret = true;
        break;
    default:
        break;
    }
    if(ret)
    {
        if(coder_tp.dir & LV_DIR_HOR)
        {
            val->point.loc_x = coder_tp.point.x + scroll_value;
            val->point.loc_y = coder_tp.point.y;
        }
        else if(coder_tp.dir & LV_DIR_VER)
        {
            val->point.loc_x = coder_tp.point.x;
            val->point.loc_y = coder_tp.point.y + scroll_value;
        }
    }
    return ret;
#else
    return false;
#endif
}

static void _coder_to_tp_cb(void *trigger_data)
{
    if(coder_data.direction != CODER_DIRECTION_NULL && coder_tp.state_ing == 0)
    {
        ui_coder_trigger_t *trigger = (ui_coder_trigger_t *)trigger_data;
        coder_tp.direction = coder_data.direction;
        coder_tp.dir = trigger->dir;
        coder_tp.tp_value = trigger->tp_value;
        memcpy(&coder_tp.point, &trigger->point, sizeof(coder_tp.point));
    }
}

static int16_t _coder_traverse_list(lv_obj_t *obj)
{
    for(int16_t i = 0 ; i < coder_manage.trigger_num ; i++)
    {
        ui_coder_trigger_t *trigger = coder_manage.trigger + i;
        if(trigger->obj == obj)
            return i;
    }
    return -1;
}

static bool _coder_list_plus(void)
{
    coder_manage.trigger_num++;
    if(coder_manage.trigger)
    {
        ui_coder_trigger_t *trigger = lv_malloc(sizeof(ui_coder_trigger_t) * coder_manage.trigger_num);
        if(trigger == NULL)
            return false;
        lv_memcpy(trigger, coder_manage.trigger, sizeof(ui_coder_trigger_t) * (coder_manage.trigger_num - 1));
        lv_free(coder_manage.trigger);
        coder_manage.trigger = trigger;
    }
    else
    {
        coder_manage.trigger = lv_malloc(sizeof(ui_coder_trigger_t) * coder_manage.trigger_num);
        if(coder_manage.trigger == NULL)
            return false;
    }
    return true;
}

static void _coder_delete_event_handler(lv_event_t * e)
{
	lv_obj_t *obj = (lv_obj_t *)lv_event_get_user_data(e);
    int32_t trigger_num = _coder_traverse_list(obj);
    if(coder_manage.trigger_num && trigger_num >= 0)
    {
        coder_manage.trigger_num--;
        uint32_t one_size = sizeof(ui_coder_trigger_t);
        ui_coder_trigger_t *trigger = lv_malloc(one_size * coder_manage.trigger_num);
        if(trigger_num)
            lv_memcpy(trigger, coder_manage.trigger, one_size * trigger_num);
        if(trigger_num < coder_manage.trigger_num)
            lv_memcpy(trigger + trigger_num, coder_manage.trigger + trigger_num + 1, one_size * (coder_manage.trigger_num - trigger_num));
        lv_free(coder_manage.trigger);
        coder_manage.trigger = trigger;
    }
    else
    {
        if(coder_manage.trigger)
        {
            lv_free(coder_manage.trigger);
            coder_manage.trigger = NULL;
        }
    }
}

static ui_coder_trigger_t *_coder_add_trigger(lv_obj_t *obj, uint16_t view_id)
{
    if(obj == NULL)
        return NULL;
    int32_t trigger_num = _coder_traverse_list(obj);
    if(trigger_num < 0)
    {
        if(!_coder_list_plus())
            return NULL;
        trigger_num = coder_manage.trigger_num - 1;
        lv_obj_add_event_cb(obj, _coder_delete_event_handler, LV_EVENT_DELETE, (void *)obj);
    }
    ui_coder_trigger_t *trigger = coder_manage.trigger + trigger_num;
    trigger->obj = obj;
    trigger->view_id = view_id;
    return trigger;
}

static void _coder_cb_execute(void)
{
    for(uint8_t i = 0 ; i < coder_manage.trigger_num ; i++)
    {
        ui_coder_trigger_t *trigger = coder_manage.trigger + i;
        bool ret = false;
        if(view_stack_get_top() == trigger->view_id)
            ret = true;
        if(ret)
            trigger->cb((void *)trigger);
    }
}

static void _coder_time_cb(lv_timer_t *timer)
{
    lv_timer_delete(coder_manage.timer);
    coder_manage.timer = NULL;
    coder_data.direction = CODER_DIRECTION_NULL;
    coder_data.coder_state = UI_CODER_END;
    _coder_cb_execute();
}
#endif

ui_coder_trigger_t *coder_simulation_tp_register(lv_obj_t *obj, uint16_t view_id, uint16_t dir, uint16_t tp_value, lv_point_t *point)
{
#ifdef CONFIG_INPUT_DEV_ACTS_KNOB
    ui_coder_trigger_t *trigger = _coder_add_trigger(obj, view_id);
    if(trigger == NULL)
        return NULL;
    trigger->cb = _coder_to_tp_cb;
    trigger->dir = dir;
    trigger->tp_value = tp_value;
    if(point)
        memcpy(&trigger->point, point, sizeof(trigger->point));
    else
    {
        trigger->point.x = DEF_UI_VIEW_WIDTH/2;
        trigger->point.y = DEF_UI_VIEW_HEIGHT/2;
    }
    input_pointer_register_filter_cb(input_filter_tp);
    return trigger;
#else
    return NULL;
#endif
}

ui_coder_trigger_t *coder_private_event_register(lv_obj_t *obj, uint16_t view_id , _coder_cb_ cd , void *user_data)
{
#ifdef CONFIG_INPUT_DEV_ACTS_KNOB
    if(cd == NULL)
        return NULL;
    ui_coder_trigger_t *trigger = _coder_add_trigger(obj, view_id);
    if(trigger == NULL)
        return NULL;
    trigger->user_data = user_data;
    trigger->cb = cd;
    return trigger;
#else
    return NULL;
#endif
}

void _coder_roll_execute(bool direction)
{
#ifdef CONFIG_INPUT_DEV_ACTS_KNOB
    coder_data.direction = direction ? CODER_DIRECTION_FORWARD : CODER_DIRECTION_INVERSION;
    if(coder_manage.timer)
    {
        coder_data.coder_state = UI_CODER_ING;
        lv_timer_reset(coder_manage.timer);
    }
    else
    {
        coder_data.coder_state = UI_CODER_BEGIN;
        coder_manage.timer = lv_timer_create(_coder_time_cb, CODER_END_TIME, NULL);
    }
    _coder_cb_execute();
#endif
}

ui_coder_data_t *_get_coder_data(void)
{
#ifdef CONFIG_INPUT_DEV_ACTS_KNOB
    return &coder_data;
#else
    return NULL;
#endif
}

void *get_coder_user_data(void *trigger_data)
{
    ui_coder_trigger_t *trigger = (ui_coder_trigger_t *)trigger_data;
    return trigger->user_data;
}
