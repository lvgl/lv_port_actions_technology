#ifndef _hx3605_H_
#define _hx3605_H_
#include <stdint.h>
#include <stdbool.h>

#include <hr_algo.h>

//******************** alg_swi *********************//
#define HRS_ALG_LIB
#define SPO2_ALG_LIB
//#define HRV_ALG_LIB
//#define CHECK_TOUCH_LIB
//#define CHECK_LIVING_LIB
#define BP_CUSTDOWN_ALG_LIB

//#define BIG_LIB           // 
//**************** read_data_mode ******************//
#define TIMMER_MODE           //timmer read fifo
//#define INT_MODE							//fifo_all_most_full
//#define TYHX_DEMO           // for ty demo
//****************** gsen_cgf *********************//
//#define GSENSER_DATA
//#define GSEN_40MS_TIMMER
//#define NEW_GSEN_SCHME

//****************** other_cgf ********************//
#define DEMO_COMMON
//#define INT_FLOATING
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

#if 0
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

#if defined(DEMO_COMMON)
  #define HRS4100_IIC_CLK  0
  #define HRS4100_IIC_SDA  30   
  #define LIS3DH_IIC_CLK   18
  #define LIS3DH_IIC_SDA   16
  #define EXT_INT_PIN      1
  #define GREEN_LED_SLE    1
  #define RED_LED_SLE      4
  #define IR_LED_SLE       2
  #define RED_AGC_OFFSET   50
  #define IR_AGC_OFFSET    50
	#define HR_GREEN_AGC_OFFSET 4
  #define BIG_SCREEN
	
#elif defined(DEMO_NEW)
  #define HRS4100_IIC_CLK  0
  #define HRS4100_IIC_SDA  30   
  #define LIS3DH_IIC_CLK   18
  #define LIS3DH_IIC_SDA   16
  #define EXT_INT_PIN      1
  #define GREEN_LED_SLE    1
  #define RED_LED_SLE      2
  #define IR_LED_SLE       4
  #define RED_AGC_OFFSET   50
  #define IR_AGC_OFFSET    50
	#define HR_GREEN_AGC_OFFSET 4
  #define BIG_SCREEN
  
#elif defined(DEMO_BAND)
  #define HRS4100_IIC_CLK  4
  #define HRS4100_IIC_SDA  3  
  #define LIS3DH_IIC_CLK   14
  #define LIS3DH_IIC_SDA   12
  #define EXT_INT_PIN      7
  #define GREEN_LED_SLE    0
  #define RED_LED_SLE      2
  #define IR_LED_SLE       1
  #define RED_AGC_OFFSET   64
  #define IR_AGC_OFFSET    64
  #define GREEN_AGC_OFFSET 8
	#define HR_GREEN_AGC_OFFSET 8
  
#elif defined(DEMO_GT01)
  #define HRS4100_IIC_CLK  5
  #define HRS4100_IIC_SDA  6
  #define LIS3DH_IIC_CLK   14
  #define LIS3DH_IIC_SDA   12 
  #define EXT_INT_PIN      7
  #define GREEN_LED_SLE    1
  #define RED_LED_SLE      4
  #define IR_LED_SLE       2
  #define RED_AGC_OFFSET   32
  #define IR_AGC_OFFSET    32
  #define GREEN_AGC_OFFSET 8
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
  #define HRS4100_IIC_CLK  9
  #define HRS4100_IIC_SDA  10  
  #define LIS3DH_IIC_CLK   13
  #define LIS3DH_IIC_SDA   14
  #define EXT_INT_PIN      11
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
} hx3605_mode_t;

typedef enum {    
	SENSOR_OK, 
	SENSOR_OP_FAILED,   
} SENSOR_ERROR_T;

typedef enum {    
	HRS_MODE, 
	SPO2_MODE,
	WEAR_MODE,
	HRV_MODE,
	LIVING_MODE,
	LAB_TEST_MODE,
	FT_LEAK_LIGHT_MODE,
	FT_GRAY_CARD_MODE,
	FT_INT_TEST_MODE
} WORK_MODE_T;

typedef enum {
	MSG_LIVING_NO_WEAR,
	MSG_LIVING_WEAR
} hx3605_living_wear_msg_code_t;

typedef enum {
	MSG_NO_WEAR,
	MSG_WEAR
} hx3605_wear_msg_code_t;

typedef struct {
	hx3605_living_wear_msg_code_t  wear_status;
	uint32_t           data_cnt;
	uint8_t            signal_quality;
	uint8_t            motion_status;
} hx3605_living_results_t;

#ifdef BIG_LIB
extern uint8_t alg_ram[11 * 1024];
#else
extern uint8_t alg_ram[5 * 1024];
#endif

uint16_t hx3605_read_fifo_data(int32_t *buf, uint8_t phase_num, uint8_t sig);
void read_data_packet(int32_t *ps_data);
void hx3605_delay_us(uint32_t us);
void hx3605_delay(uint32_t ms);
bool hx3605_write_reg(uint8_t addr, uint8_t data); 
bool hx3605_write_reg(uint8_t addr, uint8_t data);
uint8_t hx3605_read_reg(uint8_t addr); 
bool hx3605_brust_read_reg(uint8_t addr , uint8_t *buf, uint8_t length);
bool hx3605_chip_check(void);
uint8_t hx3605_read_fifo_size(void);
void hx3605_ppg_off(void);
void hx3605_ppg_on(void);  
void hx3605_ppg_timer_cfg(bool en);
void hx3605_agc_timer_cfg(bool en);
void hx3605_gpioint_cfg(bool en);
bool hx3605_init(WORK_MODE_T mode);
void hx3605_agc_Int_handle(void); 
void hx3605_gesensor_Int_handle(void);
void hx3605_spo2_ppg_init(void);
void hx3605_spo2_ppg_Int_handle(void);
void hx3605_wear_ppg_Int_handle(void);
void hx3605_ft_hrs_Int_handle(void);
void hx3605_ft_spo2_Int_handle(void);
void hx3605_hrs_ppg_init(void);
void hx3605_hrs_ppg_Int_handle(void);
void hx3605_hrv_ppg_Int_handle(void);
void hx3605_living_Int_handle(void);

void hx3605_ppg_Int_handle(void);
uint32_t hx3605_timers_start(void);
uint32_t hx3605_timers_stop(void);
uint32_t hx3605_gpioint_init(void);

uint32_t hx3605_gpioint_enable(void);
uint32_t hx3605_gpioint_disable(void);
uint32_t hx3605_40ms_timers_start(void);
uint32_t hx3605_40ms_timers_stop(void);

void hx3605_vin_check(uint16_t led_vin);
uint32_t tyhx_hrs_get_ram_size(void);

#ifdef LAB_TEST
void hx3605_lab_test_init(void);
void hx3605_test_alg_cfg(void);
SENSOR_ERROR_T hx3605_lab_test_enable(void);
void hx3605_lab_test_Int_handle(void);
void hx3605_lab_test_read_packet(uint32_t *data_buf);
#endif

#ifdef CHECK_TOUCH_LIB
hx3605_wear_msg_code_t hx3605_check_touch(int32_t *ir_data, uint8_t data_len);
SENSOR_ERROR_T hx3605_check_touch_enable(void);
#endif

bool hx3605_living_send_data(int32_t *new_raw_data, uint8_t dat_len, uint32_t green_data_als, int16_t *gsen_data_x, int16_t *gsen_data_y, int16_t *gsen_data_z);
hx3605_living_results_t hx3605_living_get_results(void);
#endif

void ppg_timeout_handler(void * p_context);
