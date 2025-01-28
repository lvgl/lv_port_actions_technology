#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>

#include <hr_algo.h>

#include "hx3690l.h"
#ifdef HRS_ALG_LIB
#include "hx3690l_hrs_agc.h"
#include "tyhx_hrs_alg.h"
#endif
#ifdef SPO2_ALG_LIB
#include "hx3690l_spo2_agc.h"
#include "tyhx_spo2_alg.h"
#endif
#ifdef HRV_ALG_LIB
//#include "hx3690l_hrv_agc.h"
#include "tyhx_hrv_alg.h"
#endif
#ifdef CHECK_TOUCH_LIB
#include "hx3690l_check_touch.h"
#endif
#include "hx3690q_factory_test.h"

#ifdef TYHX_DEMO
#include "demo_ctrl.h"
#include "SEGGER_RTT.h"
#include "app_timer.h"
#include "nrf_delay.h"
#include "nrf_gpio.h"
#include "nrf_drv_gpiote.h" 
#include "twi_master.h"
#include "lis3dh_drv.h"
#include "oled_iic.h"
#include "word.h"
#include "iic.h"
#endif

#ifdef SPO2_VECTOR
#include "spo2_vec.h" 
uint32_t spo2_send_cnt = 0;
uint32_t spo2_send_cnt1 = 0;
int32_t red_buf_vec[8];
int32_t ir_buf_vec[8];
int32_t green_buf_vec[8];
#endif

#ifdef HR_VECTOR
#include "hr_vec.h" 
uint32_t vec_send_cnt = 0;
int32_t PPG_buf_vec[8];
#endif

#ifdef HRV_TESTVEC    
#include "hrv_testvec.h"
#endif

//spo2 agc
const uint8_t  hx3690l_spo2_agc_red_idac = RED_AGC_OFFSET;
const uint8_t  hx3690l_spo2_agc_ir_idac = IR_AGC_OFFSET;
const uint8_t  hx3690l_spo2_agc_green_idac = GREEN_AGC_OFFSET;
//hrs agc
const uint8_t  hx3690l_hrs_agc_idac = HR_GREEN_AGC_OFFSET;
//hrv agc
const uint8_t  hx3690l_hrv_agc_idac = HR_GREEN_AGC_OFFSET;

const uint8_t  green_led_max_init = 255;
const uint8_t  red_led_max_init = 255;
const uint8_t  ir_led_max_init = 255;

uint8_t low_vbat_flag =0;
WORK_MODE_T work_mode_flag = HRS_MODE;
const uint16_t static_thre_val = 150;
const uint8_t gsen_lv_val = 0;

//HRS_INFRARED_THRES
const int32_t  hrs_ir_unwear_thres = 10000;
const int32_t  hrs_ir_wear_thres = 20000; 
//HRV_INFRARED_THRES
const int32_t  hrv_ir_unwear_thres = 10000;
const int32_t  hrv_ir_wear_thres = 20000; 
//SPO2_INFRARED_THRES
const int32_t  spo2_ir_unwear_thres = 10000; 
const int32_t  spo2_ir_wear_thres = 30000;
const uint8_t  spo2_ir_wear_offset = 12;
//CHECK_WEAR_MODE_THRES
const int32_t  check_mode_unwear_thre = 20000;
const int32_t  check_mode_wear_thre = 40000;

uint8_t alg_ram[11 * 1024] __attribute__((aligned(4)));
//uint8_t alg_ram[10] = {0};
ppg_sensor_data_t ppg_s_data;

#ifdef TYHX_DEMO
extern volatile oled_display_t oled_dis;
uint32_t vec_data_cnt=0;
int16_t gsen_fifo_x[32];
int16_t gsen_fifo_y[32];
int16_t gsen_fifo_z[32];
#endif

//////// spo2 para and switches
const  uint8_t   COUNT_BLOCK_NUM = 25;            //delay the block of some single good signal after a series of bad signal 
const  uint8_t   SPO2_LOW_XCORR_THRE = 30;        //(64*xcorr)'s square below this threshold, means error signal
const  uint8_t   SPO2_CALI = 1;                       //spo2_result cali mode
const  uint8_t   XCORR_MODE = 1;                  //xcorr mode switch
const  uint8_t   QUICK_RESULT = 1;                //come out the spo2 result quickly ;0 is normal,1 is quick
const  uint16_t  MEAN_NUM = 32;                  //the length of smooth-average ;the value of MEAN_NUM can be given only 256 and 512
const  uint8_t   G_SENSOR = 0;                      //if =1, open the gsensor mode
const  uint8_t   SPO2_GSEN_POW_THRE = 150;         //gsen pow judge move, range:0-200;
const  uint32_t  SPO2_BASE_LINE_INIT = 135500;    //spo2 baseline init, = 103000 + ratio(a variable quantity,depends on different cases)*SPO2_SLOPE
const  int32_t   SOP2_DEGLITCH_THRE = 100000;     //remove signal glitch over this threshold
const  int32_t   SPO2_REMOVE_JUMP_THRE = 50000;  //remove signal jump over this threshold
const  uint32_t  SPO2_SLOPE = 50000;              //increase this slope, spo2 reduce more
const  uint16_t  SPO2_LOW_CLIP_END_TIME = 1500;   //low clip mode before this data_cnt, normal clip mode after this
const  uint16_t  SPO2_LOW_CLIP_DN  = 150;         //spo2 reduce 0.15/s at most in low clip mode
const  uint16_t  SPO2_NORMAL_CLIP_DN  = 500;      //spo2 reduce 0.5/s at most in normal clip mode
const  uint8_t   SPO2_LOW_SNR_THRE = 40;          //snr below this threshold, means error signal
const  uint16_t  IR_AC_TOUCH_THRE = 200;          //AC_min*0.3
const  uint16_t  IR_FFT_POW_THRE = 500;           //fft_pow_min
const  uint8_t   SLOPE_PARA_MAX = 28;
const  uint8_t   SLOPE_PARA_MIN = 3;

#ifdef SPO2_DATA_CALI
static int32_t red_data_fifo[4] = {0};
static int32_t ir_data_fifo[4] = {0};
static int32_t red_dc_temp = 0;
static int32_t ir_dc_temp = 0;
static int32_t red_data_pre = 0;
static int32_t ir_data_pre = 0;
static int32_t red_jump_delta = 0;
static int32_t ir_jump_delta = 0;
static int32_t cali_data_cnt = 0;

#endif

/* hr os api */
extern hr_os_api_t hr_os_api;

// us 延时函数， 客户需填写
void hx3690l_delay_us(uint32_t us)
{
//    nrf_delay_us(us);
//	printk("hx3690l_delay_us %d \r\n",us);
	sensor_udelay((int32_t)us);
}

// ms 延时函数， 客户需填写，
void hx3690l_delay_ms(uint32_t ms)
{
//    nrf_delay_ms(ms);
//	printk("hx3690l_delay %d \r\n",ms);
	sensor_mdelay((int32_t)ms);
}

// i2c 写函数， 客户需填写, 地址0x44(7bit)
bool hx3690l_write_reg(uint8_t addr, uint8_t data) 
{
//    uint8_t data_buf[2];    
//    data_buf[0] = addr;
//    data_buf[1] = data;
//    twi_pin_switch(1);
//    twi_master_transfer(0x88, data_buf, 2, true);    //write   
    sensor_hal_write(ID_HR, addr, &data, 1);
    return true;      
}

// i2c 读函数， 客户需填写
uint8_t hx3690l_read_reg(uint8_t addr) 
{
    uint8_t data_buf;    
//    twi_pin_switch(1);
//    twi_master_transfer(0x88, &addr, 1, false);      //write
//    twi_master_transfer(0x89, &data_buf, 1, true);//read
	sensor_hal_read(ID_HR, addr, &data_buf, 1);
    return data_buf;      
}

// i2c 连读函数， 客户需填写
bool hx3690l_brust_read_reg(uint8_t addr , uint8_t *buf, uint8_t length) 
{
//    twi_pin_switch(1);
//    twi_master_transfer(0x88, &addr, 1, false);      //write
//    twi_master_transfer(0x89, buf, length, true); //read
    sensor_hal_read(ID_HR, addr, buf, length);  
    return true;      
}

bool timer_40ms_en = 0;
bool timer_320ms_en = 0;
uint8_t timer_320ms_cnt = 0;

/* 完成读取PPG数据定时器接口， 循环调用， 默认320ms
void heart_rate_meas_timeout_handler(void * p_context)
*/
void hx3690l_320ms_timer_cfg(bool en)
{
    // if(en)
    // {
    //     hx3690_timers_start();
    // }
    // else
    // {
    //     hx3690_timers_stop();
    // }
	timer_320ms_en = en;
	timer_320ms_cnt = 0;
	printk("320ms %d \r\n",en);
}
/* 完成agc 调节接口， 循环调用， 默认40ms， 程序调节完成会自行停止
void gsen_read_timeout_handler(void * p_context)
*/
void hx3690l_40ms_timer_cfg(bool en)
{
    // if(en)
    // {
        // #if defined(GSENSER_DATA)||!defined(EXT_INT_AGC)  
        // gsen_read_timers_start();   
        // #endif 
    // }
    // else
    // {
        // #if defined(GSENSER_DATA)||!defined(EXT_INT_AGC)  
        // gsen_read_timers_stop();   
        // #endif 
    // }
    timer_40ms_en = en;
	printk("40ms %d \r\n",en);
}

#if defined(INT_MODE)
/* 如采用中断模式， 如完成中断开启和关闭接口
void hx3690l_ppg_Int_handle(void)
*/
void hx3690l_gpioint_cfg(bool en)
{
    if(en)
    {
        //  hx3690_gpioint_enable();
    }
    else
    {
        //  hx3690_gpioint_disable();
    }
}
#endif

bool hx3690l_chip_check(void)
{
    uint8_t i = 0;
    uint8_t chip_id = 0;    
    for(i=0;i<10;i++)
    {
        hx3690l_write_reg(0x02, 0x30);
        hx3690l_delay_ms(5);
        chip_id = hx3690l_read_reg(0x00);  
        if (chip_id == 0x69)
        {
            return true;
        }        
    }
    return false;
}

uint8_t hx3690l_read_fifo_size(void)
{
    uint8_t fifo_num_temp = 0;
    fifo_num_temp = hx3690l_read_reg(0x14)&0x7f;

    return fifo_num_temp;
}

uint8_t hx3690l_read_fifo_data(int32_t *s_buf, uint8_t size, uint8_t phase_num)
{
    uint8_t data_flg = 127;
    int32_t data;
    uint8_t databuf[3];
    uint8_t ii=0;
    uint8_t data_cnt = 0;
    uint8_t data_ready = 0;
    uint8_t fifo_out_count = 0;
    
    for(ii=0; ii<size - phase_num; ii++) 
    {
        hx3690l_write_reg(0x17, 0x00);
        hx3690l_delay_us(100);
        hx3690l_brust_read_reg(0x15, databuf, 3); 
        data_flg = databuf[2]>>5;
        data = (int32_t)(databuf[0]|(databuf[1]<<8)|((databuf[2]&0x1f)<<16));

        if(data_ready == 0)
        {
            if(data_flg == 0)
            {
                s_buf[data_cnt] = data;
                data_cnt++;
                data_ready = 1;
            }
        }
        else
        {
            s_buf[data_cnt] = data;
            data_cnt++;
        }
    }
    if(data_flg != phase_num-1)
    {
        for(ii=data_flg+1; ii<phase_num; ii++)
        {
            hx3690l_write_reg(0x17, 0x00);
            hx3690l_delay_us(100);
            hx3690l_brust_read_reg(0x15, databuf, 3); 
            data_flg = databuf[2]>>5;
            data = (int32_t)(databuf[0]|(databuf[1]<<8)|((databuf[2]&0x1f)<<16));
            s_buf[data_cnt] = data;
            data_cnt++;
        }
    }
    if(size==64)
    {
        hx3690l_write_reg(0X51, 0x02);
        hx3690l_delay_ms(5);
        hx3690l_write_reg(0X51, 0x00);
    }
    fifo_out_count = data_cnt/phase_num;
    return fifo_out_count;
}

void hx3690l_vin_check(uint16_t led_vin)
{
    low_vbat_flag = 0;
    if(led_vin < 3700)
    {
      low_vbat_flag = 1;      
    }
}

void hx3690l_ppg_off(void)
{
	hx3690l_write_reg(0x6a, 0x00);
	hx3690l_delay_ms(5);
	hx3690l_write_reg(0x51, 0x02);
	hx3690l_delay_ms(5);
	hx3690l_write_reg(0x02, 0x31);
}

void hx3690l_ppg_on(void)
{
	hx3690l_write_reg(0x02, 0x30);
	hx3690l_delay_ms(5);
}

void hx3690l_data_reset(void)
{
	#if defined(TIMMER_MODE)
	hx3690l_320ms_timer_cfg(false);
	hx3690l_40ms_timer_cfg(false);
	#elif defined(INT_MODE)
	hx3690l_gpioint_cfg(false);
	#endif	
	#if defined(HRS_ALG_LIB)
	hx3690l_hrs_data_reset();
	#endif
	#if defined(SPO2_ALG_LIB)
	hx3690l_spo2_data_reset();
	#endif
	#if defined(HRV_ALG_LIB)
	hx3690l_hrv_data_reset();
	#endif
	AGC_LOG("hx3690l data reset!\r\n");
}

#ifdef HRS_ALG_LIB
// 心率模式下ppg 初始化配置
void hx3690l_hrs_ppg_init(void) //20200615 ericy ppg fs=25hz, phase3 conversion ready interupt en
{
	uint16_t sample_rate = 25;  /*config the data rate of chip alps2_fm ,uint is Hz*/

	uint32_t prf_clk_num = 32000/sample_rate;   /*period in clk num, num = Fclk/fs */

	uint8_t cic_mode_en =0;
	uint8_t cic_b2_en = 0;
	uint8_t samp_delay_leden_num = 0; /* 00=8,01=16,10=32,11=64*/
	uint8_t samp_copy_avg = 0;        /* 0 = 1 ; 1 = 2; 2 =4 ; 3 =8 ; 4 =16;*/
	uint8_t data_avg_num = 0;         /* 0 = 1 ; 1 = 2; 2 =4 ; 3 =8 ; 4 =16;*/
	uint8_t phase3_4_internal = 0;    /* phase3 and phase4 prf internal cfg */

	uint8_t phase1_enable = 1;      /*phase1_enable  , 1 mean enable ; 0 mean disable */
	uint8_t phase2_enable = 1;      /*phase2_enable  , 1 mean enable ; 0 mean disable */
	uint8_t phase3_enable = 1;      /*phase3_enable  , 1 mean enable ; 0 mean disable */
	uint8_t phase4_enable = 1;      /*phase4_enable  , 1 mean enable ; 0 mean disable */

	uint8_t phase1_adc_osr = 3;     /* 0 = 128 ; 1 = 256 ; 2 = 512 ; 3 = 1024 ; 4 = 2048*/
	uint8_t phase2_adc_osr = 3;     /* 0 = 128 ; 1 = 256 ; 2 = 512 ; 3 = 1024 ; 4 = 2048*/
	uint8_t phase3_adc_osr = 3;     /* 0 = 128 ; 1 = 256 ; 2 = 512 ; 3 = 1024 ; 4 = 2048*/
	uint8_t phase4_adc_osr = 3;     /* 0 = 128 ; 1 = 256 ; 2 = 512 ; 3 = 1024 ; 4 = 2048*/

	uint8_t phase1_inner_avg = 0;   /* phase1 adc avg num 0=1, 1=2, 2=4, 3=8 , 4=16*/
	uint8_t phase1_tia_res = 0;     /* 0= 10K; 1= 20k; 2= 50k; 3= 100k; 4= 150k; 5= 200k; 6= 500k; 7= 1M*/
	uint8_t phase1_ldr_sel = GREEN_LED_SLE;     /* 1 = ldr0; 2 = ldr1; 4 = ldr2; */
	uint8_t phase1_pd_sel = 1;      /* 1 = pd1; 2 = pd2; */
	uint8_t phase1_offset_idac = 0; /* 0~127 = 0 ~ 32ua , step = 0.25ua */
	uint8_t phase1_ldr_cur = 0;     /* 0~255 = 0 ~ 200ma, step = 0.8ma */
	uint8_t phase1_led_en = 1;      /* phase1 led enable*/

	uint8_t phase2_inner_avg = 0;   /* phase2 adc avg num 0=1, 1=2, 2=4, 3=8 , 4=16*/
	uint8_t phase2_tia_res = 0;     /* 0= 10K; 1= 20k; 2= 50k; 3= 100k; 4= 150k; 5= 200k; 6= 500k; 7= 1M*/
	uint8_t phase2_ldr_sel = 0;     /* 1 = ldr0; 2 = ldr1; 4 = ldr2; */
	uint8_t phase2_pd_sel = 1;      /* 1 = pd1; 2 = pd2; */
	uint8_t phase2_offset_idac = 0; /* 0~127 = 0 ~ 32ua , step = 0.25ua */
	uint8_t phase2_ldr_cur = 0;     /* 0~255 = 0 ~ 200ma, step = 0.8ma */
	uint8_t phase2_led_en = 0;      /* phase2 led enable*/

	uint8_t phase3_inner_avg = 0;   /* phase3 adc avg num 0=1, 1=2, 2=4, 3=8 , 4=16*/
	uint8_t phase3_tia_res = 0;     /* 0= 10K; 1= 20k; 2= 50k; 3= 100k; 4= 150k; 5= 200k; 6= 500k; 7= 1M*/
	uint8_t phase3_ldr_sel = IR_LED_SLE;     /* 1 = ldr0; 2 = ldr1; 4 = ldr2; */
	uint8_t phase3_pd_sel = 1;      /* 1 = pd1; 2 = pd2; */
	uint8_t phase3_offset_idac = 0; /* 0~127 = 0 ~ 32ua , step = 0.25ua */
	uint8_t phase3_ldr_cur = 64;    /* 0~255 = 0 ~ 200ma, step = 0.8ma */
	uint8_t phase3_led_en = 1;      /* phase3 led enable*/

	uint8_t phase4_inner_avg = 0;   /* phase4 adc avg num 0=1, 1=2, 2=4, 3=8 , 4=16*/
	uint8_t phase4_tia_res = 0;     /* 0= 10K; 1= 20k; 2= 50k; 3= 100k; 4= 150k; 5= 200k; 6= 500k; 7= 1M*/
	uint8_t phase4_ldr_sel = 0;     /* 1 = ldr0; 2 = ldr1; 4 = ldr2; */
	uint8_t phase4_pd_sel = 1;      /* 1 = pd1; 2 = pd2; */
	uint8_t phase4_offset_idac = 0; /* 0~127 = 0 ~ 32ua , step = 0.25ua */
	uint8_t phase4_ldr_cur = 0;     /* 0~255 = 0 ~ 200ma, step = 0.8ma */
	uint8_t phase4_led_en = 0;      /* phase4 led enable*/

	uint8_t init_wait_delay = 5 ; 	/* 0 = 31clk ; 1 = 64clk ; 2 = 127clk ; 3 = 255clk(d) ;
                                       4 = 511clk; 5 = 1023clk; 6 = 2047; 7 = 2048clk */
	uint8_t afe_reset = 3;        	/* 0 = 15clk ; 1 = 31clk ; 2 = 63clk ; 3 = 127clk(d) ;
                                       4 = 255clk; 5 = 511clk; 6 = 1024; 7 = 2048clk */
	uint8_t led_on_time = 3;      	/* 0 = 32clk=8us ; 1 = 64clk=16us; 2=128clk=32us ; 3 = 256clk=64us ;
                                       4 = 512clk=128us ; 5 = 1024clk=256us; 6= 2048clk=512us; 7 = 4096clk=1024us */

	hx3690l_write_reg(0x02, 0x31);
	hx3690l_delay_ms(10);
	hx3690l_write_reg(0x02, 0x30);
	hx3690l_delay_ms(5);
	hx3690l_write_reg(0X6a, 0X00);
	hx3690l_delay_ms(5);

	hx3690l_write_reg(0X1a, (uint8_t)prf_clk_num);    // prf bit<7:0>
	hx3690l_write_reg(0X1b, (uint8_t)(prf_clk_num>>8)); // prf bit<15:8>
	hx3690l_write_reg(0X1c, (uint8_t)(prf_clk_num>>16)); // prf bit<23:16>

	hx3690l_write_reg(0X1d, phase3_4_internal);
	hx3690l_write_reg(0X1e, ((afe_reset<<3)| 0x00) );
	hx3690l_write_reg(0X1f, (led_on_time<<4| phase1_led_en<<3 | phase3_led_en<<2 | phase4_led_en<<1 | phase2_led_en) );

	hx3690l_write_reg(0X26, (init_wait_delay<<4 | 0x0f));
	hx3690l_write_reg(0X27, (phase1_inner_avg | (phase2_inner_avg<<4)));
	hx3690l_write_reg(0X28, (phase3_inner_avg | (phase4_inner_avg<<4)));
	hx3690l_write_reg(0X29, cic_mode_en<<7 | cic_b2_en<<6 | samp_delay_leden_num<<4 | samp_copy_avg);

	hx3690l_write_reg(0X2c, phase1_tia_res);
	hx3690l_write_reg(0X2d, phase3_tia_res);
	hx3690l_write_reg(0X2e, phase4_tia_res);
	hx3690l_write_reg(0X2f, phase2_tia_res);

	hx3690l_write_reg(0X30, phase1_ldr_cur);
	hx3690l_write_reg(0X31, phase3_ldr_cur);
	hx3690l_write_reg(0X32, phase4_ldr_cur);
	hx3690l_write_reg(0X33, phase2_ldr_cur);

	hx3690l_write_reg(0X34, (phase1_pd_sel<<4 |  phase1_ldr_sel));
	hx3690l_write_reg(0X35, (phase3_pd_sel<<4 |  phase3_ldr_sel));
	hx3690l_write_reg(0X36, (phase4_pd_sel<<4 |  phase4_ldr_sel));
	hx3690l_write_reg(0X37, (phase2_pd_sel<<4 |  phase2_ldr_sel));

	hx3690l_write_reg(0X38, phase1_offset_idac);
	hx3690l_write_reg(0X39, phase3_offset_idac);
	hx3690l_write_reg(0X3a, phase4_offset_idac);
	hx3690l_write_reg(0X3b, phase2_offset_idac);
	hx3690l_write_reg(0X3c, data_avg_num<<4 | data_avg_num );
	hx3690l_write_reg(0X3d, data_avg_num<<4 | data_avg_num );

	hx3690l_write_reg(0X60, 0x0a);	//1a= adc self test
	hx3690l_write_reg(0X66, 0x92);	//0x92= r2r idac en; 0x91= mos idac en; 0x93= two idac en;
	hx3690l_write_reg(0X67, 0xbf);	//32k osc cfg relate
	hx3690l_write_reg(0X69, 0xa0);	//bit<0>: rc_comb_en bits<1>=rc_rbp_en bits<7>= vcom_clamp_en bits<6:4>= LED_vdesl

	hx3690l_write_reg(0X13, 0x11);  /* bits<7:4> fifo data sel, 0000 = p1-p2;0001= p1,p2;0010=p3,p4;
                                       0011=p1,p2,p3,p4;0100=p3-(p2+p4)/2;0101=p1-p2,p3-p4;0110=p2-(p1+p3)/2;
                                       bits<3:2> fifo int clear mode, 00 = selfclear;01=reserve;10=manual clear;
                                       bits<1:0> fifo mode sel, 00=bypass,01=fifo,10=stream,11=reserve;*/
	hx3690l_write_reg(0X20, 0x03);  // int width
	hx3690l_write_reg(0X23, 0x00);  // phase int sel  80=p1 / 10=p2 / 40=p3 / 20 =p4
	#if defined(INT_MODE)
	hx3690l_write_reg(0X12, 0x20);  // fifo almostfull cfg ,max=0x40;
	hx3690l_write_reg(0X24, 0x20);  // fifo int output sel
	#else
	hx3690l_write_reg(0X12, 0x40);  // fifo almostfull cfg ,max=0x40;
	hx3690l_write_reg(0X24, 0x00);  // fifo int output sel
	#endif

	hx3690l_write_reg(0X18,(phase1_enable<<3)|(phase1_adc_osr)|(phase3_enable<<7)|(phase3_adc_osr<<4) );
	hx3690l_write_reg(0X19,(phase4_enable<<3)|(phase4_adc_osr)|(phase2_enable<<7)|(phase2_adc_osr<<4) );

	hx3690l_write_reg(0X51, 0x02);
	hx3690l_delay_ms(5);
	hx3690l_write_reg(0X13, 0x10);
	hx3690l_delay_ms(5);
	hx3690l_write_reg(0X13, 0x11);	
	hx3690l_write_reg(0X51, 0x00);
	hx3690l_delay_ms(5);
	hx3690l_write_reg(0X6a, 0X02);	 //02= u_low_pow, INT cmos output
}
#endif

#ifdef SPO2_ALG_LIB
// 血氧模式下 初始化配置
void hx3690l_spo2_ppg_init(void) //20200615 ericy ppg fs=25hz, phase3 conversion ready interupt en
{
    uint16_t sample_rate = 25;  /*config the data rate of chip alps2_fm ,uint is Hz*/

    uint32_t prf_clk_num = 32000/sample_rate;   /*period in clk num, num = Fclk/fs */

    uint8_t cic_mode_en =0;
    uint8_t cic_b2_en = 0;
    uint8_t samp_delay_leden_num = 0; /* 00=8,01=16,10=32,11=64*/
    uint8_t samp_copy_avg = 0;        /* 0=1, 1=2, 2=4 ,3=8, 4=16*/
    uint8_t data_avg_num = 0;         /* 0 = 1 ; 1 = 2; 2 =4 ; 3 =8 ; 4 =16 ;*/
    uint8_t phase3_4_internal = 0;    /* phase3 and phase4 prf internal cfg */

    uint8_t phase1_enable = 1;     /*phase1_enable  , 1 mean enable ; 0 mean disable */
    uint8_t phase2_enable = 1;     /*phase2_enable  , 1 mean enable ; 0 mean disable */
    uint8_t phase3_enable = 1;     /*phase3_enable  , 1 mean enable ; 0 mean disable */
    uint8_t phase4_enable = 1;     /*phase4_enable  , 1 mean enable ; 0 mean disable */

    uint8_t phase1_adc_osr = 3;    /* 0 = 128 ; 1 = 256 ; 2 = 512 ; 3 = 1024 ; 4 = 2048*/
    uint8_t phase2_adc_osr = 3;    /* 0 = 128 ; 1 = 256 ; 2 = 512 ; 3 = 1024 ; 4 = 2048*/
    uint8_t phase3_adc_osr = 3;    /* 0 = 128 ; 1 = 256 ; 2 = 512 ; 3 = 1024 ; 4 = 2048*/
    uint8_t phase4_adc_osr = 3;    /* 0 = 128 ; 1 = 256 ; 2 = 512 ; 3 = 1024 ; 4 = 2048*/

    uint8_t phase1_inner_avg = 0;   /* phase1 adc avg num 0=1, 1=2, 2=4, 3=8 , 4=16*/
    uint8_t phase1_tia_res = 0;     /* 0= 10K; 1= 20k; 2= 50k; 3= 100k; 4= 150k; 5= 200k; 6= 500k; 7= 1M*/
    uint8_t phase1_ldr_sel = 0;     /* 1 = ldr0; 2 = ldr1; 4 = ldr2; */
    uint8_t phase1_pd_sel = 1;      /* 1 = pd1; 2 = pd2; */
    uint8_t phase1_offset_idac = 0; /* 0~127 = 0 ~ 32ua , step = 0.25ua */
    uint8_t phase1_ldr_cur = 0;     /* 0~255 = 0 ~ 200ma, step = 0.8ma */
    uint8_t phase1_led_en = 0;      /* phase1 led enable*/

    uint8_t phase2_inner_avg = 0;   /* phase2 adc avg num 0=1, 1=2, 2=4, 3=8 , 4=16*/
    uint8_t phase2_tia_res = 0;     /* 0= 10K; 1= 20k; 2= 50k; 3= 100k; 4= 150k; 5= 200k; 6= 500k; 7= 1M*/
    uint8_t phase2_ldr_sel = GREEN_LED_SLE;     /* 1 = ldr0; 2 = ldr1; 4 = ldr2; */
    uint8_t phase2_pd_sel = 1;      /* 1 = pd1; 2 = pd2; */
    uint8_t phase2_offset_idac = 0; /* 0~127 = 0 ~ 32ua , step = 0.25ua */
    uint8_t phase2_ldr_cur = 0;     /* 0~255 = 0 ~ 200ma, step = 0.8ma */
    uint8_t phase2_led_en = 1;      /* phase2 led enable*/

    uint8_t phase3_inner_avg = 0;   /* phase3 adc avg num 0=1, 1=2, 2=4, 3=8 , 4=16*/
    uint8_t phase3_tia_res = 0;     /* 0= 10K; 1= 20k; 2= 50k; 3= 100k; 4= 150k; 5= 200k; 6= 500k; 7= 1M*/
    uint8_t phase3_ldr_sel = RED_LED_SLE;     /* 1 = ldr0; 2 = ldr1; 4 = ldr2; */
    uint8_t phase3_pd_sel = 1;      /* 1 = pd1; 2 = pd2; */
    uint8_t phase3_offset_idac = 0; /* 0~127 = 0 ~ 32ua , step = 0.25ua */
    uint8_t phase3_ldr_cur = 0;     /* 0~255 = 0 ~ 200ma, step = 0.8ma */
    uint8_t phase3_led_en = 1;      /* phase3 led enable*/

    uint8_t phase4_inner_avg = 0;   /* phase4 adc avg num 0=1, 1=2, 2=4, 3=8 , 4=16*/
    uint8_t phase4_tia_res = 0;     /* 0= 10K; 1= 20k; 2= 50k; 3= 100k; 4= 150k; 5= 200k; 6= 500k; 7= 1M*/
    uint8_t phase4_ldr_sel = IR_LED_SLE;     /* 1 = ldr0; 2 = ldr1; 4 = ldr2; */
    uint8_t phase4_pd_sel = 1;      /* 1 = pd1; 2 = pd2; */
    uint8_t phase4_offset_idac = 0; /* 0~127 = 0 ~ 32ua , step = 0.25ua */
    uint8_t phase4_ldr_cur = 64;    /* 0~255 = 0 ~ 200ma, step = 0.8ma */
    uint8_t phase4_led_en = 1;      /* phase4 led enable*/

    uint8_t init_wait_delay = 5 ; /* 0 = 31clk ; 1 = 64clk ; 2 = 127clk ; 3 = 255clk(d) ;
                                     4 = 511clk; 5 = 1023clk; 6 = 2047; 7 = 2048clk */
    uint8_t afe_reset = 4;        /* 0 = 15clk ; 1 = 31clk ; 2 = 63clk ; 3 = 127clk(d) ;
                                     4 = 255clk; 5 = 511clk; 6 = 1024; 7 = 2048clk */
    uint8_t led_on_time = 3;      /* 0 = 32clk=8us ; 1 = 64clk=16us; 2=128clk=32us ; 3 = 256clk=64us ;
                                     4 = 512clk=128us ; 5 = 1024clk=256us; 6= 2048clk=512us; 7 = 4096clk=1024us */

    hx3690l_write_reg(0x02, 0x31);
    hx3690l_delay_ms(10);
    hx3690l_write_reg(0x02, 0x30);
    hx3690l_delay_ms(5);
    hx3690l_write_reg(0X6a, 0X00);
    hx3690l_delay_ms(5);

    hx3690l_write_reg(0X1a, (uint8_t)prf_clk_num);    // prf bit<7:0>
    hx3690l_write_reg(0X1b, (uint8_t)(prf_clk_num>>8)); // prf bit<15:8>
    hx3690l_write_reg(0X1c, (uint8_t)(prf_clk_num>>16)); // prf bit<23:16>

    hx3690l_write_reg(0X1d, phase3_4_internal);
    hx3690l_write_reg(0X1e, ((afe_reset<<3)| 0x00) );
    hx3690l_write_reg(0X1f, (led_on_time<<4| phase1_led_en<<3 | phase3_led_en<<2 | phase4_led_en<<1 | phase2_led_en) );

    hx3690l_write_reg(0X26, (init_wait_delay<<4 | 0x0f));
    hx3690l_write_reg(0X27, (phase1_inner_avg | (phase2_inner_avg<<4)));
    hx3690l_write_reg(0X28, (phase3_inner_avg | (phase4_inner_avg<<4)));
    hx3690l_write_reg(0X29, cic_mode_en<<7 | cic_b2_en<<6 | samp_delay_leden_num<<4 | samp_copy_avg);

    hx3690l_write_reg(0X2c, phase1_tia_res);
    hx3690l_write_reg(0X2d, phase3_tia_res);
    hx3690l_write_reg(0X2e, phase4_tia_res);
    hx3690l_write_reg(0X2f, phase2_tia_res);

    hx3690l_write_reg(0X30, phase1_ldr_cur);
    hx3690l_write_reg(0X31, phase3_ldr_cur);
    hx3690l_write_reg(0X32, phase4_ldr_cur);
    hx3690l_write_reg(0X33, phase2_ldr_cur);

    hx3690l_write_reg(0X34, (phase1_pd_sel<<4 |  phase1_ldr_sel));
    hx3690l_write_reg(0X35, (phase3_pd_sel<<4 |  phase3_ldr_sel));
    hx3690l_write_reg(0X36, (phase4_pd_sel<<4 |  phase4_ldr_sel));
    hx3690l_write_reg(0X37, (phase2_pd_sel<<4 |  phase2_ldr_sel));

    hx3690l_write_reg(0X38, phase1_offset_idac);
    hx3690l_write_reg(0X39, phase3_offset_idac);
    hx3690l_write_reg(0X3a, phase4_offset_idac);
    hx3690l_write_reg(0X3b, phase2_offset_idac);
    hx3690l_write_reg(0X3c, data_avg_num<<4 | data_avg_num );
    hx3690l_write_reg(0X3d, data_avg_num<<4 | data_avg_num );

    hx3690l_write_reg(0X60, 0x0a);	//1a= adc self test
    hx3690l_write_reg(0X66, 0x92);	//0x92= r2r idac en; 0x91= mos idac en; 0x93= two idac en;
    hx3690l_write_reg(0X67, 0xbf);	//32k osc cfg relate
    hx3690l_write_reg(0X69, 0xa0);	//bit<0>: rc_comb_en bits<1>=rc_rbp_en bits<7>= vcom_clamp_en bits<6:4>= LED_vdesl

    hx3690l_write_reg(0X13, 0x31);  /* bits<7:4> fifo data sel, 0000 = p1-p2;0001= p1,p2;0010=p3,p4;
                                       0011=p1,p2,p3,p4;0100=p3-(p2+p4)/2;0101=p1-p2,p3-p4;0110=p2-(p1+p3)/2;
                                       bits<3:2> fifo int clear mode, 00 = selfclear;01=reserve;10=manual clear;
                                       bits<1:0> fifo mode sel, 00=bypass,01=fifo,10=stream,11=reserve;*/
    hx3690l_write_reg(0X20, 0x03);  // int width
    hx3690l_write_reg(0X23, 0x00);  // phase int sel  80=p1 / 10=p2 / 40=p3 / 20 =p4
    #if defined(INT_MODE)
    hx3690l_write_reg(0X12, 0x20);  // fifo almostfull cfg ,max=0x40;
    hx3690l_write_reg(0X24, 0x20);  // fifo int output sel
    #else
    hx3690l_write_reg(0X12, 0x40);  // fifo almostfull cfg ,max=0x40;
    hx3690l_write_reg(0X24, 0x00);  // fifo int output sel
    #endif

    hx3690l_write_reg(0X18,(phase1_enable<<3)|(phase1_adc_osr)|(phase3_enable<<7)|(phase3_adc_osr<<4) );
    hx3690l_write_reg(0X19,(phase4_enable<<3)|(phase4_adc_osr)|(phase2_enable<<7)|(phase2_adc_osr<<4) );

    hx3690l_write_reg(0X51, 0x02);
    hx3690l_delay_ms(5);
    hx3690l_write_reg(0X13, 0x30);
    hx3690l_delay_ms(5);
    hx3690l_write_reg(0X13, 0x31);
    hx3690l_write_reg(0X51, 0x00);
    hx3690l_delay_ms(5);  
    hx3690l_write_reg(0X6a, 0X02);	//02= u_low_pow, INT cmos output
}
#endif

#ifdef HRV_ALG_LIB
// HRV模式下 初始化配置
void hx3690l_hrv_ppg_init(void) //20200615 ericy ppg fs=25hz, phase3 conversion ready interupt en
{
    uint16_t sample_rate = 125; /*config the data rate of chip alps2_fm ,uint is Hz*/

    uint32_t prf_clk_num = 32000/sample_rate;   /*period in clk num, num = Fclk/fs */

    uint8_t cic_mode_en =0;
    uint8_t cic_b2_en = 0;
    uint8_t samp_delay_leden_num = 0; /* 00=8,01=16,10=32,11=64*/
    uint8_t samp_copy_avg = 0;        /* 0=1, 1=2, 2=4 ,3=8, 4=16*/
    uint8_t data_avg_num = 0;         /* 0 = 1 ; 1 = 2; 2 =4 ; 3 =8 ; 4 =16 ;*/
    uint8_t phase3_4_internal = 0;    /* phase3 and phase4 prf internal cfg */

    uint8_t phase1_enable = 1;     /*phase1_enable  , 1 mean enable ; 0 mean disable */
    uint8_t phase2_enable = 0;     /*phase2_enable  , 1 mean enable ; 0 mean disable */
    uint8_t phase3_enable = 1;     /*phase3_enable  , 1 mean enable ; 0 mean disable */
    uint8_t phase4_enable = 1;     /*phase4_enable  , 1 mean enable ; 0 mean disable */

    uint8_t phase1_adc_osr = 3;    /* 0 = 128 ; 1 = 256 ; 2 = 512 ; 3 = 1024 ; 4 = 2048*/
    uint8_t phase2_adc_osr = 0;    /* 0 = 128 ; 1 = 256 ; 2 = 512 ; 3 = 1024 ; 4 = 2048*/
    uint8_t phase3_adc_osr = 3;    /* 0 = 128 ; 1 = 256 ; 2 = 512 ; 3 = 1024 ; 4 = 2048*/
    uint8_t phase4_adc_osr = 3;    /* 0 = 128 ; 1 = 256 ; 2 = 512 ; 3 = 1024 ; 4 = 2048*/
	
    uint8_t phase1_inner_avg = 0;   /* phase1 adc avg num 0=1, 1=2, 2=4, 3=8 , 4=16*/
    uint8_t phase1_tia_res = 0;     /* 0= 10K; 1= 20k; 2= 50k; 3= 100k; 4= 150k; 5= 200k; 6= 500k; 7= 1M*/
    uint8_t phase1_ldr_sel = GREEN_LED_SLE;     /* 1 = ldr0; 2 = ldr1; 4 = ldr2; */
    uint8_t phase1_pd_sel = 1;      /* 1 = pd1; 2 = pd2; */
    uint8_t phase1_offset_idac = 0; /* 0~127 = 0 ~ 32ua , step = 0.25ua */
    uint8_t phase1_ldr_cur = 0;     /* 0~255 = 0 ~ 200ma, step = 0.8ma */
    uint8_t phase1_led_en = 1;      /* phase1 led enable*/

    uint8_t phase2_inner_avg = 0;   /* phase2 adc avg num 0=1, 1=2, 2=4, 3=8 , 4=16*/
    uint8_t phase2_tia_res = 0;     /* 0= 10K; 1= 20k; 2= 50k; 3= 100k; 4= 150k; 5= 200k; 6= 500k; 7= 1M*/
    uint8_t phase2_ldr_sel = 0;     /* 1 = ldr0; 2 = ldr1; 4 = ldr2; */
    uint8_t phase2_pd_sel = 0;      /* 1 = pd1; 2 = pd2; 4 = pd3; */
    uint8_t phase2_offset_idac = 0; /* 0~127 = 0 ~ 32ua , step = 0.25ua */
    uint8_t phase2_ldr_cur = 0;     /* 0~255 = 0 ~ 200ma, step = 0.8ma */
    uint8_t phase2_led_en = 0;      /* phase2 led enable*/

    uint8_t phase3_inner_avg = 0;   /* phase3 adc avg num 0=1, 1=2, 2=4, 3=8 , 4=16*/
    uint8_t phase3_tia_res = 0;     /* 0= 10K; 1= 20k; 2= 50k; 3= 100k; 4= 150k; 5= 200k; 6= 500k; 7= 1M*/
    uint8_t phase3_ldr_sel = IR_LED_SLE;     /* 1 = ldr0; 2 = ldr1; 4 = ldr2; */
    uint8_t phase3_pd_sel = 1;      /* 1 = pd1; 2 = pd2; 4 = pd3; */
    uint8_t phase3_offset_idac = 0; /* 0~127 = 0 ~ 32ua , step = 0.25ua */
    uint8_t phase3_ldr_cur = 64;    /* 0~255 = 0 ~ 200ma, step = 0.8ma */
    uint8_t phase3_led_en = 1;      /* phase3 led enable*/

    uint8_t phase4_inner_avg = 0;   /* phase4 adc avg num 0=1, 1=2, 2=4, 3=8 , 4=16*/
    uint8_t phase4_tia_res = 0;     /* 0= 10K; 1= 20k; 2= 50k; 3= 100k; 4= 150k; 5= 200k; 6= 500k; 7= 1M*/
    uint8_t phase4_ldr_sel = 0;     /* 1 = ldr0; 2 = ldr1; 4 = ldr2; */
    uint8_t phase4_pd_sel = 1;      /* 1 = pd1; 2 = pd2; 4 = pd3; */
    uint8_t phase4_offset_idac = 0; /* 0~127 = 0 ~ 32ua , step = 0.25ua */
    uint8_t phase4_ldr_cur = 0;     /* 0~255 = 0 ~ 200ma, step = 0.8ma */
    uint8_t phase4_led_en = 0;      /* phase4 led enable*/

    uint8_t init_wait_delay = 5 ; /* 0 = 31clk ; 1 = 64clk ; 2 = 127clk ; 3 = 255clk(d) ;
                                     4 = 511clk; 5 = 1023clk; 6 = 2047; 7 = 2048clk */

    uint8_t afe_reset = 3;        /* 0 = 15clk ; 1 = 31clk ; 2 = 63clk ; 3 = 127clk(d) ;
                                     4 = 255clk; 5 = 511clk; 6 = 1024; 7 = 2048clk */

    uint8_t led_on_time = 3;      /* 0 = 32clk=8us ; 1 = 64clk=16us; 2=128clk=32us ; 3 = 256clk=64us ;
                                     4 = 512clk=128us ; 5 = 1024clk=256us; 6= 2048clk=512us; 7 = 4096clk=1024us */
    hx3690l_write_reg(0x02, 0x31);
    hx3690l_delay_ms(10);
    hx3690l_write_reg(0x02, 0x30);
    hx3690l_delay_ms(5);
    hx3690l_write_reg(0X6a, 0X00);
    hx3690l_delay_ms(5);
    
    hx3690l_write_reg(0X1a, (uint8_t)prf_clk_num);    // prf bit<7:0>
    hx3690l_write_reg(0X1b, (uint8_t)(prf_clk_num>>8)); // prf bit<15:8>
    hx3690l_write_reg(0X1c, (uint8_t)(prf_clk_num>>16)); // prf bit<23:16>

    hx3690l_write_reg(0X1d, phase3_4_internal);
    hx3690l_write_reg(0X1e, ((afe_reset<<3)| 0x00) );
    hx3690l_write_reg(0X1f, (led_on_time<<4| phase1_led_en<<3 | phase3_led_en<<2 | phase4_led_en<<1 | phase2_led_en) );

    hx3690l_write_reg(0X26, (init_wait_delay<<4 | 0x0f));
    hx3690l_write_reg(0X27, (phase1_inner_avg | (phase2_inner_avg<<4)));
    hx3690l_write_reg(0X28, (phase3_inner_avg | (phase4_inner_avg<<4)));
    hx3690l_write_reg(0X29, cic_mode_en<<7 | cic_b2_en<<6 | samp_delay_leden_num<<4 | samp_copy_avg);

    hx3690l_write_reg(0X2c, phase1_tia_res);
    hx3690l_write_reg(0X2d, phase3_tia_res);
    hx3690l_write_reg(0X2e, phase4_tia_res);
    hx3690l_write_reg(0X2f, phase2_tia_res);

    hx3690l_write_reg(0X30, phase1_ldr_cur);
    hx3690l_write_reg(0X31, phase3_ldr_cur);
    hx3690l_write_reg(0X32, phase4_ldr_cur);
    hx3690l_write_reg(0X33, phase2_ldr_cur);

    hx3690l_write_reg(0X34, (phase1_pd_sel<<4 |  phase1_ldr_sel));
    hx3690l_write_reg(0X35, (phase3_pd_sel<<4 |  phase3_ldr_sel));
    hx3690l_write_reg(0X36, (phase4_pd_sel<<4 |  phase4_ldr_sel));
    hx3690l_write_reg(0X37, (phase2_pd_sel<<4 |  phase2_ldr_sel));

    hx3690l_write_reg(0X38, phase1_offset_idac);
    hx3690l_write_reg(0X39, phase3_offset_idac);
    hx3690l_write_reg(0X3a, phase4_offset_idac);
    hx3690l_write_reg(0X3b, phase2_offset_idac);
    hx3690l_write_reg(0X3c, data_avg_num<<4 | data_avg_num );
    hx3690l_write_reg(0X3d, data_avg_num<<4 | data_avg_num );

    hx3690l_write_reg(0X60, 0x0a);	//1a= adc self test
    hx3690l_write_reg(0X66, 0x92);	//0x92= r2r idac en; 0x91= mos idac en; 0x93= two idac en;
    hx3690l_write_reg(0X67, 0xbf);	//32k osc cfg relate
    hx3690l_write_reg(0X69, 0xa0);	//bit<0>: rc_comb_en bits<1>=rc_rbp_en bits<7>= vcom_clamp_en bits<6:4>= LED_vdesl

    hx3690l_write_reg(0X13, 0x11);  /* bits<7:4> fifo data sel, 0000 = p1-p2;0001= p1,p2;0010=p3,p4;
                                       0011=p1,p2,p3,p4;0100=p3-(p2+p4)/2;0101=p1-p2,p3-p4;0110=p2-(p1+p3)/2;
                                       bits<3:2> fifo int clear mode, 00 = selfclear;01=reserve;10=manual clear;
                                       bits<1:0> fifo mode sel, 00=bypass,01=fifo,10=stream,11=reserve;*/
    hx3690l_write_reg(0X20, 0x03);  // int width
    hx3690l_write_reg(0X23, 0x00);  // phase int sel  80=p1 / 10=p2 / 40=p3 / 20 =p4
    #if defined(INT_MODE)
    hx3690l_write_reg(0X12, 0x28);  // fifo almostfull cfg ,max=0x40;
    hx3690l_write_reg(0X24, 0x20);  // fifo int output sel
    #else
    hx3690l_write_reg(0X12, 0x40);  // fifo almostfull cfg ,max=0x40;
    hx3690l_write_reg(0X24, 0x00);  // fifo int output sel
    #endif

    hx3690l_write_reg(0X18,(phase1_enable<<3)|(phase1_adc_osr)|(phase3_enable<<7)|(phase3_adc_osr<<4) );
    hx3690l_write_reg(0X19,(phase4_enable<<3)|(phase4_adc_osr)|(phase2_enable<<7)|(phase2_adc_osr<<4) );

    hx3690l_write_reg(0X51, 0x02);
    hx3690l_delay_ms(5);
    hx3690l_write_reg(0X13, 0x10);
    hx3690l_delay_ms(5);
    hx3690l_write_reg(0X13, 0x11);	
    hx3690l_write_reg(0X51, 0x00);
    hx3690l_delay_ms(5);
    hx3690l_write_reg(0X6a, 0X02);	//02= u_low_pow, INT cmos output
}
#endif

#ifdef CHECK_TOUCH_LIB
void hx3690l_check_touch_init(void) //20200615 ericy ppg fs=25hz, phase3 conversion ready interupt en
{
    uint16_t sample_rate = 10;  /*config the data rate of chip alps2_fm ,uint is Hz*/

    uint32_t prf_clk_num = 32000/sample_rate;   /*period in clk num, num = Fclk/fs */

    uint8_t cic_mode_en =0;
    uint8_t cic_b2_en = 0;
    uint8_t samp_delay_leden_num = 0; /* 00=8,01=16,10=32,11=64*/
    uint8_t samp_copy_avg = 0;        /* 0=1, 1=2, 2=4 ,3=8, 4=16*/
    uint8_t data_avg_num = 0;         /* 0 = 1 ; 1 = 2; 2 =4 ; 3 =8 ; 4 =16 ;*/
    uint8_t phase3_4_internal = 0;    /* phase3 and phase4 prf internal cfg */

    uint8_t phase1_enable = 1;     /*phase1_enable  , 1 mean enable ; 0 mean disable */
    uint8_t phase2_enable = 1;     /*phase2_enable  , 1 mean enable ; 0 mean disable */
    uint8_t phase3_enable = 0;     /*phase3_enable  , 1 mean enable ; 0 mean disable */
    uint8_t phase4_enable = 0;     /*phase4_enable  , 1 mean enable ; 0 mean disable */

    uint8_t phase1_adc_osr = 3;    /* 0 = 128 ; 1 = 256 ; 2 = 512 ; 3 = 1024 ; 4 = 2048*/
    uint8_t phase2_adc_osr = 3;    /* 0 = 128 ; 1 = 256 ; 2 = 512 ; 3 = 1024 ; 4 = 2048*/
    uint8_t phase3_adc_osr = 3;    /* 0 = 128 ; 1 = 256 ; 2 = 512 ; 3 = 1024 ; 4 = 2048*/
    uint8_t phase4_adc_osr = 3;    /* 0 = 128 ; 1 = 256 ; 2 = 512 ; 3 = 1024 ; 4 = 2048*/

    uint8_t phase1_inner_avg = 0;   /* phase1 adc avg num 0=1, 1=2, 2=4, 3=8 , 4=16*/
    uint8_t phase1_tia_res = 0;     /* 0= 10K; 1= 20k; 2= 50k; 3= 100k; 4= 150k; 5= 200k; 6= 500k; 7= 1M*/
    uint8_t phase1_ldr_sel = IR_LED_SLE;     /* 1 = ldr0; 2 = ldr1; 4 = ldr2; */
    uint8_t phase1_pd_sel = 1;      /* 1 = pd1; 2 = pd2; */
    uint8_t phase1_offset_idac = 0; /* 0~127 = 0 ~ 32ua , step = 0.25ua */
    uint8_t phase1_ldr_cur = 64;    /* 0~255 = 0 ~ 200ma, step = 0.8ma */
    uint8_t phase1_led_en = 1;      /* phase1 led enable*/

    uint8_t phase2_inner_avg = 0;   /* phase2 adc avg num 0=1, 1=2, 2=4, 3=8 , 4=16*/
    uint8_t phase2_tia_res = 0;     /* 0= 10K; 1= 20k; 2= 50k; 3= 100k; 4= 150k; 5= 200k; 6= 500k; 7= 1M*/
    uint8_t phase2_ldr_sel = 0;     /* 1 = ldr0; 2 = ldr1; 4 = ldr2; */
    uint8_t phase2_pd_sel = 1;      /* 1 = pd1; 2 = pd2; 4 = pd3; */
    uint8_t phase2_offset_idac = 0; /* 0~127 = 0 ~ 32ua , step = 0.25ua */
    uint8_t phase2_ldr_cur = 0;     /* 0~255 = 0 ~ 200ma, step = 0.8ma */
    uint8_t phase2_led_en = 0;      /* phase2 led enable*/

    uint8_t phase3_inner_avg = 0;   /* phase3 adc avg num 0=1, 1=2, 2=4, 3=8 , 4=16*/
    uint8_t phase3_tia_res = 0;     /* 0= 10K; 1= 20k; 2= 50k; 3= 100k; 4= 150k; 5= 200k; 6= 500k; 7= 1M*/
    uint8_t phase3_ldr_sel = 0;     /* 1 = ldr0; 2 = ldr1; 4 = ldr2; */
    uint8_t phase3_pd_sel = 0;      /* 1 = pd1; 2 = pd2; 4 = pd3; */
    uint8_t phase3_offset_idac = 0; /* 0~127 = 0 ~ 32ua , step = 0.25ua */
    uint8_t phase3_ldr_cur = 0;     /* 0~255 = 0 ~ 200ma, step = 0.8ma */
    uint8_t phase3_led_en = 0;      /* phase3 led enable*/

    uint8_t phase4_inner_avg = 0;   /* phase4 adc avg num 0=1, 1=2, 2=4, 3=8 , 4=16*/
    uint8_t phase4_tia_res = 0;     /* 0= 10K; 1= 20k; 2= 50k; 3= 100k; 4= 150k; 5= 200k; 6= 500k; 7= 1M*/
    uint8_t phase4_ldr_sel = 0;     /* 1 = ldr0; 2 = ldr1; 4 = ldr2; */
    uint8_t phase4_pd_sel = 0;      /* 1 = pd1; 2 = pd2; 4 = pd3; */
    uint8_t phase4_offset_idac = 0; /* 0~127 = 0 ~ 32ua , step = 0.25ua */
    uint8_t phase4_ldr_cur = 0;     /* 0~255 = 0 ~ 200ma, step = 0.8ma */
    uint8_t phase4_led_en = 0;      /* phase4 led enable*/

    uint8_t init_wait_delay = 5 ; /* 0 = 31clk ; 1 = 64clk ; 2 = 127clk ; 3 = 255clk(d) ;
                                     4 = 511clk; 5 = 1023clk; 6 = 2047; 7 = 2048clk */

    uint8_t afe_reset = 3;        /* 0 = 15clk ; 1 = 31clk ; 2 = 63clk ; 3 = 127clk(d) ;
                                     4 = 255clk; 5 = 511clk; 6 = 1024; 7 = 2048clk */

    uint8_t led_on_time = 1;      /* 0 = 32clk=8us ; 1 = 64clk=16us; 2=128clk=32us ; 3 = 256clk=64us ;
                                     4 = 512clk=128us ; 5 = 1024clk=256us; 6= 2048clk=512us; 7 = 4096clk=1024us */
    
    hx3690l_write_reg(0x02, 0x31);
    hx3690l_delay_ms(10);
    hx3690l_write_reg(0x02, 0x30);
    hx3690l_delay_ms(5);
    hx3690l_write_reg(0X6a, 0X00);
    hx3690l_delay_ms(5);
    
    hx3690l_write_reg(0X1a, (uint8_t)prf_clk_num);    // prf bit<7:0>
    hx3690l_write_reg(0X1b, (uint8_t)(prf_clk_num>>8)); // prf bit<15:8>
    hx3690l_write_reg(0X1c, (uint8_t)(prf_clk_num>>16)); // prf bit<23:16>

    hx3690l_write_reg(0X1d, phase3_4_internal);
    hx3690l_write_reg(0X1e, ((afe_reset<<3)| 0x00) );
    hx3690l_write_reg(0X1f, (led_on_time<<4| phase1_led_en<<3 | phase3_led_en<<2 | phase4_led_en<<1 | phase2_led_en) );

    hx3690l_write_reg(0X26, (init_wait_delay<<4 | 0x0f));
    hx3690l_write_reg(0X27, (phase1_inner_avg | (phase2_inner_avg<<4)));
    hx3690l_write_reg(0X28, (phase3_inner_avg | (phase4_inner_avg<<4)));
    hx3690l_write_reg(0X29, cic_mode_en<<7 | cic_b2_en<<6 | samp_delay_leden_num<<4 | samp_copy_avg);

    hx3690l_write_reg(0X2c, phase1_tia_res);
    hx3690l_write_reg(0X2d, phase3_tia_res);
    hx3690l_write_reg(0X2e, phase4_tia_res);
    hx3690l_write_reg(0X2f, phase2_tia_res);

    hx3690l_write_reg(0X30, phase1_ldr_cur);
    hx3690l_write_reg(0X31, phase3_ldr_cur);
    hx3690l_write_reg(0X32, phase4_ldr_cur);
    hx3690l_write_reg(0X33, phase2_ldr_cur);

    hx3690l_write_reg(0X34, (phase1_pd_sel<<4 |  phase1_ldr_sel));
    hx3690l_write_reg(0X35, (phase3_pd_sel<<4 |  phase3_ldr_sel));
    hx3690l_write_reg(0X36, (phase4_pd_sel<<4 |  phase4_ldr_sel));
    hx3690l_write_reg(0X37, (phase2_pd_sel<<4 |  phase2_ldr_sel));

    hx3690l_write_reg(0X38, phase1_offset_idac);
    hx3690l_write_reg(0X39, phase3_offset_idac);
    hx3690l_write_reg(0X3a, phase4_offset_idac);
    hx3690l_write_reg(0X3b, phase2_offset_idac);
    hx3690l_write_reg(0X3c, data_avg_num<<4 | data_avg_num );
    hx3690l_write_reg(0X3d, data_avg_num<<4 | data_avg_num );
		
    hx3690l_write_reg(0X18,(phase1_enable<<3)|(phase1_adc_osr)|(phase3_enable<<7)|(phase3_adc_osr<<4) );
    hx3690l_write_reg(0X19,(phase4_enable<<3)|(phase4_adc_osr)|(phase2_enable<<7)|(phase2_adc_osr<<4) );

    hx3690l_write_reg(0X60, 0x0a);	//1a= adc self test
    hx3690l_write_reg(0X66, 0x92);	//0x92= r2r idac en; 0x91= mos idac en; 0x93= two idac en;
    hx3690l_write_reg(0X67, 0xbf);	//32k osc cfg relate
    hx3690l_write_reg(0X69, 0xa2);	//bit<0>: rc_comb_en bits<1>=rc_rbp_en bits<7>= vcom_clamp_en bits<6:4>= LED_vdesl

    hx3690l_write_reg(0X13, 0x11); 
    hx3690l_write_reg(0X20, 0x03);  // int width
    hx3690l_write_reg(0X23, 0x00);  // phase int sel  80=p1 / 10=p2 / 40=p3 / 20 =p4
		
    #if defined(INT_MODE)
    hx3690l_write_reg(0X12, 0x0b);  // fifo almostfull cfg ,max=0x40;
    hx3690l_write_reg(0X24, 0x20);  // fifo int output sel
    #else
    hx3690l_write_reg(0X12, 0x40);  // fifo almostfull cfg ,max=0x40;
    hx3690l_write_reg(0X24, 0x00);  // fifo int output sel
    #endif

    hx3690l_write_reg(0X51, 0x02);
    hx3690l_delay_ms(5);
    hx3690l_write_reg(0X13, 0x10);
    hx3690l_delay_ms(5);
    hx3690l_write_reg(0X13, 0x11);	
    hx3690l_write_reg(0X51, 0x00);
    hx3690l_delay_ms(5);
    hx3690l_write_reg(0X6a, 0X02);	//02= u_low_pow, INT cmos output		
}
#endif

// 初始化调用入口函数
bool hx3690l_init(WORK_MODE_T mode)
{
	work_mode_flag = mode;
	hx3690l_data_reset();
	hx3690l_vin_check(3800);               
	switch (work_mode_flag)
	{
		case HRS_MODE:
			#ifdef HRS_ALG_LIB
			if(hx3690l_hrs_enable()== SENSOR_OP_FAILED) 
			{
				return false;
			}
			#endif
			break;
				
		case LIVING_MODE:
			#ifdef CHECK_LIVING_LIB
			if(hx3690l_hrs_enable()== SENSOR_OP_FAILED) 
			{
				return false;
			}
			#endif
			break;
				
		case SPO2_MODE:
			#ifdef SPO2_DATA_CALI
			hx3690_spo2_data_cali_init();
			#endif
			
			#ifdef SPO2_ALG_LIB
			tyhx_spo2_para_usuallyadjust(SPO2_LOW_XCORR_THRE,SPO2_LOW_SNR_THRE,COUNT_BLOCK_NUM,SPO2_BASE_LINE_INIT,SPO2_SLOPE,SPO2_GSEN_POW_THRE);
            tyhx_spo2_para_barelychange(MEAN_NUM,SOP2_DEGLITCH_THRE,SPO2_REMOVE_JUMP_THRE, \
                                        SPO2_LOW_CLIP_END_TIME,SPO2_LOW_CLIP_DN,SPO2_NORMAL_CLIP_DN, \
                                        IR_AC_TOUCH_THRE,IR_FFT_POW_THRE,SPO2_CALI,SLOPE_PARA_MAX,SLOPE_PARA_MIN);
			if(hx3690l_spo2_enable()== SENSOR_OP_FAILED) 
			{
				return false;
			}
			#endif
			break;
			
		case HRSPO2_MODE:
			#if defined(HRS_ALG_LIB) && defined (SPO2_ALG_LIB)
			tyhx_spo2_para_usuallyadjust(SPO2_LOW_XCORR_THRE,SPO2_LOW_SNR_THRE,COUNT_BLOCK_NUM,SPO2_BASE_LINE_INIT,SPO2_SLOPE,SPO2_GSEN_POW_THRE);
            tyhx_spo2_para_barelychange(MEAN_NUM,SOP2_DEGLITCH_THRE,SPO2_REMOVE_JUMP_THRE, \
                                        SPO2_LOW_CLIP_END_TIME,SPO2_LOW_CLIP_DN,SPO2_NORMAL_CLIP_DN, \
                                        IR_AC_TOUCH_THRE,IR_FFT_POW_THRE,SPO2_CALI,SLOPE_PARA_MAX,SLOPE_PARA_MIN);
			if(hx3690l_spo2_enable()== SENSOR_OP_FAILED) 
			{
				return false;
			}
			#endif
				
		case HRV_MODE:
			#ifdef HRV_ALG_LIB
			if(hx3690l_hrv_enable()== SENSOR_OP_FAILED) 
			{
				return false;
			}
			#endif
			break;
				
		case WEAR_MODE:
			#ifdef CHECK_TOUCH_LIB
			if(hx3690l_check_touch_enable()== SENSOR_OP_FAILED) 
			{
				return false;
			}
			#endif
			break;
				
		case FT_LEAK_LIGHT_MODE:
			hx3690l_factroy_test(LEAK_LIGHT_TEST);
			break;
			
		case FT_GRAY_CARD_MODE:
			hx3690l_factroy_test(GRAY_CARD_TEST);
			break;
        
        case FT_INT_TEST_MODE:
			hx3690l_factroy_test(FT_INT_TEST);
			break;
			
		case FT_SINGLE_CHECK_MODE:
			hx3690l_factroy_test(SINGLE_CHECK_TEST);
			break;
        
        case FT_LED_OFF_MODE:
			hx3690l_factroy_test(LED_OFF_TEST);
			break;
			
		case FT_WEAR_MODE:
			hx3690l_factroy_test(WEAR_MODE_TEST);
			break;
			
		case LAB_TEST_MODE:
			#ifdef LAB_TEST
			if(hx3690l_lab_test_enable()== SENSOR_OP_FAILED) 
			{
				return false;
			}
			#endif
			break;
			
		default:
			break;
	}
  return true;
}

void hx3690l_agc_Int_handle(void)
{      
	switch (work_mode_flag)
	{
		case HRS_MODE:
			#ifdef HRS_ALG_LIB
			PPG_hrs_agc();					
			#endif
			break;
        
		case LIVING_MODE:
			#ifdef HRS_ALG_LIB
			PPG_hrs_agc();					
			#endif
			break;
        
		case HRV_MODE:
			#ifdef HRV_ALG_LIB
			PPG_hrv_agc();					
			#endif
			break;

		case SPO2_MODE:
			#ifdef SPO2_ALG_LIB 
			PPG_spo2_agc();					
			#endif
			break;

        case HRSPO2_MODE:
            #if defined(HRS_ALG_LIB) && defined (SPO2_ALG_LIB)
			PPG_spo2_agc();
            #endif
        
        case LAB_TEST_MODE:
            #ifdef LAB_TEST
            hx3690l_lab_test_Int_handle();
            #endif

		default:
			break;
	}	
}

void gsen_read_timeout_handler(void * p_context)
{
		hx3690l_agc_Int_handle();
		
		#if defined(GSEN_40MS_TIMMER) && defined(TYHX_DEMO)
		hx3690l_gesensor_Int_handle();
		#endif	
}

void heart_rate_meas_timeout_handler(void * p_context)
{ 
	switch(work_mode_flag)
	{
		case HRS_MODE:
			#ifdef HRS_ALG_LIB
			hx3690l_hrs_ppg_Int_handle();
			#endif
			break;
		
		case HRV_MODE:
			#ifdef HRV_ALG_LIB
			hx3690l_hrv_ppg_Int_handle();
			#endif
			break;
		
		case SPO2_MODE:
			#ifdef SPO2_ALG_LIB
			hx3690l_spo2_ppg_Int_handle();
			#endif
			break;
		
		case WEAR_MODE:
			#ifdef CHECK_TOUCH_LIB
			hx3690l_wear_ppg_Int_handle();
			#endif
			break;
		
		case LIVING_MODE:
			#ifdef CHECK_LIVING_LIB
			hx3690l_living_Int_handle();
			#endif
			break;
        
        case HRSPO2_MODE:
            #if defined(HRS_ALG_LIB) && defined (SPO2_ALG_LIB)
            hx3690l_hrspo2_ppg_Int_handle();
            #endif
		
		default:
			break;
	}
}

void hx3690l_ppg_Int_handle(void)
{
    hx3690l_agc_Int_handle();
    #if defined(INT_MODE)
    switch(work_mode_flag)
    {
        case HRS_MODE:
            #ifdef HRS_ALG_LIB
            hx3690l_hrs_ppg_Int_handle();
            #endif
            break;
        
        case SPO2_MODE:
            #ifdef SPO2_ALG_LIB
            hx3690l_spo2_ppg_Int_handle();
            #endif
            break;
        
        case HRV_MODE:
            #ifdef HRV_ALG_LIB
            hx3690l_hrv_ppg_Int_handle();
            #endif
            break;
                
        case WEAR_MODE:
            #ifdef CHECK_TOUCH_LIB
            hx3690l_wear_ppg_Int_handle();
            #endif
            break;
        
        case LAB_TEST_MODE:
            #ifdef LAB_TEST
            hx3690l_lab_test_Int_handle();
            #endif
        
        default:
            break;
    } 
    #endif
}

#ifdef TYHX_DEMO
void hx3690l_gesensor_Int_handle(void)
{
    uint8_t ii = 0;
    AxesRaw_t gsen_buf;
    if(work_mode_flag == WEAR_MODE)
    {
        return;
    }
    
    LIS3DH_GetAccAxesRaw(&gsen_buf);

    for(ii=0;ii<31;ii++)
    {
        gsen_fifo_x[ii] = gsen_fifo_x[ii+1];
        gsen_fifo_y[ii] = gsen_fifo_y[ii+1];
        gsen_fifo_z[ii] = gsen_fifo_z[ii+1];
    }
    gsen_fifo_x[31] = gsen_buf.AXIS_X>>1;
    gsen_fifo_y[31] = gsen_buf.AXIS_Y>>1;
    gsen_fifo_z[31] = gsen_buf.AXIS_Z>>1;
}
#endif

#ifdef HRS_ALG_LIB
/*TYHX 心率模式下 读取PPG， 默认320ms*/
void hx3690l_hrs_ppg_Int_handle(void)
{
	uint8_t     ii=0;   
	hrs_results_t   alg_results = {MSG_HRS_ALG_NOT_OPEN,0,0,0,0};
	hrs_wear_msg_code_t     hrs_wear_status = MSG_HRS_NO_WEAR;
	int32_t     *PPG_buf = &(ppg_s_data.green_data[0]);
//	int32_t 	*ir_buf = &(ppg_s_data.ir_data[0]);
	uint8_t 	*count = &(ppg_s_data.count);
	//int32_t 	*s_buf = &(ppg_s_data.s_buf[0]);
	int16_t 	gsen_fifo_x_send[32]={0};
	int16_t 	gsen_fifo_y_send[32]={0};
	int16_t 	gsen_fifo_z_send[32]={0};  

    #ifdef HR_VECTOR
	for(ii=0;ii<8;ii++)
	{
		PPG_buf_vec[ii] = hrm_input_data[vec_send_cnt+ii];
		gsen_fifo_x_send[ii] = gsen_input_data_x[vec_send_cnt+ii];
		gsen_fifo_y_send[ii] = gsen_input_data_y[vec_send_cnt+ii];
		gsen_fifo_z_send[ii] = gsen_input_data_z[vec_send_cnt+ii];
	}
	vec_send_cnt = vec_send_cnt+8;
	*count = 8;
	tyhx_hrs_alg_send_data(PPG_buf_vec,*count, gsen_fifo_x_send, gsen_fifo_y_send, gsen_fifo_z_send); 
    
    #else
	if(hx3690l_hrs_read(&ppg_s_data) == 0)
	{ 
		return;
	}
	for(ii=0;ii<*count;ii++)
	{
        #ifdef TYHX_DEMO  //需要客户填写，将最新的COUNT组GSEN数据分别传入这3个数组
		gsen_fifo_x_send[ii] = gsen_fifo_x[32-*count+ii];
		gsen_fifo_y_send[ii] = gsen_fifo_y[32-*count+ii];
		gsen_fifo_z_send[ii] = gsen_fifo_z[32-*count+ii];
        #endif
		//DEBUG_PRINTF("%d/%d %d %d %d %d %d %d\r\n" ,1+ii,*count,PPG_buf[ii],ir_buf[0],gsen_fifo_x[ii],gsen_fifo_y[ii],gsen_fifo_z[ii],ppg_s_data.green_cur);
	}
    
	hrs_wear_status = hx3690_hrs_get_wear_status();
	if(hrs_wear_status == MSG_HRS_WEAR)
	{
		tyhx_hrs_alg_send_data(PPG_buf, *count, gsen_fifo_x_send, gsen_fifo_y_send, gsen_fifo_z_send);   
	}
    #endif      
                                                                                                               
    alg_results = tyhx_hrs_alg_get_results();  
	//DEBUG_PRINTF("data_cnt: %d,  hr_result:%d \r\n" ,alg_results.data_cnt, alg_results.hr_result);
    hr_os_api.hb_handler(alg_results.hr_result, 0, 0);
    printk("tyhx_hrs_alg_get_results %d \r\n",alg_results.hr_result);
	#ifdef BP_CUSTDOWN_ALG_LIB        
	//bp_results_t    bp_alg_results;    
    //bp_alg_results = tyhx_alg_get_bp_results();
	#endif 

////////////////////////////////////////BLE+DISPLAY/////////////////////////////////	
    #ifdef TYHX_DEMO	
        oled_dis.refresh_time++;
        if(oled_dis.refresh_time >= 5) //320ms*5 = 1600ms
        {
            oled_dis.refresh_flag = 1;
            oled_dis.refresh_time = 0;
            oled_dis.dis_mode = DIS_HR;         
            oled_dis.dis_data1 = alg_results.hr_result;
            oled_dis.dis_data2 = alg_results.cal_result;
        }

        #ifdef HRS_BLE_APP
        {
            rawdata_vector_t rawdata;
            HRS_CAL_SET_T cal;
            cal = get_hrs_agc_status();
            for(ii=0;ii<*count;ii++)
            {
                rawdata.vector_flag = HRS_VECTOR_FLAG;
                rawdata.data_cnt = alg_results.data_cnt-*count+ii;
                rawdata.hr_result = alg_results.hr_result;           
                rawdata.red_raw_data = PPG_buf[ii];
                rawdata.ir_raw_data = ir_buf[ii];
                rawdata.gsensor_x = gsen_fifo_x_send[ii];
                rawdata.gsensor_y = gsen_fifo_y_send[ii];
                rawdata.gsensor_z = gsen_fifo_z_send[ii];
                rawdata.red_cur = cal.LED;
                rawdata.ir_cur = alg_results.hrs_alg_status;

                ble_rawdata_vector_push(rawdata);   
            }
        }
        #endif 
    #endif    
////////////////////////////////////////////////////////////////////////////////////		
}
#endif

#ifdef CHECK_LIVING_LIB
void hx3690l_living_Int_handle(void)
{
	uint8_t     ii=0;   
	hx3690_living_results_t     living_alg_results = {MSG_LIVING_NO_WEAR,0,0,0};
	int32_t     *PPG_buf = &(ppg_s_data.green_data[0]);
    int32_t     *ir_buf = &(ppg_s_data.ir_data[0]);
	uint8_t     *count = &(ppg_s_data.count);
    hrs_wear_msg_code_t     hrs_wear_status = MSG_HRS_NO_WEAR;
    int16_t 	gsen_fifo_x_send[32]={0};
	int16_t 	gsen_fifo_y_send[32]={0};
	int16_t 	gsen_fifo_z_send[32]={0};

	if(hx3690l_hrs_read(&ppg_s_data) == 0)
	{			
		return;
	}
	for(ii=0;ii<*count;ii++)
	{
        #ifdef TYHX_DEMO  //需要客户填写，将最新的COUNT组GSEN数据分别传入这3个数组
		gsen_fifo_x_send[ii] = gsen_fifo_x[32-*count+ii];
		gsen_fifo_y_send[ii] = gsen_fifo_y[32-*count+ii];
		gsen_fifo_z_send[ii] = gsen_fifo_z[32-*count+ii];
        #endif
        //DEBUG_PRINTF("%d/%d %d %d %d %d %d %d\r\n" ,1+ii,*count,PPG_buf[ii],ir_buf[0],gsen_fifo_x[ii],gsen_fifo_y[ii],gsen_fifo_z[ii],ppg_s_data.green_cur);
	}
	hrs_wear_status = hx3690_hrs_get_wear_status();
	if(hrs_wear_status == MSG_HRS_WEAR)
	{
        hx3690_living_send_data(PPG_buf, *count, gsen_fifo_x_send, gsen_fifo_y_send, gsen_fifo_z_send);   
    }
	living_alg_results = hx3690_living_get_results(); 
	DEBUG_PRINTF("%d %d %d %d\r\n" ,living_alg_results.data_cnt,living_alg_results.motion_status,living_alg_results.signal_quality,hrs_wear_status);
}
#endif


#ifdef SPO2_ALG_LIB

#ifdef SPO2_DATA_CALI
void hx3690_spo2_data_cali_init(void)
{
	uint8_t ii;
	for(ii=0; ii<4; ii++)
	{
		red_data_fifo[ii] = 0;
		ir_data_fifo[ii] = 0;
	}
	red_dc_temp = 0;
	ir_dc_temp = 0;
	red_data_pre = 0;
	ir_data_pre = 0;
	red_jump_delta = 0;
	ir_jump_delta = 0;
	cali_data_cnt = 0;
}

int32_t hx3690l_red_data_cali(int32_t red_new_raw_data)
{
    uint8_t ii;
    int32_t red_data_final;
    int32_t red_data_cali;
    spo2_wear_msg_code_t touch_status;
    touch_status = hx3690_spo2_get_wear_status();
    if(touch_status == MSG_SPO2_NO_WEAR)
    {
        cali_data_cnt = 0;
        red_data_cali = red_new_raw_data;
        for(ii=0;ii<4;ii++)
        {
            red_data_fifo[ii] = 0;
        }
        red_dc_temp = 0;
    }
    else
    {
        for(ii=3;ii>0;ii--)
        {
            red_data_fifo[ii] = red_data_fifo[ii-1];
        }
        red_data_fifo[0] = red_new_raw_data; 
        if(cali_data_cnt>=25) 
        {
        if ((((red_data_fifo[1] - red_data_fifo[2]) > SOP2_DEGLITCH_THRE) && ((red_data_fifo[1] - red_new_raw_data) > SOP2_DEGLITCH_THRE)) || \
           (((red_data_fifo[1] - red_data_fifo[2]) < -SOP2_DEGLITCH_THRE) && ((red_data_fifo[1] - red_new_raw_data) < -SOP2_DEGLITCH_THRE)))
        { 
            red_new_raw_data = red_data_fifo[2]; 
            red_data_fifo[1] = red_data_fifo[2]; 
        }
        else
        {
            red_new_raw_data = red_data_fifo[1];
        }	
        if ((abs((red_new_raw_data - red_jump_delta) - red_data_pre) > SPO2_REMOVE_JUMP_THRE))
        {
            red_jump_delta = red_new_raw_data - red_data_pre;
        }  
            red_data_cali = red_new_raw_data - red_jump_delta; 
            red_data_pre = red_data_cali;
        }
        else
        {
            red_data_cali = red_data_fifo[1];
            red_data_pre = red_data_fifo[1];
        }
    }
    if(cali_data_cnt<=30)
    {
		red_dc_temp = red_data_cali;
		red_data_final = red_new_raw_data;
    }
    else
    {
      //red_dc_temp = (red_dc_temp*31 + red_data_cali)>>5;
		red_dc_temp = 0;
		red_data_final = red_data_cali - red_dc_temp + 2608*50*hx3690l_spo2_agc_red_idac;
    }
	//DEBUG_PRINTF("%d %d %d %d %d %d\r\n", cali_data_cnt, red_new_raw_data, red_data_final, red_data_cali, red_dc_temp, hx3690l_spo2_agc_red_idac);
    return red_data_final;
}
int32_t hx3690l_ir_data_cali(int32_t ir_new_raw_data)
{
    uint8_t ii;
    int32_t ir_data_final;
    int32_t ir_data_cali;
    spo2_wear_msg_code_t touch_status;
    touch_status = hx3690_spo2_get_wear_status();
    if(touch_status == MSG_SPO2_NO_WEAR)
    {
        cali_data_cnt = 0;
        ir_data_cali = ir_new_raw_data;
        for(ii=0;ii<4;ii++)
        {
            ir_data_fifo[ii] = 0;
        }
        ir_dc_temp = 0;
    }
    else
    {
        for(ii=3;ii>0;ii--)
        {
          ir_data_fifo[ii] = ir_data_fifo[ii-1];
        }
        ir_data_fifo[0] = ir_new_raw_data; 
        cali_data_cnt++;
        if(cali_data_cnt>=25) 
        {
        if ((((ir_data_fifo[1] - ir_data_fifo[2]) > SOP2_DEGLITCH_THRE) && ((ir_data_fifo[1] - ir_new_raw_data) > SOP2_DEGLITCH_THRE)) || \
           (((ir_data_fifo[1] - ir_data_fifo[2]) < -SOP2_DEGLITCH_THRE) && ((ir_data_fifo[1] - ir_new_raw_data) < -SOP2_DEGLITCH_THRE)))
        { 
            ir_new_raw_data = ir_data_fifo[2]; 
            ir_data_fifo[1] = ir_data_fifo[2]; 
        }
        else
        {
            ir_new_raw_data = ir_data_fifo[1];
        }	
        if ((abs((ir_new_raw_data - ir_jump_delta) - ir_data_pre) > SPO2_REMOVE_JUMP_THRE))
        {
            ir_jump_delta = ir_new_raw_data - ir_data_pre;
        }  
            ir_data_cali = ir_new_raw_data - ir_jump_delta; 
            ir_data_pre = ir_data_cali;
        }
        else
        {
            ir_data_cali = ir_data_fifo[1];
            ir_data_pre = ir_data_fifo[1];
        }
    }
    if(cali_data_cnt<=30)
    {
		ir_dc_temp = ir_data_cali;
		ir_data_final = ir_new_raw_data;
    }
    else
    {
      //ir_dc_temp = (ir_dc_temp*31 + ir_data_cali)>>5;
		ir_dc_temp = 0;
		ir_data_final = ir_data_cali - ir_dc_temp + 2608*50*hx3690l_spo2_agc_ir_idac;
    }
	//DEBUG_PRINTF("%d %d %d %d %d %d\r\n", cali_data_cnt, ir_new_raw_data, ir_data_final, ir_data_cali, ir_dc_temp, hx3690l_spo2_agc_ir_idac);
    return ir_data_final;
}
#endif //SPO2_DATA_CALI

/*TYHX 血氧模式下 读取PPG， 默认320ms*/
void hx3690l_spo2_ppg_Int_handle(void)
{ 
	uint8_t     ii=0; 
	spo2_results_t spo2_alg_results = {MSG_SPO2_ALG_NOT_OPEN,0,0,0,0,0,0,1,0,0};
	spo2_wear_msg_code_t spo2_wear_status = MSG_SPO2_NO_WEAR;
	SPO2_CAL_SET_T cal = get_spo2_agc_status();
	int32_t     *red_buf = &(ppg_s_data.red_data[0]);
	int32_t     *ir_buf = &(ppg_s_data.ir_data[0]);
	int32_t     *green_buf = &(ppg_s_data.green_data[0]);
	uint8_t     *count = &(ppg_s_data.count);
    int16_t 	gsen_fifo_x_send[32]={0};
	int16_t 	gsen_fifo_y_send[32]={0};
	int16_t 	gsen_fifo_z_send[32]={0};
	#ifdef SPO2_DATA_CALI
	int32_t red_data_cali, ir_data_cali;
	#endif

    #ifdef SPO2_VECTOR
	for(ii=0;ii<8;ii++)
	{
		red_buf_vec[ii] = vec_red_data[spo2_send_cnt+ii];
		ir_buf_vec[ii] = vec_ir_data[spo2_send_cnt+ii];
		green_buf_vec[ii] = vec_green_data[spo2_send_cnt+ii];
	}
	spo2_send_cnt = spo2_send_cnt+8;
	*count = 8;
	tyhx_spo2_alg_send_data(red_buf, ir_buf, green_buf, cal.R_LEDDAC, cal.IR_LEDDAC, cal.G_LEDDAC, gsen_fifo_x_send, gsen_fifo_y_send, gsen_fifo_z_send, *count);
    
    #else
	if(hx3690l_spo2_read(&ppg_s_data) == 0)
	{
		return;
	}
    for(ii=0;ii<*count;ii++)
	{
        #ifdef TYHX_DEMO  //需要客户填写，将最新的COUNT组GSEN数据分别传入这3个数组
		gsen_fifo_x_send[ii] = gsen_fifo_x[32-*count+ii];
		gsen_fifo_y_send[ii] = gsen_fifo_y[32-*count+ii];
		gsen_fifo_z_send[ii] = gsen_fifo_z[32-*count+ii];
     //DEBUG_PRINTF("%d/%d %d %d %d %d %d %d\r\n" ,1+ii,*count,red_buf[ii],ir_buf[ii],green_buf[ii],gsen_fifo_x[ii],gsen_fifo_y[ii],gsen_fifo_z[ii]);
        #endif
	}
    //DEBUG_PRINTF("%d %d %d %d %d %d\r\n", cal.R_LED, cal.IR_LED, cal.G_LED, cal.R_LEDDAC, cal.IR_LEDDAC, cal.G_LEDDAC);
	spo2_wear_status = hx3690_spo2_get_wear_status();
	if(spo2_wear_status == MSG_SPO2_WEAR)
	{
		tyhx_spo2_alg_send_data(red_buf, ir_buf, green_buf, cal.R_LEDDAC, cal.IR_LEDDAC, cal.G_LEDDAC, gsen_fifo_x_send, gsen_fifo_y_send, gsen_fifo_z_send, *count);   
	}
    #endif      

    spo2_alg_results = tyhx_spo2_alg_get_results(); 				
//------------------------adjust-------------------	
	DEBUG_PRINTF("block_cnt: %d, data_cnt:%d, hr_result:%d,ir_quality:%d,motion:%d,poor_signal:%d,reliable_score_result:%d,screen_up:%d,spo2_result:%d\r\n" ,spo2_alg_results.block_cnt,spo2_alg_results.data_cnt,spo2_alg_results.hr_result,spo2_alg_results.ir_quality,spo2_alg_results.motion,spo2_alg_results.poor_signal,spo2_alg_results.reliable_score_result,spo2_alg_results.screen_up,spo2_alg_results.spo2_result);

/////////////////////////////////////////BLE+DISPLAY//////////////////////////////////////////	
	#ifdef TYHX_DEMO	
        oled_dis.refresh_time++;
        if(oled_dis.refresh_time >= 7) //330ms*3 = 990ms ~ 1s
        {
            oled_dis.refresh_flag = 1;
            oled_dis.refresh_time = 0;
            oled_dis.dis_mode = DIS_SPO2;
            oled_dis.dis_data1 = spo2_alg_results.hr_result;
            oled_dis.dis_data2 = spo2_alg_results.spo2_result;
        }

        #ifdef HRS_BLE_APP
        {
            rawdata_vector_t rawdata;
            SPO2_CAL_SET_T cal= get_spo2_agc_status();
            for(ii=0;ii<*count;ii++)
            {
                #ifdef SPO2_DATA_CALI
                ir_data_cali = hx3690l_ir_data_cali(ir_buf[ii]);
                red_data_cali = hx3690l_red_data_cali(red_buf[ii]);
                rawdata.red_raw_data = red_data_cali>>2;
                rawdata.ir_raw_data = ir_data_cali>>2;
                #else
                rawdata.red_raw_data = red_buf[ii];
                rawdata.ir_raw_data = ir_buf[ii];
                #endif
                rawdata.vector_flag = SPO2_VECTOR_FLAG;
                rawdata.data_cnt = cal.G_LEDDAC;
                rawdata.hr_result = spo2_alg_results.spo2_result;            
                rawdata.gsensor_x = (int16_t)(green_buf[ii]>>5);
                rawdata.gsensor_y = cal.R_LEDDAC;
                rawdata.gsensor_z = cal.IR_LEDDAC;
                rawdata.red_cur = cal.R_LED;
                rawdata.ir_cur = cal.IR_LED;
                ble_rawdata_vector_push(rawdata);                  
            }
        }
        #endif   
    #endif
/////////////////////////////////////////////////////////////////////////////////////////////////		
}

void hx3690l_hrspo2_ppg_Int_handle(void)
{ 
	uint8_t     ii=0; 
	spo2_results_t  spo2_alg_results = {MSG_SPO2_ALG_NOT_OPEN,0,0,0,0,0,0};
    hrs_results_t   alg_results = {MSG_HRS_ALG_NOT_OPEN,0,0,0,0};
	spo2_wear_msg_code_t spo2_wear_status = MSG_SPO2_NO_WEAR;
	SPO2_CAL_SET_T cal = get_spo2_agc_status();
	int32_t *red_buf = &(ppg_s_data.red_data[0]);
	int32_t *ir_buf = &(ppg_s_data.ir_data[0]);
	int32_t *green_buf = &(ppg_s_data.green_data[0]);
	uint8_t *count = &(ppg_s_data.count);
	int16_t gsen_fifo_x_send[32]={0};
	int16_t gsen_fifo_y_send[32]={0};
	int16_t gsen_fifo_z_send[32]={0};
	#ifdef SPO2_DATA_CALI
	int32_t red_data_cali, ir_data_cali;
	#endif

	if(hx3690l_spo2_read(&ppg_s_data) == 0)
	{
		return;
	}
    for(ii=0;ii<*count;ii++)
	{
        #ifdef TYHX_DEMO  //需要客户填写，将最新的COUNT组GSEN数据分别传入这3个数组
		gsen_fifo_x_send[ii] = gsen_fifo_x[32-*count+ii];
		gsen_fifo_y_send[ii] = gsen_fifo_y[32-*count+ii];
		gsen_fifo_z_send[ii] = gsen_fifo_z[32-*count+ii];
        #endif
	}
	spo2_wear_status = hx3690_spo2_get_wear_status();
	if(spo2_wear_status == MSG_SPO2_WEAR)
	{
		tyhx_spo2_alg_send_data(red_buf, ir_buf, green_buf, cal.R_LEDDAC, cal.IR_LEDDAC, cal.G_LEDDAC, gsen_fifo_x_send, gsen_fifo_y_send, gsen_fifo_z_send, *count); 
		tyhx_hrs_alg_send_data(green_buf,*count, gsen_fifo_x_send, gsen_fifo_y_send, gsen_fifo_z_send);
        //DEBUG_PRINTF("%d/%d %d %d %d %d %d %d\r\n" ,1+ii,*count,red_buf[ii],ir_buf[ii],green_buf[ii],gsen_fifo_x[ii],gsen_fifo_y[ii],gsen_fifo_z[ii]);        
	}  
    //DEBUG_PRINTF("%d %d %d %d %d %d\r\n", cal.R_LED, cal.IR_LED, cal.G_LED, cal.R_LEDDAC, cal.IR_LEDDAC, cal.G_LEDDAC);    

    spo2_alg_results = tyhx_spo2_alg_get_results(); 
    alg_results = tyhx_hrs_alg_get_results();   
	//DEBUG_PRINTF("spo2_wear_status: %d, hr_result:%d, spo2_result:%d\r\n" ,spo2_wear_status, spo2_alg_results.hr_result,spo2_alg_results.spo2_result);

/////////////////////////////////////////BLE+DISPLAY//////////////////////////////////////////	
    #ifdef TYHX_DEMO	
        oled_dis.refresh_time++;
        if(oled_dis.refresh_time >= 7)
        {
            oled_dis.refresh_flag = 1;
            oled_dis.refresh_time = 0;
            oled_dis.dis_mode = DIS_SPO2;
            oled_dis.dis_data1 = spo2_alg_results.hr_result;
            oled_dis.dis_data2 = spo2_alg_results.spo2_result;
        }

        #ifdef HRS_BLE_APP
        {
            rawdata_vector_t rawdata;
            SPO2_CAL_SET_T cal= get_spo2_agc_status();
            for(ii=0;ii<*count;ii++)
            {
                #ifdef SPO2_DATA_CALI
                ir_data_cali = hx3690l_ir_data_cali(ir_buf[ii]);
                red_data_cali = hx3690l_red_data_cali(red_buf[ii]);
                rawdata.red_raw_data = red_data_cali>>2;
                rawdata.ir_raw_data = ir_data_cali>>2;
                #else
                rawdata.red_raw_data = red_buf[ii];
                rawdata.ir_raw_data = ir_buf[ii];
                #endif
                rawdata.vector_flag = SPO2_VECTOR_FLAG;
                rawdata.data_cnt = cal.G_LEDDAC;
                rawdata.hr_result = spo2_alg_results.spo2_result;            
                rawdata.gsensor_x = (int16_t)(green_buf[ii]>>5);
                rawdata.gsensor_y = cal.R_LEDDAC;
                rawdata.gsensor_z = cal.IR_LEDDAC;
                rawdata.red_cur = cal.R_LED;
                rawdata.ir_cur = cal.IR_LED;
                ble_rawdata_vector_push(rawdata);                  
            }
        }
        #endif  
    #endif    
/////////////////////////////////////////////////////////////////////////////////////////////////		
}
#endif //SPO2_ALG_LIB

#ifdef HRV_ALG_LIB
void hx3690l_hrv_ppg_Int_handle(void)
{
	uint8_t        ii=0;
	hrv_results_t hrv_alg_results= {MSG_HRV_ALG_NOT_OPEN,0,0,0,0,0,1,0,0};
	int32_t *PPG_buf = &(ppg_s_data.green_data[0]);
    int32_t *ir_buf = &(ppg_s_data.ir_data[0]);
	uint8_t *count = &(ppg_s_data.count);
	hrv_wear_msg_code_t hrv_wear_status = MSG_HRV_NO_WEAR;
    int16_t gsen_fifo_x_send[64]={0};
	int16_t gsen_fifo_y_send[64]={0};
	int16_t gsen_fifo_z_send[64]={0};
    
   // DEBUG_PRINTF("1111 \r\n" );
    #ifdef HRV_TESTVEC
	int32_t hrm_raw_data;
	hrm_raw_data = vec_data[vec_data_cnt];
	vec_data_cnt++;
	alg_hrv_results = hx3690l_hrv_alg_send_data(hrm_raw_data, 0, 0);
    #else
	if(hx3690l_hrv_read(&ppg_s_data) == 0)
	{
		return;
	}
    for(ii=0;ii<*count;ii++)
	{
        #ifdef TYHX_DEMO  //需要客户填写，将最新的COUNT组GSEN数据分别传入这3个数组
		gsen_fifo_x_send[ii] = gsen_fifo_x[32-8+ii%8];
		gsen_fifo_y_send[ii] = gsen_fifo_y[32-8+ii%8];
		gsen_fifo_z_send[ii] = gsen_fifo_z[32-8+ii%8];
      //  DEBUG_PRINTF("%d/%d %d %d %d %d %d \r\n" ,1+ii,*count,PPG_buf[ii],ir_buf[0],gsen_fifo_x_send[ii] ,gsen_fifo_y_send[ii] ,gsen_fifo_z_send[ii] );
        #endif
	}
	hrv_wear_status = hx3690_hrv_get_wear_status();
//    DEBUG_PRINTF("wear:%d \r\n" ,hrv_wear_status);
	if(hrv_wear_status==MSG_HRS_WEAR)
	{
	   hrv_alg_results = tyhx_hrv_alg_send_bufdata(PPG_buf, *count, gsen_fifo_x_send, gsen_fifo_y_send, gsen_fifo_z_send);
	}
	//--------------adjust----------------
//	hrv_alg_results.motion,hrv_alg_results.screen_up;
	//DEBUG_PRINTF("data_cnt:%d hrv_result:%d motion:%d p_m:%d screen_up:%d signal_quality:%d \r\n", hrv_alg_results.data_cnt,hrv_alg_results.hrv_result,hrv_alg_results.motion,hrv_alg_results.p_m,hrv_alg_results.screen_up,hrv_alg_results.signal_quality);
	#endif

/////////////////////////////////////////BLE+DISPLAY//////////////////////////////////////////
    #ifdef TYHX_DEMO
        oled_dis.refresh_time++;
        if(oled_dis.refresh_time >= 10)
        {
            oled_dis.refresh_flag = 1;
            oled_dis.refresh_time = 0;
            oled_dis.dis_mode = DIS_HRV;   
            oled_dis.dis_data1 = hrv_alg_results.hrv_result;
            oled_dis.dis_data2 = hrv_alg_results.hrv_result;			
        }
        
        #ifdef HRS_BLE_APP
        {
            rawdata_vector_t rawdata;       
            HRS_CAL_SET_T cal= get_hrs_agc_status();
            for(ii=0;ii<*count;ii++)
            {
                rawdata.vector_flag = HRS_VECTOR_FLAG;
                rawdata.data_cnt = 0;
                rawdata.hr_result = hrv_alg_results.hrv_result;           
                rawdata.red_raw_data = PPG_buf[ii];
                rawdata.ir_raw_data = 0;
                rawdata.gsensor_x = gsen_fifo_x_send[ii];
                rawdata.gsensor_y = gsen_fifo_y_send[ii];
                rawdata.gsensor_z = gsen_fifo_z_send[ii];
                rawdata.red_cur = cal.LED;
                rawdata.ir_cur = 0;
                ble_rawdata_vector_push(rawdata);   
            }
        }
        #endif
    #endif    
//////////////////////////////////////////////////////////////////////////////////////////////
}
#endif

#ifdef CHECK_TOUCH_LIB
void hx3690l_wear_ppg_Int_handle(void)
{
    uint8_t *count = &(ppg_s_data.count);
    int32_t *ir_buf = &(ppg_s_data.ir_data[0]);
    hx3690_wear_msg_code_t hx3690l_wear_status;
    hx3690l_check_touch_read(&ppg_s_data);
    hx3690l_wear_status = hx3690l_check_touch_send_data(ir_buf,*count);
//    for(int i=0;i<*count;i++)
//    {
//        DEBUG_PRINTF("%d,%d,%d,%d,%d,%d\r\n",1+i,*count,ir_buf[i],ppg_s_data.s_buf[i*2],ppg_s_data.s_buf[i*2+1],hx3690l_wear_status);
//    }
}
#endif

#ifdef LAB_TEST
void hx3690l_lab_test_Int_handle(void)
{
    NOISE_PS_T ps_noise;
	ps_noise = hx3690l_lab_test_read_packet();
    DEBUG_PRINTF("%d %d %d %d\r\n", ps_noise.p1_noise, ps_noise.p2_noise, ps_noise.p3_noise, ps_noise.p4_noise);
}
#endif
