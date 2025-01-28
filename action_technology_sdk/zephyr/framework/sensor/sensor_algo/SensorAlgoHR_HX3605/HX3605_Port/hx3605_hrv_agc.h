#ifndef _HX3605_HRV_AGC_H_
#define _HX3605_HRV_AGC_H_
#include <stdint.h>
#include <stdbool.h>
#include "hx3605.h"

#define CAL_DELAY_COUNT (3)

#define CAL_FLG_LED_DR     0x01
#define CAL_FLG_LED_DAC    0x02 
#define CAL_FLG_AMB_DAC    0x04
#define CAL_FLG_RF         0x08

#define HRV_CAL_INIT_LED 16
#define NO_TOUCH_CHECK_NUM 2 


typedef enum {    
    hrvCalStart, 
		hrvLedCur,
    hrvCalLed,
    hrvCalLed2,
    hrvCalFinish, 
}HRV_STATE_T;

typedef struct {
	uint8_t count;
	int32_t ppg_data[32];
	int32_t ir_data[32];
	int32_t s_buf[32]; 
	uint32_t als;
	uint32_t agc_green;
}hrv_sensor_data_t;

typedef enum {
	MSG_HRV_NO_WEAR,
	MSG_HRV_WEAR,
	MSG_HRV_INIT
} hx3605_hrv_wear_msg_code_t;

typedef struct {
	uint8_t flag;
	bool work;
	uint8_t int_cnt;
	uint8_t cur255_cnt;
	uint8_t led_cur;
	uint8_t led_idac; 
	uint8_t amb_idac;
	uint8_t ontime;
	uint16_t led_step;
	uint8_t state;
	uint8_t led_max_cur;
} HRV_CAL_SET_T;

void Init_hrv_PPG_Calibration_Routine(HRV_CAL_SET_T *calR,uint8_t led);
void Restart_hrv_PPG_Calibration_Routine(HRV_CAL_SET_T *calR);
void PPG_hrv_Calibration_Routine(HRV_CAL_SET_T *calR, int32_t led, int32_t amb);
HRV_CAL_SET_T PPG_hrv_agc(void);

void hx3605_hrv_cal_init(void);
void hx3605_hrv_cal_off(void);
uint8_t hx3605_read_fifo_size(void);
void read_hrv_data_packet(int32_t *buf);
void read_hrv_ir_packet(int32_t *buf);

void hx3605_hrv_low_power(void);
void hx3605_hrv_normal_power(void);
void hx3605_hrv_updata_reg(void);
void hx3605_hrv_set_mode(uint8_t mode_cmd);
SENSOR_ERROR_T hx3605_hrv_enable(void);
void hx3605_hrv_disable(void);
hx3605_hrv_wear_msg_code_t hx3605_hrv_get_wear_status(void);
uint8_t hx3605_hrv_read(hrv_sensor_data_t * s_dat);
bool hx3605_hrv_init(void);

hx3605_hrv_wear_msg_code_t hx3605_hrv_wear_mode_check(int32_t infrared_data);
HRV_CAL_SET_T get_hrv_agc_status(void);
void hx3605_hrv_data_reset(void);

#endif
