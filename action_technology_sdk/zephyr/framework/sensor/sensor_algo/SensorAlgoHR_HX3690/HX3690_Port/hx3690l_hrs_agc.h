#ifndef _hx3690l_agc_H_
#define _hx3690l_agc_H_
#include <stdint.h>
#include <stdbool.h>
#include "hx3690l.h"

#define CAL_FLG_LED_DR     0x01
#define CAL_FLG_LED_DAC    0x02 
#define CAL_FLG_AMB_DAC    0x04
#define CAL_FLG_RF         0x08

#define HRS_CAL_INIT_LED   128
#define NO_TOUCH_CHECK_NUM 2 
#define CAL_DELAY_COUNT    2

typedef enum {
	MSG_HRS_NO_WEAR,
	MSG_HRS_WEAR
} hrs_wear_msg_code_t;

typedef struct {
	uint8_t count;
	int32_t ppg_data[32];
	int32_t ir_data;
	int32_t s_buf[64]; 
	uint32_t als;
	uint32_t agc_green;
} hrs_sensor_data_t;

typedef enum {    
    hrsCalStart, 
    hrsCalLedCur,
    hrsCalLed,
    hrsCalLed2,
    hrsCalLed3,
    hrsCalFinish, 
} HRS_STATE_T;

typedef struct {
	uint8_t flag;
	bool work;
	uint8_t int_cnt;
	uint8_t cur255_cnt;
	uint8_t led_idac; 
	uint16_t LED; 
	uint8_t LEDDAC; 
	uint8_t AMBDAC;  
	uint8_t RF; 
	uint32_t led_step;
	uint8_t state;
	uint8_t led_max_cur;
} HRS_CAL_SET_T;

HRS_CAL_SET_T PPG_hrs_agc(void);
void Init_hrs_PPG_Calibration_Routine(HRS_CAL_SET_T *calR);
void Restart_hrs_PPG_Calibration_Routine(HRS_CAL_SET_T *calR);
void PPG_hrs_Calibration_Routine(HRS_CAL_SET_T *calR, int32_t led, int32_t amb);
void hx3690l_hrs_cal_init(void);
void hx3690l_hrs_cal_off(void);
uint8_t hx3690l_read_fifo_size(void);
void read_hrs_data_packet(int32_t *buf);
void read_hrs_ir_packet(int32_t *buf);
void hx3690l_hrs_read_data_packet(int32_t *buf); 
void hx3690l_hrs_low_power(void);
void hx3690l_hrs_normal_power(void);
void hx3690l_hrs_updata_reg(void);
void hx3690l_hrs_set_mode(uint8_t mode_cmd);
SENSOR_ERROR_T hx3690l_hrs_enable(void);
void hx3690l_hrs_disable(void);
hrs_wear_msg_code_t hx3690_hrs_get_wear_status(void);
uint8_t hx3690l_hrs_read(ppg_sensor_data_t *s_dat);
bool hx3690l_hrs_init(void);      
hrs_wear_msg_code_t hx3690_hrs_wear_mode_check(int32_t infrared_data);
HRS_CAL_SET_T get_hrs_agc_status(void);
void hx3690l_hrs_data_reset(void);

#endif
