#ifndef LAUNCHER_UI_CODER_H_
#define LAUNCHER_UI_CODER_H_

#ifdef __cplusplus
extern "C" {
#endif
#include <lvgl/lvgl.h>

enum {
	UI_CODER_BEGIN = 0,
    UI_CODER_ING,
	UI_CODER_END,
};

enum {
	CODER_DIRECTION_NULL = 0,
    CODER_DIRECTION_FORWARD,
	CODER_DIRECTION_INVERSION,
};

typedef void (*_coder_cb_)(void *trigger_data);
typedef struct {
    lv_obj_t *obj;
    void *user_data;
    _coder_cb_ cb;
    lv_point_t point;
    uint16_t view_id;
    uint16_t tp_value;
    uint8_t dir;
}ui_coder_trigger_t;

typedef struct {
    uint8_t coder_state;
    int8_t direction;
}ui_coder_data_t;

typedef struct {
    lv_point_t point;
    uint16_t tp_value;
    int8_t direction;
    uint8_t state_ing;
    uint8_t dir;
}coder_to_tp_t;

ui_coder_trigger_t *coder_simulation_tp_register(lv_obj_t *obj, uint16_t view_id, uint16_t dir, uint16_t tp_value, lv_point_t *point);

ui_coder_trigger_t *coder_private_event_register(lv_obj_t *obj, uint16_t view_id , _coder_cb_ cd , void *user_data);

void _coder_roll_execute(bool direction);

ui_coder_data_t *_get_coder_data(void);

void *get_coder_user_data(void *trigger_data);

#ifdef __cplusplus
}
#endif
#endif
