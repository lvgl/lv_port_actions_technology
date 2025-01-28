#ifndef _TYHX_HRV_ALG_H_
#define _TYHX_HRV_ALG_H_
#endif
	
#include <stdint.h>


#define HRV_GLITCH_THRES     20000 

#ifdef HRV_ALG_LIB

typedef enum {
	MSG_HRV_ALG_NOT_OPEN,
  MSG_HRV_ALG_OPEN,
	MSG_HRV_READY,
	MSG_HRV_ALG_TIMEOUT,
} hrv_alg_msg_code_t;

typedef struct {
    hrv_alg_msg_code_t  hrv_alg_status;
    uint32_t           data_cnt;
    uint8_t            hrv_result;
    uint32_t           hrv_peak; 
		uint8_t            spirit_pressure;
		uint8_t            p_m;
} hrv_results_t;


void tyhx_hrv_alg(int32_t new_raw_data);
bool tyhx_hrv_alg_open(void);
bool tyhx_hrv_alg_open_deep(void);
void tyhx_hrv_alg_close(void);
void kfft(double *pr,double *pi,int n,int k,double *fr,double *fi);

hrv_results_t tyhx_hrv_alg_send_data(int32_t new_raw_data, int32_t green_data_als, int32_t infrared_data);
hrv_results_t tyhx_hrv_alg_send_bufdata(int32_t *new_raw_data, int32_t green_data_als, int32_t infrared_data);
#endif //HRV_ALG_LIB
		
