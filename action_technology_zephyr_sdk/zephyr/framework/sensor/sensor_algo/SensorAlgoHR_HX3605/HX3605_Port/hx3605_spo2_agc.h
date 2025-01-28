#ifndef _HX3605_SPO2_AGC_H_
#define _HX3605_SPO2_AGC_H_
#include <stdint.h>
#include <stdbool.h>
#include "hx3605.h"

#define CAL_DELAY_COUNT (3)

#define CAL_FLG_LED_DR     0x01
#define CAL_FLG_LED_DAC    0x02 
#define CAL_FLG_AMB_DAC    0x04
#define CAL_FLG_RF         0x08

#define SPO2_CAL_INIT_LED_RED 16
#define SPO2_CAL_INIT_LED_IR 16

//touch check 
#define SPO2_NO_TOUCH_CHECK_NUM 2     //1 second one times



typedef enum {    
    sCalStart,
		sCalLedCur,	
    sCalLed,
    sCalLed2,
    sCalFinish, 
} SPO2_STATE_T;

typedef struct {
	uint8_t count;
	int32_t red_data[16];
	int32_t ir_data[16];  
	int32_t s_buf[32];
	uint8_t agc_red; 
	uint8_t agc_ir;    
}spo2_sensor_data_t;

typedef enum {
	MSG_SPO2_NO_WEAR,
	MSG_SPO2_WEAR,
	MSG_SPO2_INIT
} hx3605_spo2_wear_msg_code_t;

typedef struct {
  uint8_t                 red_offset_idac;
  uint8_t                 ir_offset_idac; 
  uint8_t                 green_offset_idac; 
  uint16_t                spo2_start_cnt;
} hx3605_spo2_agcpara_t;

typedef struct {
	uint8_t flag;
	bool work;
	uint8_t int_cnt;
	uint8_t cur255_cnt;
	uint8_t green_idac;
	uint8_t red_idac;
	uint8_t ir_idac;
	uint8_t amb_idac;
	uint16_t green_cur; 
	uint16_t red_cur;     
	uint16_t ir_cur;       
	uint8_t ontime;
	uint32_t green_led_step;
	uint32_t red_led_step;
	uint32_t ir_led_step;
	uint8_t state;
	uint8_t green_max_cur;
	uint8_t red_max_cur;
	uint8_t ir_max_cur;
} SPO2_CAL_SET_T;


void Init_Spo2_PPG_Calibration_Routine(SPO2_CAL_SET_T *calR);  
void Restart_spo2_PPG_Calibration_Routine(SPO2_CAL_SET_T *calR);
void PPG_Spo2_Calibration_Routine(SPO2_CAL_SET_T *calR, int32_t r_led, int32_t amb, int32_t ir_led);
SPO2_CAL_SET_T PPG_spo2_agc(void);

void hx3605_spo2_cal_init(void);
void hx3605_spo2_cal_off(void);

void read_spo2_data_packet(int32_t *buf);
void read_spo2_ir_packet(int32_t *buf);
void hx3605_spo2_read_data(int32_t *buf);

void hx3605_spo2_low_power(void);
void hx3605_spo2_normal_power(void);

void hx3605_spo2_updata_reg(void);
void hx3605_spo2_set_mode(uint8_t mode_cmd);
SENSOR_ERROR_T hx3605_spo2_enable(void);
void hx3605_spo2_disable(void);
hx3605_spo2_wear_msg_code_t hx3605_spo2_get_wear_status(void);
uint8_t hx3605_spo2_read(spo2_sensor_data_t * s_dat);
uint8_t hx3605_spo2_read_data_packet(int32_t *s_buf);
bool hx3605_spo2_check_unwear(void);
bool hx3605_spo2_check_wear(int32_t infrared_data);
SPO2_CAL_SET_T get_spo2_agc_status(void);
void hx3605_spo2_data_reset(void);

#endif
