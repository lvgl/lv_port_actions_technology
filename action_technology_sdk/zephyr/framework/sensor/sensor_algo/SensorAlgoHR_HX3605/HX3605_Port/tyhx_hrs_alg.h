#ifndef _TYHX_HRS_ALG_H_
#define _TYHX_HRS_ALG_H_
#endif

#include <stdint.h>
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

#ifndef  bool
#define  bool unsigned char
#endif
#ifndef  true
#define  true  1
#endif
#ifndef  false
#define  false  0
#endif


#ifdef HRS_ALG_LIB

typedef enum {
	NORMAL_MODE,
	RIDE_MODE,
	WALK_MODE,
	JUMP_MODE,
	ALLDAY_HR_MODE,
	HIGH_POW_MODE
} hrs_sports_mode_t;

typedef enum {
	MSG_HRS_ALG_NOT_OPEN,
	MSG_HRS_ALG_OPEN,
	MSG_HRS_READY,
	MSG_HRS_ALG_TIMEOUT,
} hrs_alg_msg_code_t;

typedef enum {
	MSG_LIVING_INITIAL,
	MSG_LIVING_TRUE,
	MSG_LIVING_FAIL
} living_msg_code_t;

typedef struct {
	hrs_alg_msg_code_t  hrs_alg_status;
    living_msg_code_t   living_status;
	uint32_t            data_cnt;
	uint8_t             hr_result;
	uint16_t            cal_result;
	uint8_t             hr_result_qual;
    uint32_t            hr_result_std;
} hrs_results_t;

typedef struct {
	uint32_t data_cnt;
	uint8_t  motion_status;
	uint32_t gsen_pow;
	int8_t   up_clip;
	int8_t   dn_clip;
	uint8_t  runrun_cnt;
} HRS_ALG_INFO_T;

#ifdef BP_CUSTDOWN_ALG_LIB
typedef enum {
	MSG_BP_ALG_NOT_OPEN = 0x01,
	MSG_BP_NO_TOUCH = 0x02,
	MSG_BP_PPG_LEN_TOO_SHORT = 0x03,
	MSG_BP_READY = 0x04,
	MSG_BP_ALG_TIMEOUT = 0x05,
	MSG_BP_SETTLE = 0x06
} bp_msg_code_t;

typedef struct {
	uint8_t age;
	uint8_t height;
	uint8_t weight;
	uint8_t gender;
	uint8_t ref_sbp;
	uint8_t ref_dbp;
} bp_exinf_t;

typedef struct {
	bp_msg_code_t   bp_alg_status;
	uint8_t         sbp;
	uint8_t         dbp; 
	uint32_t        data_cnt;
	uint8_t         hr_result;
	bool            object_flg;
} bp_results_t;


void tyhx_bp_set_exinf(uint8_t age, uint8_t height, uint8_t weight, uint8_t gender, uint8_t ref_sbp, uint8_t ref_dbp);
void tyhx_bp_age_fn(uint8_t hr_result, bp_exinf_t body_s);
void tyhx_restart_bp_cal(void);
bp_results_t tyhx_alg_get_bp_results(void);
#endif //BP_CUSTDOWN_ALG_LIB

bool tyhx_hrs_alg_open(void);
bool tyhx_hrs_alg_open_deep(void);
void tyhx_hrs_alg_close(void);
bool tyhx_hrs_alg_send_data(int32_t *new_raw_data,uint8_t dat_len, int16_t *gsen_data_x, int16_t *gsen_data_y, int16_t *gsen_data_z);
bool tyhx_hrs_alg(int32_t new_raw_data, int16_t gsen_data_x, int16_t gsen_data_y, int16_t gsen_data_z);
hrs_results_t tyhx_hrs_alg_get_results(void);
void tyhx_hrs_set_alg_para(hrs_sports_mode_t sports_mode, uint16_t static_thre_val, uint8_t gsen_fun_lv_val);
void tyhx_hrs_set_living(uint8_t mode, uint8_t qu_thre, uint8_t st_thre);
HRS_ALG_INFO_T tyhx_get_hrs_alg_info(void);
#endif //HRS_ALG_LIB
