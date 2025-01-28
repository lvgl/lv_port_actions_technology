#ifndef _HX3690L_SPO2_AGC_H_
#define _HX3690L_SPO2_AGC_H_
#include <stdint.h>
#include <stdbool.h>
#include "hx3690l.h"

#define CAL_FLG_LED_DR     0x01
#define CAL_FLG_LED_DAC    0x02 
#define CAL_FLG_AMB_DAC    0x04
#define CAL_FLG_RF         0x08

#define SPO2_CAL_INIT_LED_RED 128
#define SPO2_CAL_INIT_LED_IR 128
#define SPO2_CAL_INIT_LED_GREEN 128

#define SPO2_CAL_DELAY_COUNT    8
#define SPO2_NO_TOUCH_CHECK_NUM 2


typedef enum {    
    sCalStart,
	sCalLedCur,	
    sCalLed,
    sCalLed2,
    sCalLed3,
    sCalFinish, 
} SPO2_STATE_T;

typedef struct {
	uint8_t count;
	int32_t red_data[16];
	int32_t green_data[16];
	int32_t ir_data[16];  
	int32_t s_buf[64];
	uint8_t agc_green;
	uint8_t agc_red; 
	uint8_t agc_ir;    
}spo2_sensor_data_t;

typedef enum {
	MSG_SPO2_NO_WEAR,
	MSG_SPO2_WEAR
} spo2_wear_msg_code_t;

typedef struct {
	uint8_t flag;
	bool work;
	uint8_t int_cnt;
	uint8_t cur255_cnt;
	uint8_t green_idac;
	uint8_t red_idac;
	uint8_t ir_idac;
	uint16_t G_LED; 
	uint16_t R_LED;     
	uint16_t IR_LED;     
	uint8_t G_LEDDAC;
	uint8_t R_LEDDAC;  
	uint8_t IR_LEDDAC;  
	uint8_t AMBDAC;  
	uint8_t G_RF;
	uint8_t R_RF; 
	uint8_t IR_RF;
	uint32_t G_led_step;
	uint32_t R_led_step;
	uint32_t IR_led_step;
	uint8_t state;
	uint8_t green_max_cur;
	uint8_t red_max_cur;
	uint8_t ir_max_cur;
} SPO2_CAL_SET_T;

void Init_Spo2_PPG_Calibration_Routine(SPO2_CAL_SET_T *calR,uint8_t led);  
void Restart_spo2_PPG_Calibration_Routine(SPO2_CAL_SET_T *calR);
void PPG_Spo2_Calibration_Routine(SPO2_CAL_SET_T *calR, int32_t g_led, int32_t r_led, int32_t amb, int32_t ir_led);

void hx3690l_spo2_cal_init(void);
void hx3690l_spo2_cal_off(void); 
void hx3690l_spo2_read_data(int32_t *buf);
void hx3690l_spo2_low_power(void);
void hx3690l_spo2_normal_power(void); 
void hx3690l_spo2_updata_reg(void);
void hx3690l_spo2_set_mode(uint8_t mode_cmd);
SENSOR_ERROR_T hx3690l_spo2_enable(void);
void hx3690l_spo2_disable(void);
spo2_wear_msg_code_t hx3690_spo2_get_wear_status(void);
uint8_t hx3690l_spo2_read(ppg_sensor_data_t * s_dat);
bool hx3690_spo2_check_unwear(int32_t infrared_data);
bool hx3690_spo2_check_wear(int32_t infrared_data);
SPO2_CAL_SET_T get_spo2_agc_status(void);
SPO2_CAL_SET_T PPG_spo2_agc(void);
void hx3690l_spo2_data_reset(void);

#endif
