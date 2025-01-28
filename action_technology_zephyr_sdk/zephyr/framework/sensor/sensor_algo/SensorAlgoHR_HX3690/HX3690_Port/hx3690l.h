#ifndef _hx3690l_H_
#define _hx3690l_H_
#include <stdint.h>
#include <stdbool.h>

/********************* alg_swi *********************
HRS_ALG_LIB   心率功能
SPO2_ALG_LIB  血氧功能
HRV_ALG_LIB   HRV功能
CHECK_TOUCH_LIB   纯佩戴功能（非心率血氧模式）
CHECK_LIVING_LIB  纯物体识别功能（非心率血氧模式）
BP_CUSTDOWN_ALG_LIB  血压功能

TIMMER_MODE  定时器模式， 读PPG数据， 默认采用定时器模式
INT_MODE     中断模式，读PPG数据

DEMO_MI      demo相关配置， 客户关闭， 在下面else里进行配置
TYHX_DEMO    TYHX专用平台demo， 客户调试时关闭
HRS_BLE_APP  DEMO蓝牙控制宏， 客户关闭
SPO2_DATA_CALI   血氧DC补偿模式， 客户关闭
HRS_DEBUG    调试宏， 根据平台定义打印函数
AGC_DEBUG    调试宏， 根据平台定义打印函数
GSEN_40MS_TIMMER   DEMO调试使用， 客户关闭
NEW_GSEN_SCHME     DEMO调试使用， 客户关闭
*/
#define HRS_ALG_LIB   
#define SPO2_ALG_LIB
//#define HRV_ALG_LIB
//#define CHECK_TOUCH_LIB
//#define CHECK_LIVING_LIB
#define BP_CUSTDOWN_ALG_LIB

//**************** read_data_mode ******************//
#define TIMMER_MODE           //timmer read fifo
//#define INT_MODE				//fifo_all_most_full

//****************** gsen_cgf *********************//
//#define GSEN_40MS_TIMMER
//#define NEW_GSEN_SCHME

//****************** other_cgf ********************//
#define DEMO_COMMON
//#define TYHX_DEMO
//#define HRS_BLE_APP
//#define SPO2_DATA_CALI

//***************** vecter_swi ********************//
//#define SPO2_VECTOR
//#define HR_VECTOR
//#define HRV_TESTVEC

//**************** lab_test_mode ******************//
//#define LAB_TEST             
//#define LAB_TEST_AGC							

//****************** print_swi ********************//
//#define HRS_DEBUG
//#define AGC_DEBUG

//**************************************************//
#ifdef AGC_DEBUG
#define  AGC_LOG(...)     SEGGER_RTT_printf(0,__VA_ARGS__)
#else
#define	 AGC_LOG(...)
#endif

#ifdef HRS_DEBUG
#define  DEBUG_PRINTF(...)     SEGGER_RTT_printf(0,__VA_ARGS__)
#else
#define	 DEBUG_PRINTF(...)
#endif

#if defined(DEMO_COMMON)
	#define HRS4100_IIC_CLK  30
	#define HRS4100_IIC_SDA  0   
	#define LIS3DH_IIC_CLK   18
	#define LIS3DH_IIC_SDA   16
	#define EXT_INT_PIN      1
	#define GREEN_LED_SLE    1
	#define RED_LED_SLE      4
	#define IR_LED_SLE       2
	#define RED_AGC_OFFSET   64
	#define IR_AGC_OFFSET    64
	#define GREEN_AGC_OFFSET 4
	#define HR_GREEN_AGC_OFFSET 10
	#define BIG_SCREEN
	
#elif defined(DEMO_MI)
	#define HRS4100_IIC_CLK  30
	#define HRS4100_IIC_SDA  0   
	#define LIS3DH_IIC_CLK   18
	#define LIS3DH_IIC_SDA   16
	#define EXT_INT_PIN      1
	#define GREEN_LED_SLE    1
	#define RED_LED_SLE      2
	#define IR_LED_SLE       4
	#define RED_AGC_OFFSET   50
	#define IR_AGC_OFFSET    50
	#define GREEN_AGC_OFFSET 4
	#define HR_GREEN_AGC_OFFSET 4
	#define BIG_SCREEN
	
#elif defined(DEMO_NEW)
	#define HRS4100_IIC_CLK  30
	#define HRS4100_IIC_SDA  0   
	#define LIS3DH_IIC_CLK   18
	#define LIS3DH_IIC_SDA   16
	#define EXT_INT_PIN      1
	#define GREEN_LED_SLE    1
	#define RED_LED_SLE      2
	#define IR_LED_SLE       4
	#define RED_AGC_OFFSET   75
	#define IR_AGC_OFFSET    75
	#define GREEN_AGC_OFFSET 4
	#define HR_GREEN_AGC_OFFSET 8
	#define BIG_SCREEN
  
#elif defined(EVB)
	#define HRS4100_IIC_CLK  9
	#define HRS4100_IIC_SDA  10  
	#define LIS3DH_IIC_CLK   13
	#define LIS3DH_IIC_SDA   14
	#define EXT_INT_PIN      11
	#define GREEN_LED_SLE    1
	#define RED_LED_SLE      2
	#define IR_LED_SLE       4
	#define RED_AGC_OFFSET   64
	#define IR_AGC_OFFSET    64
	#define GREEN_AGC_OFFSET 8
	#define HR_GREEN_AGC_OFFSET 10
    
#else
	#define GREEN_LED_SLE    1
	#define RED_LED_SLE      4
	#define IR_LED_SLE       2
	#define RED_AGC_OFFSET   64
	#define IR_AGC_OFFSET    64
	#define GREEN_AGC_OFFSET 8
    
#endif

typedef enum {    
	PPG_INIT, 
	PPG_OFF, 
	PPG_LED_OFF,
	CAL_INIT,
	CAL_OFF,
	RECAL_INIT    
} hx3690l_mode_t;

typedef enum {    
	SENSOR_OK, 
	SENSOR_OP_FAILED,   
} SENSOR_ERROR_T;

typedef enum {    
	HRS_MODE, 
	SPO2_MODE,
	HRSPO2_MODE,
	WEAR_MODE,
	HRV_MODE,
	LIVING_MODE,
	LAB_TEST_MODE,
	FT_LEAK_LIGHT_MODE,
	FT_GRAY_CARD_MODE,
	FT_INT_TEST_MODE,
	FT_SINGLE_CHECK_MODE,
    FT_LED_OFF_MODE,
    FT_WEAR_MODE
} WORK_MODE_T;

typedef struct {
	uint8_t count;
	int32_t red_data[16];
	int32_t green_data[64];
	int32_t ir_data[16];  
	int32_t s_buf[64];
	uint8_t green_cur;
	uint8_t red_cur; 
	uint8_t ir_cur;
	uint8_t green_offset;
	uint8_t red_offset; 
	uint8_t ir_offset;     
}ppg_sensor_data_t;

typedef enum {
    MSG_LIVING_INIT,
	MSG_LIVING_NO_WEAR,
	MSG_LIVING_WEAR
} hx3690_living_wear_msg_code_t;

typedef enum {
	MSG_NO_WEAR,
	MSG_WEAR
} hx3690_wear_msg_code_t;

typedef struct {
	hx3690_living_wear_msg_code_t  wear_status;
	uint32_t           data_cnt;
	uint8_t            signal_quality;
	uint8_t            motion_status;
} hx3690_living_results_t;

typedef struct {
    int32_t p1_noise; 
	int32_t p2_noise; 
	int32_t p3_noise;
	int32_t p4_noise;
}NOISE_PS_T;


extern uint8_t alg_ram[11*1024];
//extern uint8_t alg_ram[10];

void hx3690l_delay_us(uint32_t us);
void hx3690l_delay_ms(uint32_t ms);
bool hx3690l_write_reg(uint8_t addr, uint8_t data); 
uint8_t hx3690l_read_reg(uint8_t addr); 
bool hx3690l_brust_read_reg(uint8_t addr , uint8_t *buf, uint8_t length);
bool hx3690l_chip_check(void);
uint8_t hx3690l_read_fifo_size(void);
uint8_t hx3690l_read_fifo_data(int32_t *s_buf, uint8_t size, uint8_t phase_num);
void hx3690l_ppg_off(void);
void hx3690l_ppg_on(void);  
void hx3690l_320ms_timer_cfg(bool en);
void hx3690l_40ms_timer_cfg(bool en);
void hx3690l_gpioint_cfg(bool en);
bool hx3690l_init(WORK_MODE_T mode);
void hx3690l_agc_Int_handle(void); 
void hx3690l_gesensor_Int_handle(void);
void hx3690l_spo2_ppg_init(void);
void hx3690_spo2_data_cali_init(void);
void hx3690l_spo2_ppg_Int_handle(void);
void hx3690l_check_touch_init(void);
void hx3690l_wear_ppg_Int_handle(void);
void hx3690l_hrs_ppg_init(void);
void hx3690l_hrs_ppg_Int_handle(void);
void hx3690l_hrv_ppg_init(void);
void hx3690l_hrv_ppg_Int_handle(void);
void hx3690l_living_Int_handle(void);
void hx3690l_hrspo2_ppg_Int_handle(void);

void hx3690l_ppg_Int_handle(void);
uint32_t hx3690_timers_start(void);
uint32_t hx3690_timers_stop(void);
uint32_t hx3690_gpioint_init(void);

uint32_t hx3690_gpioint_enable(void);
uint32_t hx3690_gpioint_disable(void);
uint32_t gsen_read_timers_start(void);
uint32_t gsen_read_timers_stop(void);

void hx3690l_vin_check(uint16_t led_vin);

#ifdef LAB_TEST
void hx3690l_lab_test_init(void);
void hx3690l_test_alg_cfg(void);
SENSOR_ERROR_T hx3690l_lab_test_enable(void);
void hx3690l_lab_test_Int_handle(void);
NOISE_PS_T hx3690l_lab_test_read_packet(void);
#endif

bool hx3690_living_send_data(int32_t *new_raw_data, uint8_t dat_len, int16_t *gsen_data_x, int16_t *gsen_data_y, int16_t *gsen_data_z);
hx3690_living_results_t hx3690_living_get_results(void);
#endif

void gsen_read_timeout_handler(void * p_context);
void heart_rate_meas_timeout_handler(void * p_context);
