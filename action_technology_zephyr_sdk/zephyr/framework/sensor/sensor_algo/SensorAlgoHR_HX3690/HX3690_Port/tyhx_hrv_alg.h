#ifndef _TYHX_HRV_ALG_H_
#define _TYHX_HRV_ALG_H_
#endif
	
#include <stdint.h>

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

#ifndef  bool
#define  bool unsigned char
#endif
#ifndef  true
#define  true  1
#endif
#ifndef  false
#define  false  0
#endif

#define HRV_GLITCH_THRES     20000 
#define HRV_GSEN_POW_THRE    150

#ifdef HRV_ALG_LIB

typedef enum {
    MSG_HRV_ALG_NOT_OPEN,
    MSG_HRV_ALG_OPEN,
    MSG_HRV_READY,
    MSG_HRV_ALG_TIMEOUT,
} hrv_alg_msg_code_t;

typedef struct {
    hrv_alg_msg_code_t  hrv_alg_status;
    uint32_t    data_cnt;
    uint8_t     hrv_result;
    uint32_t    hrv_peak; 
    uint8_t     spirit_pressure;
    uint8_t     p_m;
    uint8_t     screen_up;
    uint8_t     motion;
    uint8_t     signal_quality;
} hrv_results_t;


void tyhx_hrv_alg(int32_t new_raw_data);
bool tyhx_hrv_alg_open(void);
bool tyhx_hrv_alg_open_deep(void);
void tyhx_hrv_alg_close(void);
void kfft(double *pr,double *pi,int n,int k,double *fr,double *fi);

hrv_results_t tyhx_hrv_alg_send_data(int32_t new_raw_data, int32_t green_data_als, int32_t infrared_data);
hrv_results_t tyhx_hrv_alg_send_bufdata(int32_t *new_raw_data, uint8_t count, int16_t *gsen_data_x, int16_t *gsen_data_y, int16_t *gsen_data_z);
#endif //HRV_ALG_LIB
		
