#ifndef _SPO2_ALG_H_
#define _SPO2_ALG_H_
#endif
#include <stdint.h>

//version:2.3.18

//#include "tyhx_spo2_alg_para.h"

#if 0
/* portable 8-bit unsigned integer */
typedef unsigned char           uint8_t;
/* portable 8-bit signed integer */
typedef signed char             int8_t;
/* portable 16-bit unsigned integer */
typedef unsigned short int      uint16_t;
/* portable 16-bit signed integer */
typedef signed short int        int16_t;
/* portable 32-bit unsigned integer */
typedef unsigned int            uint32_t;
/* portable 32-bit signed integer */
typedef signed int              int32_t;
#endif

#ifndef bool
#define bool unsigned char
#endif

#ifndef  true
#define  true  1
#endif
#ifndef  false
#define  false  0
#endif


#ifdef SPO2_ALG_LIB

typedef enum {
	MSG_SPO2_ALG_NOT_OPEN,
	MSG_SPO2_ALG_OPEN,
	MSG_SPO2_READY,
	MSG_SPO2_ALG_TIMEOUT,
} spo2_alg_msg_code_t;

typedef struct {
    spo2_alg_msg_code_t		spo2_alg_status;
    uint32_t		data_cnt;
    uint8_t			spo2_result;
    uint8_t			hr_result;
    uint8_t			block_cnt;
    uint8_t			ir_quality;
    uint16_t		reliable_score_result;

    uint8_t			screen_up;
    uint8_t			motion;
    uint8_t			poor_signal;
} spo2_results_t;

typedef struct {
    uint8_t			screen_up;
    uint8_t			motion;
    uint8_t			poor_signal;
} spo2_gesture_recognize_t;

typedef struct {
	uint32_t data_cnt;
	int32_t red_new_raw_data;
	int32_t ir_new_raw_data;
	int32_t green_new_raw_data;
	int32_t ir_iir_data_filter_out;
	int32_t ir_data_smooth;
	int32_t red_iir_data_filter_out;
	int32_t red_data_smooth;
	uint8_t ir_offset_50;
	uint8_t red_offset_50;
	uint8_t green_offset_50;
	int32_t ir_dc;
	int32_t red_dc;
	int32_t green_dc;
	int32_t ir_data_o2_128;
	int32_t red_data_o2_128;
	int32_t green_data_o2_128;
	int16_t red_ir_dc;
	int16_t red_green_dc;
	int16_t ir_green_dc;
	int16_t ir_fft_snr;
	int8_t  motion_status;
	uint8_t xcorr_max;
	int16_t COUNT_IN_judge;
	uint8_t spo2_result;
	uint8_t count_block;
	int32_t SaO2_data_temp;
} spo2_debug_t;

typedef struct {
	  uint8_t	red_offset_idac;
	  uint8_t	ir_offset_idac; 
	  uint8_t	green_offset_idac; 
	  uint16_t	spo2_start_cnt;
} spo2_agcpara_t;

typedef struct {
	uint8_t XCORR_MODE;
	uint8_t QUICK_RESULT;
} spo2_switch_t;

typedef struct {
	uint8_t  SPO2_LOW_XCORR_THRE;
	uint8_t  SPO2_LOW_SNR_THRE;
	uint8_t  COUNT_BLOCK_NUM;
	uint32_t SPO2_BASE_LINE_INIT;
	uint32_t SPO2_SLOPE;
    uint16_t SPO2_GSEN_POW_THRE;
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

void tyhx_spo2_set_switch(uint8_t XCORR_MODE,uint8_t QUICK_RESULT);
void tyhx_spo2_para_usuallyadjust(uint8_t SPO2_LOW_XCORR_THRE,uint8_t SPO2_LOW_SNR_THRE,uint8_t COUNT_BLOCK_NUM,uint32_t SPO2_BASE_LINE_INIT,uint32_t SPO2_SLOPE,uint16_t SPO2_GSEN_POW_THRE);
void tyhx_spo2_para_barelychange(uint16_t MEAN_NUM,int32_t SOP2_DEGLITCH_THRE,int32_t SPO2_REMOVE_JUMP_THRE, \
                                   uint16_t SPO2_LOW_CLIP_END_TIME,uint16_t SPO2_LOW_CLIP_DN,uint16_t SPO2_NORMAL_CLIP_DN, \
                                   uint16_t IR_AC_TOUCH_THRE,uint16_t IR_FFT_POW_THRE,uint8_t SPO2_CALI,uint8_t SLOPE_PARA_MAX,uint8_t SLOPE_PARA_MIN);

bool tyhx_spo2_alg_send_data(int32_t *red_new_raw_data,int32_t *ir_new_raw_data,int32_t *green_new_raw_data,uint8_t R_LEDDAC,uint8_t IR_LEDDAC,uint8_t G_LEDDAC, int16_t *gsen_data_x, int16_t *gsen_data_y, int16_t *gsen_data_z, uint8_t dat_len);
bool tyhx_spo2_alg(int32_t red_new_raw_data , int32_t ir_new_raw_data, int32_t green_new_raw_data,uint8_t R_LEDDAC,uint8_t IR_LEDDAC,uint8_t G_LEDDAC);
void tyhx_spo2_alg_close(void);
bool tyhx_spo2_alg_open(void);
bool tyhx_spo2_alg_open_deep(void);
spo2_results_t tyhx_spo2_alg_get_results(void);
spo2_debug_t tyhx_spo2_alg_debug_results(void);

#endif 

