#ifndef _HX3690L_CHECK_TOUCH_H_
#define _HX3690L_CHECK_TOUCH_H_

#include <stdint.h>
#include <stdbool.h>
#include "hx3690l.h"


hx3690_wear_msg_code_t  check_touch_alg(int32_t ir_data);
SENSOR_ERROR_T hx3690l_check_touch_enable(void);
uint8_t hx3690l_check_touch_read(ppg_sensor_data_t *s_dat);
hx3690_wear_msg_code_t hx3690l_check_touch_send_data(int32_t *ir_data, uint8_t count);
hx3690_wear_msg_code_t check_touch_alg(int32_t ir_data);

#endif
