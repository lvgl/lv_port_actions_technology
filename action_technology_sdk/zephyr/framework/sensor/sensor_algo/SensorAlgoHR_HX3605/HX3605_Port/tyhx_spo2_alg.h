#ifndef _TYHX_SPO2_ALG_H_
#define _TYHX_SPO2_ALG_H_

#include <stdint.h>




//#define SPO2_CALI_EN

#ifdef SPO2_ALG_LIB

typedef enum {
	MSG_SPO2_ALG_NOT_OPEN,
	MSG_SPO2_ALG_OPEN,
	MSG_SPO2_ALG_OPEN_DEEP,
	MSG_SPO2_READY,
	MSG_SPO2_ALG_TIMEOUT,
	MSG_SPO2_SETTLE
} spo2_alg_msg_code_t;

typedef struct {
  spo2_alg_msg_code_t  spo2_alg_status;
  uint32_t                 data_cnt;
  uint8_t                 spo2_result;
  uint8_t                  hr_result;
  uint8_t                  block_cnt;
	uint8_t                  ir_quality;
	uint16_t                 reliable_score_result;
} tyhx_spo2_results_t;

typedef struct {
  uint8_t                 red_offset_idac;
  uint8_t                 ir_offset_idac; 
  uint8_t                 green_offset_idac; 
  uint16_t                spo2_start_cnt;
} tyhx_spo2_agcpara_t;


typedef struct {
	uint8_t XCORR_MODE;   //ericy 20180619 
	uint8_t QUICK_RESULT;
	uint8_t   G_SENSOR; 

} spo2_switch_t;

typedef struct {
	uint8_t SPO2_LOW_XCORR_THRE;
	uint8_t SPO2_LOW_SNR_THRE;
	uint8_t COUNT_BLOCK_NUM;
	//uint8_t SPO2_GSEN_POW_THRE;
	uint32_t SPO2_BASE_LINE_INIT;
	uint32_t SPO2_SLOPE;
	uint8_t   SPO2_GSEN_POW_THRE;
} spo2_usuallyadjust_t;

typedef struct {
	uint16_t MEAN_NUM;
	int32_t  SOP2_DEGLITCH_THRE;
	int32_t  SPO2_REMOVE_JUMP_THRE;
	uint16_t SPO2_LOW_CLIP_END_TIME;
	uint16_t SPO2_LOW_CLIP_DN;
	uint16_t SPO2_NORMAL_CLIP_DN;
	uint16_t IR_AC_TOUCH_THRE;
	uint16_t IR_FFT_POW_THRE;
	uint8_t  SPO2_CALI;
	uint8_t  SLOPE_PARA_MAX;
	uint8_t  SLOPE_PARA_MIN;
} spo2_barelychange_t;
//void tyhx_spo2_alg_send_para(spo2_switch_t spo2_switch_set,spo2_usuallyadjust_t spo2_usuallyadjust_set,spo2_barelychange_t spo2_barelychange_set);
void tyhx_spo2_switch(uint8_t XCORR_MODE,uint8_t QUICK_RESULT,uint8_t G_SENSOR);
void tyhx_spo2_para_usuallyadjust(uint8_t SPO2_LOW_XCORR_THRE,uint8_t SPO2_LOW_SNR_THRE,uint8_t COUNT_BLOCK_NUM,uint32_t SPO2_BASE_LINE_INIT,uint32_t SPO2_SLOPE,uint8_t SPO2_GSEN_POW_THRE);
void tyhx_spo2_para_barelychange(uint16_t MEAN_NUM,int32_t SOP2_DEGLITCH_THRE,int32_t SPO2_REMOVE_JUMP_THRE, \
                                   uint16_t SPO2_LOW_CLIP_END_TIME,uint16_t SPO2_LOW_CLIP_DN,uint16_t SPO2_NORMAL_CLIP_DN, \
                                   uint16_t IR_AC_TOUCH_THRE,uint16_t IR_FFT_POW_THRE,uint8_t SPO2_CALI,uint8_t SLOPE_PARA_MAX,uint8_t SLOPE_PARA_MIN);

bool tyhx_spo2_alg_send_data(int32_t *red_new_raw_data,int32_t *ir_new_raw_data,uint8_t R_LEDDAC,uint8_t IR_LEDDAC, uint8_t dat_len,volatile int16_t *gsen_data_x,volatile int16_t *gsen_data_y,volatile int16_t *gsen_data_z);
bool tyhx_spo2_alg(int32_t red_new_raw_data , int32_t ir_new_raw_data,uint8_t R_LEDDAC,uint8_t IR_LEDDAC,int16_t gsen_data_x, int16_t gsen_data_y, int16_t gsen_data_z);
void tyhx_spo2_alg_close(void);
bool tyhx_spo2_alg_open(void);
bool tyhx_spo2_alg_open_deep(void);
tyhx_spo2_results_t tyhx_spo2_alg_get_results(void);
#endif




#endif 




