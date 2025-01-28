/*
                       客制化配置说明

1. 根据平台完成以下函数接口
	void hx3605_delay_us(uint32_t us);
	void hx3605_delay(uint32_t ms);
	bool hx3605_write_reg(uint8_t addr, uint8_t data);
	uint8_t hx3605_read_reg(uint8_t addr);
	bool hx3605_brust_read_reg(uint8_t addr , uint8_t *buf, uint8_t length);
	void hx3605_ppg_timer_cfg(bool en);
	void hx3605_agc_timer_cfg(bool en);

2. void hx3605_ppg_timer_cfg(bool en); 默认定时器为320ms循环调用， 定时器调用函数为 void ppg_timeout_handler(void * p_context)

	void hx3605_agc_timer_cfg(bool en); 默认定时器为40ms 循环调用， 定时器调用函数为 void agc_timeout_handler(void * p_context)

*/

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>



#include "hx3605.h"

#include "hx3605_hrs_agc.h"
#include "hx3605_spo2_agc.h"
#include "hx3605_hrv_agc.h"
#include "hx3605_factory_test.h"
#include "tyhx_hrs_alg.h"
#include "tyhx_spo2_alg.h"
#include "tyhx_hrv_alg.h"

#ifdef TYHX_DEMO
#include "demo_ctrl.h"
#include "oled_iic.h"
#include "word.h"
#include "iic.h"
#include "SEGGER_RTT.h"
#include "app_timer.h"
#include "nrf_delay.h"
#include "nrf_gpio.h"
#include "nrf_drv_gpiote.h" 
#include "twi_master.h"

#include "drv_oled.h"
#include "opr_oled.h"

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
uint32_t spo2_send_cnt = 0;
int32_t PPG_buf_vec[8];
#endif

#ifdef HRV_TESTVEC    
#include "hrv_testvec.h"
#endif

#ifdef GSENSER_DATA
#include "lis3dh_drv.h"
#endif


//SPO2 agc
const uint8_t  hx3605_spo2_agc_red_idac = RED_AGC_OFFSET;
const uint8_t  hx3605_spo2_agc_ir_idac = IR_AGC_OFFSET;
//hrs agc
const uint8_t  hx3605_hrs_agc_idac = HR_GREEN_AGC_OFFSET;
//hrv agc
const uint8_t  hx3605_hrv_agc_idac = HR_GREEN_AGC_OFFSET;

const uint8_t  red_led_max_init = 63;
const uint8_t  ir_led_max_init = 63;
const uint8_t  green_led_max_init = 63;

uint8_t low_vbat_flag =0;
WORK_MODE_T work_mode_flag = HRS_MODE;
const uint16_t static_thre_val = 150;
const uint8_t  gsen_lv_val = 0;

//HRS_INFRARED_THRES
const int32_t  hrs_ir_unwear_thres = 15000;
const int32_t  hrs_ir_wear_thres = 15000;
//HRV_INFRARED_THRES
const int32_t  hrv_ir_unwear_thres = 4000;
const int32_t  hrv_ir_wear_thres = 2000; 
//SPO2_INFRARED_THRES 
const uint8_t  spo2_check_unwear_oft = 10;
const int32_t  spo2_ir_wear_thres = 2000;
//CHECK_WEAR_MODE_THRES
const int32_t  check_mode_unwear_thre = 4000;
const int32_t  check_mode_wear_thre = 2000;

#ifdef BIG_LIB
uint8_t alg_ram[11 * 1024] __attribute__((aligned(4)));
#else
uint8_t alg_ram[5 * 1024] __attribute__((aligned(4)));
#endif

uint32_t vec_data_cnt=0;
uint8_t spo2_rand_cnt = 0;
int8_t  spo2_rand_value = 0;
int8_t  spo2_rand_value_pre = 0;
uint8_t read_fifo_first_flg = 0;

#ifdef TYHX_DEMO
volatile oled_display_t oled_dis;
#endif

#ifdef GSENSER_DATA
int16_t gsen_fifo_x[32]; 
int16_t gsen_fifo_y[32];
int16_t gsen_fifo_z[32];
#else
int16_t gen_dummy[32] = {0};
#endif

//////// spo2 para and switches
const  uint8_t   COUNT_BLOCK_NUM = 50;            //delay the block of some single good signal after a series of bad signal 
const  uint8_t   SPO2_LOW_XCORR_THRE = 30;        //(64*xcorr)'s square below this threshold, means error signal
const  uint8_t   SPO2_CALI = 1;                       //spo2_result cali mode
const  uint8_t   XCORR_MODE = 1;                  //xcorr mode switch
const  uint8_t   QUICK_RESULT = 1;                //come out the spo2 result quickly ;0 is normal,1 is quick
const  uint16_t  MEAN_NUM = 32;                  //the length of smooth-average ;the value of MEAN_NUM can be given only 256 and 512
const  uint8_t   G_SENSOR = 0;                      //if =1, open the gsensor mode
const  uint8_t   SPO2_GSEN_POW_THRE = 150;         //gsen pow judge move, range:0-200;
const  uint32_t  SPO2_BASE_LINE_INIT = 135500;    //spo2 baseline init, = 103000 + ratio(a variable quantity,depends on different cases)*SPO2_SLOPE
const  int32_t   SOP2_DEGLITCH_THRE = 100000;     //remove signal glitch over this threshold
const  int32_t   SPO2_REMOVE_JUMP_THRE = 100000;  //remove signal jump over this threshold
const  uint32_t  SPO2_SLOPE = 50000;              //increase this slope, spo2 reduce more
const  uint16_t  SPO2_LOW_CLIP_END_TIME = 1500;   //low clip mode before this data_cnt, normal clip mode after this
const  uint16_t  SPO2_LOW_CLIP_DN  = 150;         //spo2 reduce 0.15/s at most in low clip mode
const  uint16_t  SPO2_NORMAL_CLIP_DN  = 500;      //spo2 reduce 0.5/s at most in normal clip mode
const  uint8_t   SPO2_LOW_SNR_THRE = 40;          //snr below this threshold, means error signal
const  uint16_t  IR_AC_TOUCH_THRE = 200;          //AC_min*0.3
const  uint16_t  IR_FFT_POW_THRE = 200;           //fft_pow_min
const  uint8_t   SLOPE_PARA_MAX = 30;
const  uint8_t   SLOPE_PARA_MIN = 3;

bool timer_40ms_en = 0;
bool timer_320ms_en = 0;
uint8_t timer_320ms_cnt = 0;

/* hr os api */
extern hr_os_api_t hr_os_api;

void hx3605_delay_us(uint32_t us)
{
	#ifdef TYHX_DEMO
		nrf_delay_us(us);
	#endif
	sensor_udelay(us);
//	printk("hx3605_delay_us %d \r\n",us);
}

void hx3605_delay(uint32_t ms)
{
	#ifdef TYHX_DEMO
		nrf_delay_ms(ms);
	#endif
	sensor_mdelay(ms);
//	printk("hx3605_delay %d \r\n",ms);
}

bool hx3605_write_reg(uint8_t addr, uint8_t data) 
{
	#ifdef TYHX_DEMO
  	uint8_t data_buf[2];    
    data_buf[0] = addr;
    data_buf[1] = data;
    twi_pin_switch(1);
    twi_master_transfer(0x88, data_buf, 2, true);    //write  
	#endif

    return sensor_hal_write(ID_HR, addr, &data, 1);
}

uint8_t hx3605_read_reg(uint8_t addr) 
{
    
	uint8_t data_buf = 0;   
	#ifdef TYHX_DEMO
    twi_pin_switch(1);
    twi_master_transfer(0x88, &addr, 1, false);      //write
    twi_master_transfer(0x89, &data_buf, 1, true);//read
	#endif

	sensor_hal_read(ID_HR, addr, &data_buf, 1);

    return data_buf;  
}

bool hx3605_brust_read_reg(uint8_t addr , uint8_t *buf, uint8_t length) 
{
	#ifdef TYHX_DEMO
    twi_pin_switch(1);
    twi_master_transfer(0x88, &addr, 1, false);      //write
    twi_master_transfer(0x89, buf, length, true); //read
	#endif

    return sensor_hal_read(ID_HR, addr, buf, length);      
}

// 定时器调用函数 void ppg_timeout_handler(void * p_context)

void hx3605_ppg_timer_cfg(bool en)
{
	#ifdef TYHX_DEMO
    if(en)
    {
        hx3605_timers_start();
    }
    else
    {
        hx3605_timers_stop();
    }
	#endif
	timer_320ms_en = en;
	timer_320ms_cnt = 0;
//	printk("320ms timer %d \r\n",en);
}
// agc 调用函数 void agc_timeout_handler(void * p_context)
void hx3605_agc_timer_cfg(bool en)
{
	#ifdef TYHX_DEMO
    if(en)
    {
        hx3605_40ms_timers_start();   
    }
    else
    { 
        hx3605_40ms_timers_stop();   
    }
	#endif
	timer_40ms_en = en;
//	printk("40ms timer %d \r\n",en);
}
void hx3605_gpioint_cfg(bool en)
{
	#ifdef TYHX_DEMO
    if(en)
    {
         hx3605_gpioint_enable();
    }
    else
    {
         hx3605_gpioint_disable();
    }
	#endif
}
    

uint16_t hx3605_read_fifo_data(int32_t *buf, uint8_t phase_num, uint8_t sig)
{
    uint8_t data_flg_start = 0;
		uint8_t data_flg_end = 0;
    uint8_t databuf[3];
    uint32_t ii=0;
		uint16_t data_len = 0;
		uint16_t fifo_data_length = 0;
		uint16_t fifo_read_length = 0;
		uint16_t fifo_read_bytes = 0;
		uint16_t fifo_out_length = 0;
		uint16_t fifo_out_count = 0;
		uint8_t fifo_data_buf[96] = {0};
		
		data_len = hx3605_read_reg(0x30);
		fifo_data_length = data_len;
		//DEBUG_PRINTF("%d\r\n",data_len);
		if(fifo_data_length<2*phase_num)
		{
			return 0;
		}
		fifo_read_length = ((fifo_data_length-phase_num)/phase_num)*phase_num;
		fifo_read_bytes = fifo_read_length*3;
		if(read_fifo_first_flg == 1)
		{
			hx3605_brust_read_reg(0x31, databuf, 3);	
			read_fifo_first_flg = 0;
		}
		hx3605_brust_read_reg(0x31, fifo_data_buf, fifo_read_bytes);	
//		for(ii=0; ii<fifo_read_bytes; ii++)
//		{
//			DEBUG_PRINTF("%d/%d, %d\r\n", ii+1,fifo_read_bytes,fifo_data_buf[ii]);				
//		}
		for(ii=0; ii<fifo_read_length; ii++)
		{
			if(sig==0)
			{
				buf[ii] = (int32_t)(fifo_data_buf[ii*3]|(fifo_data_buf[ii*3+1]<<8)|((fifo_data_buf[ii*3+2]&0x1f)<<16));
			}
			else
			{
				if((fifo_data_buf[ii*3+2]&0x10)!=0)
				{
					buf[ii] = (int32_t)(fifo_data_buf[ii*3]|(fifo_data_buf[ii*3+1]<<8)|((fifo_data_buf[ii*3+2]&0x0f)<<16))-1048576;
				}
				else
				{
					buf[ii] = (int32_t)(fifo_data_buf[ii*3]|(fifo_data_buf[ii*3+1]<<8)|((fifo_data_buf[ii*3+2]&0x1f)<<16));
				}
			}
			//DEBUG_PRINTF("%d/%d, %d %d\r\n", ii+1,fifo_read_length,buf[ii],(fifo_data_buf[ii*3+2]>>5)&0x03);				
		}
		data_flg_start = (fifo_data_buf[2]>>5)&0x03;
		data_flg_end = (fifo_data_buf[fifo_read_bytes-1]>>5)&0x03;
		fifo_out_length = fifo_read_length;
		if(data_flg_start>0)
		{
			fifo_out_length = fifo_read_length-phase_num+data_flg_start;
			for(ii=0; ii<fifo_out_length; ii++)
			{
				buf[ii] = buf[ii+phase_num-data_flg_start];
			}
			for(ii=fifo_out_length; ii<fifo_read_length; ii++)
			{
				buf[ii] = 0;
			}
		}
		if(data_flg_end<phase_num-1)
		{
			for(ii=fifo_out_length; ii<fifo_out_length+phase_num-data_flg_end-1; ii++)
			hx3605_brust_read_reg(0x31, databuf, 3);
			buf[ii] = (int32_t)(databuf[0]|(databuf[1]<<8)|((databuf[2]&0x1f)<<16)); 
		}   
//		for(ii=0; ii<fifo_out_length; ii++)
//		{
//			DEBUG_PRINTF("%d/%d, %d\r\n", ii+1,fifo_out_length,buf[ii]);				
//		}
		fifo_out_length = fifo_out_length+phase_num-data_flg_end-1;
		fifo_out_count = fifo_out_length/phase_num;
		if(data_len==32)
		{
			uint8_t reg_0x2d = hx3605_read_reg(0x2d);
			hx3605_write_reg(0x2d,0x00);
			hx3605_delay(5);
			hx3605_write_reg(0x2d,reg_0x2d);
			read_fifo_first_flg = 1;
		}
		return fifo_out_count;
}

void read_data_packet(int32_t *ps_data)
{    
    uint8_t  databuf1[6] = {0};
    uint8_t  databuf2[6] = {0};
    hx3605_brust_read_reg(0x02, databuf1, 6);
		hx3605_brust_read_reg(0x08, databuf2, 6);

		ps_data[0] = ((databuf1[0]) | (databuf1[1] << 8) | (databuf1[2] << 16));
		ps_data[1] = ((databuf1[3]) | (databuf1[4] << 8) | (databuf1[5] << 16));
		ps_data[2] = ((databuf2[0]) | (databuf2[1] << 8) | (databuf2[2] << 16));
		ps_data[3] = ((databuf2[3]) | (databuf2[4] << 8) | (databuf2[5] << 16));
		//DEBUG_PRINTF(" %d %d %d %d \r\n",  ps_data[0], ps_data[1], ps_data[2], ps_data[3]);
}

uint8_t chip_id = 0;
bool hx3605_chip_check(void)
{
    uint8_t i = 0;   
    for(i=0;i<10;i++)
    {
        hx3605_write_reg(0x01, 0x00);
        hx3605_delay(5);
        chip_id = hx3605_read_reg(0x00);  
        if (chip_id == 0x25)
        {
            return true;
        }        
    }
    return false;
}

uint8_t hx3605_read_fifo_size(void) // 20200615 ericy read fifo data number
{
    uint8_t fifo_num_temp = 0;
    fifo_num_temp = hx3605_read_reg(0x14)&0x7f;

    return fifo_num_temp;
}

void hx3605_vin_check(uint16_t led_vin)
{
    low_vbat_flag = 0;
    if(led_vin < 3700)
    {
      low_vbat_flag = 1;      
    }
}

void hx3605_ppg_off(void) // 20200615 ericy chip sleep enable
{
#ifdef INT_FLOATING
		hx3605_write_reg(0xc0, 0x08);
#endif
    hx3605_write_reg(0x22, 0x00);
    hx3605_write_reg(0x10, 0x00);
    hx3605_write_reg(0x01, 0x01);
}

void hx3605_ppg_on(void)
{
    hx3605_write_reg(0x01, 0x00);
		hx3605_delay(5);
}

void hx3605_data_reset(void)
{
		#if defined(TIMMER_MODE)
    hx3605_ppg_timer_cfg(false);
    hx3605_agc_timer_cfg(false);
		#elif defined(INT_MODE)
    hx3605_gpioint_cfg(false);
		#endif	
		#if defined(HRS_ALG_LIB)
		hx3605_hrs_data_reset();
		#endif
		#if defined(SPO2_ALG_LIB)
		hx3605_spo2_data_reset();
		#endif
		#if defined(HRV_ALG_LIB)
		hx3605_hrv_data_reset();
		#endif
    AGC_LOG("hx3690l data reset!\r\n");
}

bool hx3605_init(WORK_MODE_T mode)
{
    work_mode_flag = mode;
    hx3605_data_reset();
    hx3605_vin_check(3800);
		switch (work_mode_flag)
		{
			case HRS_MODE:
				#ifdef HRS_ALG_LIB
				if(hx3605_hrs_enable()== SENSOR_OP_FAILED) 
        {
            return false;
        }
				#endif
				break;
				
			case LIVING_MODE:
				#ifdef CHECK_LIVING_LIB
				if(hx3605_hrs_enable()== SENSOR_OP_FAILED) 
        {
            return false;
        }
				#endif
				break;
				
			case SPO2_MODE:
				#ifdef SPO2_DATA_CALI
				hx3605_spo2_data_cali_init();
				#endif
				#ifdef SPO2_ALG_LIB
				tyhx_spo2_para_usuallyadjust(SPO2_LOW_XCORR_THRE,SPO2_LOW_SNR_THRE,COUNT_BLOCK_NUM,SPO2_BASE_LINE_INIT,SPO2_SLOPE,SPO2_GSEN_POW_THRE);
				tyhx_spo2_para_barelychange(MEAN_NUM,SOP2_DEGLITCH_THRE,SPO2_REMOVE_JUMP_THRE,SPO2_LOW_CLIP_END_TIME,SPO2_LOW_CLIP_DN, \
																		SPO2_NORMAL_CLIP_DN,IR_AC_TOUCH_THRE,IR_FFT_POW_THRE,SPO2_CALI,SLOPE_PARA_MAX,SLOPE_PARA_MIN);
				if(hx3605_spo2_enable()== SENSOR_OP_FAILED) 
        {
            return false;
        }
				#endif
				break;
				
			case HRV_MODE:
				#ifdef HRV_ALG_LIB
				if(hx3605_hrv_enable()== SENSOR_OP_FAILED) 
        {
            return false;
        }
				#endif
				break;
				
			case WEAR_MODE:
				#ifdef CHECK_TOUCH_LIB
				if(hx3605_check_touch_enable()== SENSOR_OP_FAILED) 
        {
            return false;
        }
				#endif
				break;
				
			case FT_LEAK_LIGHT_MODE:
				hx3605_factroy_test(LEAK_LIGHT_TEST);
				break;
				
			case FT_GRAY_CARD_MODE:
				hx3605_factroy_test(GRAY_CARD_TEST);
				break;
			
			case FT_INT_TEST_MODE:
				hx3605_factroy_test(FT_INT_TEST);
			  hx3605_gpioint_cfg(true);
				break;
			
			case LAB_TEST_MODE:
				#ifdef LAB_TEST
				if(hx3605_lab_test_enable()== SENSOR_OP_FAILED) 
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

void hx3605_agc_Int_handle(void)
{      
    switch (work_mode_flag)
		{
				case HRS_MODE:
				{
					#ifdef HRS_ALG_LIB
					HRS_CAL_SET_T cal;
					cal = PPG_hrs_agc();					
					if(cal.work)
					{
							AGC_LOG("AGC: led_drv=%d,ledDac=%d,ambDac=%d,ledstep=%d,rf=%d\r\n",
							cal.led_cur, cal.led_idac, cal.amb_idac,cal.led_step,cal.ontime);
					}
					#endif
					break;
				}
				case LIVING_MODE:
				{
					#ifdef HRS_ALG_LIB
					HRS_CAL_SET_T cal;
					cal = PPG_hrs_agc();					
					if(cal.work)
					{
							AGC_LOG("AGC: led_drv=%d,ledDac=%d,ambDac=%d,ledstep=%d,rf=%d\r\n",
							cal.led_cur, cal.led_idac, cal.amb_idac,cal.led_step,cal.ontime);
					}
					#endif
					break;
				}
				case HRV_MODE:
				{
					#ifdef HRV_ALG_LIB
					HRV_CAL_SET_T cal;
					cal = PPG_hrv_agc();					
					if(cal.work)
					{
							AGC_LOG("AGC: led_drv=%d,ledDac=%d,ambDac=%d,ledstep=%d,rf=%d\r\n",
							cal.led_cur, cal.led_idac, cal.amb_idac,cal.led_step,cal.ontime);
					}
					#endif
					break;
				}
				case SPO2_MODE:
				{
					#ifdef SPO2_ALG_LIB 
					SPO2_CAL_SET_T cal;
					cal = PPG_spo2_agc();					
					if(cal.work)
					{
							AGC_LOG("INT_AGC: Rled_drv=%d,Irled_drv=%d,RledDac=%d,IrledDac=%d,ambDac=%d,Rledstep=%d,Irledstep=%d,Rrf=%d,Irrf=%d,\r\n",\
							cal.red_cur, cal.ir_cur,cal.red_idac,cal.ir_idac,cal.amb_idac,cal.red_led_step,cal.red_led_step,cal.ontime,cal.state);
					}
					#endif
					break;
				}
				default:
					break;
		}	
}

void agc_timeout_handler(void * p_context)
{
		#ifdef TIMMER_MODE
    switch (work_mode_flag)
		{
				case HRS_MODE:
				{
					#ifdef HRS_ALG_LIB
					HRS_CAL_SET_T cal;
					cal = PPG_hrs_agc();					
					if(cal.work)
					{
							AGC_LOG("AGC: led_drv=%d,ledDac=%d,ambDac=%d,ledstep=%d,rf=%d\r\n",
							cal.led_cur, cal.led_idac, cal.amb_idac,cal.led_step,cal.ontime);
					}
					#endif
					break;
				}
				case LIVING_MODE:
				{
					#ifdef HRS_ALG_LIB
					HRS_CAL_SET_T cal;
					cal = PPG_hrs_agc();					
					if(cal.work)
					{
							AGC_LOG("AGC: led_drv=%d,ledDac=%d,ambDac=%d,ledstep=%d,rf=%d\r\n",
							cal.led_cur, cal.led_idac, cal.amb_idac,cal.led_step,cal.ontime);
					}
					#endif
					break;
				}
				case HRV_MODE:
				{
					#ifdef HRV_ALG_LIB
					HRV_CAL_SET_T cal;
					cal = PPG_hrv_agc();					
					if(cal.work)
					{
							AGC_LOG("AGC: led_drv=%d,ledDac=%d,ambDac=%d,ledstep=%d,rf=%d\r\n",
							cal.led_cur, cal.led_idac, cal.amb_idac,cal.led_step,cal.ontime);
					}
					#endif
					break;
				}
				case SPO2_MODE:
				{
					#ifdef SPO2_ALG_LIB 
					SPO2_CAL_SET_T cal;
					cal = PPG_spo2_agc();					
					if(cal.work)
					{
							AGC_LOG("AGC: Rled_drv=%d,Irled_drv=%d,RledDac=%d,IrledDac=%d,ambDac=%d,Rledstep=%d,Irledstep=%d,Rrf=%d,Irrf=%d,\r\n",\
							cal.red_cur, cal.ir_cur,cal.red_idac,cal.ir_idac,cal.amb_idac,cal.red_led_step,cal.red_led_step,cal.ontime,cal.state);
					}
					#endif
					break;
				}
				case LAB_TEST_MODE:
				{
					#ifdef LAB_TEST
					hx3605_lab_test_Int_handle();
					#endif
				}
				default:
					break;
		}
		#endif 
		
		#ifdef GSEN_40MS_TIMMER
		hx3605_gesensor_Int_handle();
		#endif	
}

void ppg_timeout_handler(void * p_context)
{ 
		switch(work_mode_flag)
		{
			case HRS_MODE:
				#ifdef HRS_ALG_LIB
        hx3605_hrs_ppg_Int_handle();
        #endif
				break;
			
			case HRV_MODE:
				#ifdef HRV_ALG_LIB
        hx3605_hrv_ppg_Int_handle();
        #endif
				break;
			
			case SPO2_MODE:
				#ifdef SPO2_ALG_LIB
        hx3605_spo2_ppg_Int_handle();
        #endif
				break;
			
			case WEAR_MODE:
				#ifdef CHECK_TOUCH_LIB
				hx3605_wear_ppg_Int_handle();
				#endif
				break;
			
			case LIVING_MODE:
				#ifdef CHECK_LIVING_LIB
				hx3605_living_Int_handle();
				#endif
				break;
			
			default:
				break;
		}
}

void hx3605_ppg_Int_handle(void)
{
		hx3605_agc_Int_handle();
		#if defined(INT_MODE)
		switch(work_mode_flag)
		{
			case HRS_MODE:
				#ifdef HRS_ALG_LIB
        hx3605_hrs_ppg_Int_handle();
        #endif
				break;
			
			case SPO2_MODE:
				#ifdef SPO2_ALG_LIB
        hx3605_spo2_ppg_Int_handle();
        #endif
				break;
			
			case HRV_MODE:
				#ifdef HRV_ALG_LIB
        hx3605_hrv_ppg_Int_handle();
        #endif
				break;
					
			case WEAR_MODE:
				#ifdef CHECK_TOUCH_LIB
				hx3605_wear_ppg_Int_handle();
				#endif
				break;
			
			case LAB_TEST_MODE:
				#ifdef LAB_TEST
				hx3605_lab_test_Int_handle();
				#endif
			
			default:
				break;
		} 
		#endif
}


void hx3605_gesensor_Int_handle(void)
{
		#ifdef GSENSER_DATA
    uint8_t ii = 0;
    AxesRaw_t gsen_buf;
    if(work_mode_flag == WEAR_MODE)
    {
        return;
    }
    
    LIS3DH_GetAccAxesRaw(&gsen_buf);

    for(ii=0;ii<15;ii++)
    {
        gsen_fifo_x[ii] = gsen_fifo_x[ii+1];
        gsen_fifo_y[ii] = gsen_fifo_y[ii+1];
        gsen_fifo_z[ii] = gsen_fifo_z[ii+1];
    }
    gsen_fifo_x[15] = gsen_buf.AXIS_X>>1;
    gsen_fifo_y[15] = gsen_buf.AXIS_Y>>1;
    gsen_fifo_z[15] = gsen_buf.AXIS_Z>>1;
//		DEBUG_PRINTF("%d %d %d\r\n",gsen_fifo_x[15],gsen_fifo_y[15],gsen_fifo_z[15]);
		#endif 
}

#ifdef HRS_ALG_LIB
hrs_sensor_data_t hrs_s_dat;
hrs_results_t alg_results_pre;
void hx3605_hrs_ppg_Int_handle(void)
{
    uint8_t        ii=0;   
    hrs_results_t alg_results = {MSG_HRS_ALG_NOT_OPEN,0,0,0,0};
		hx3605_hrs_wear_msg_code_t hrs_wear_status = MSG_HRS_INIT;
    int32_t *PPG_buf = &(hrs_s_dat.ppg_data[0]);
//    uint32_t *als = &(hrs_s_dat.als);
//    int32_t *ir_buf = &(hrs_s_dat.ir_data[0]);
    uint8_t *count = &(hrs_s_dat.count);
 //   int32_t *s_buf = &(hrs_s_dat.s_buf[0]);
		int16_t gsen_fifo_x_send[32]={0};
		int16_t gsen_fifo_y_send[32]={0};
		int16_t gsen_fifo_z_send[32]={0};
		#ifdef BP_CUSTDOWN_ALG_LIB        
//    bp_results_t    bp_alg_results ;   
		#endif   
    #ifdef HR_VECTOR
      for(ii=0;ii<8;ii++)
      {
        PPG_buf_vec[ii] = hrm_input_data[spo2_send_cnt+ii];
        gsen_fifo_x[ii] = gsen_input_data_x[spo2_send_cnt+ii];
        gsen_fifo_y[ii] = gsen_input_data_y[spo2_send_cnt+ii];
        gsen_fifo_z[ii] = gsen_input_data_z[spo2_send_cnt+ii];
      }
      spo2_send_cnt = spo2_send_cnt+8;
      *count = 8;
      tyhx_hrs_alg_send_data(PPG_buf_vec,*count, gsen_fifo_x, gsen_fifo_y, gsen_fifo_z); 
    #else
      if(hx3605_hrs_read(&hrs_s_dat) == 0)
      { 
          return;
      }
      for(ii=0;ii<*count;ii++)
      {
				#ifdef GSENSER_DATA
				gsen_fifo_x_send[ii] = gsen_fifo_x[32-*count+ii];
				gsen_fifo_y_send[ii] = gsen_fifo_y[32-*count+ii];
				gsen_fifo_z_send[ii] = gsen_fifo_z[32-*count+ii];
				#endif
				//DEBUG_PRINTF("%d %d %d %d %d %d\r\n", PPG_buf[ii],ir_buf[ii],gsen_fifo_x[ii],gsen_fifo_y[ii],gsen_fifo_z[ii],hrs_s_dat.agc_green);
      }
			hrs_wear_status = hx3605_hrs_get_wear_status();
			if(hrs_wear_status == MSG_HRS_WEAR)
			{
					tyhx_hrs_alg_send_data(PPG_buf,*count, gsen_fifo_x_send, gsen_fifo_y_send, gsen_fifo_z_send);  
			}				
    #endif      
    alg_results = tyhx_hrs_alg_get_results();  

	if(hrs_wear_status)
		hr_os_api.hb_handler(alg_results.hr_result, 0, 0);
	else
		hr_os_api.hb_handler(0, 0, 0);
	printk("hrs_wear_status %d \r\n",hrs_wear_status);
	printk("tyhx_hrs_alg_get_results %d \r\n",alg_results.hr_result);		
///////////////////////////////////////////////////////////////////////////////////////////////////  
#ifdef TYHX_DEMO
    oled_dis.refresh_time++;
		if(alg_results.hr_result==0)
		{
			oled_dis.dis_mode = DIS_HR;    
		}
//		if(alg_results.hr_result>0 && alg_results_pre.hr_result==0)
//		{
//			if(alg_results.hr_result_qual<5)
//			{
//				oled_dis.dis_mode = DIS_WAIT;
//			}
//		}
    if(oled_dis.refresh_time >= 5) //320ms*5 = 1600ms
    {
        oled_dis.refresh_flag = 1;
        oled_dis.refresh_time = 0;
        //oled_dis.dis_mode = DIS_HR;         
        oled_dis.dis_data1 = alg_results.hr_result;
				oled_dis.dis_data2 = alg_results.cal_result;
    }
		alg_results_pre = alg_results;
#endif
    #ifdef HRS_BLE_APP
    {
        rawdata_vector_t rawdata;
        
        HRS_CAL_SET_T cal= get_hrs_agc_status();
        for(ii=0;ii<*count;ii++)
        {
            rawdata.vector_flag = HRS_VECTOR_FLAG;
            rawdata.data_cnt = alg_results.data_cnt-*count+ii;
            rawdata.hr_result = alg_results.hr_result;           
            rawdata.red_raw_data = PPG_buf[ii];
            rawdata.ir_raw_data = ir_buf[ii];
            rawdata.gsensor_x = gsen_fifo_x[ii];
            rawdata.gsensor_y = gsen_fifo_y[ii];
            rawdata.gsensor_z = gsen_fifo_z[ii];
            rawdata.red_cur = cal.led_cur;
            rawdata.ir_cur = alg_results.hrs_alg_status;
            
            ble_rawdata_vector_push(rawdata);   
        }
    }
    #endif    
}


void hx3605_living_Int_handle(void)
{
	#ifdef CHECK_LIVING_LIB
	uint8_t        ii=0;   
	hx3605_living_results_t living_alg_results = {MSG_LIVING_NO_WEAR,0,0,0};

	int32_t *PPG_buf = &(hrs_s_dat.ppg_data[0]);
	uint32_t *als = &(hrs_s_dat.als);
	uint8_t *count = &(hrs_s_dat.count);

	if(hx3605_hrs_read(&hrs_s_dat) == NULL)
	{
		
		return;
	}
	for(ii=0;ii<*count;ii++)
	{
	 //DEBUG_PRINTF("%d/%d %d %d %d %d %d %d\r\n",1+ii,*count,PPG_buf[ii],ir_buf[ii],gsen_fifo_x[ii],gsen_fifo_y[ii],gsen_fifo_z[ii],hrs_s_dat.agc_green);
	}
	hx3605_living_send_data(PPG_buf, *count, *als, gsen_fifo_x, gsen_fifo_y, gsen_fifo_z);   
	living_alg_results = hx3605_living_get_results(); 
		//DEBUG_PRINTF("%d %d %d %d\r\n" ,living_alg_results.data_cnt,living_alg_results.motion_status,living_alg_results.signal_quality,living_alg_results.wear_status);
	#endif
}
#endif


#ifdef SPO2_ALG_LIB
spo2_sensor_data_t spo2_s_dat;
void hx3605_spo2_ppg_Int_handle(void)
{ 
    uint8_t        ii=0; 
    tyhx_spo2_results_t alg_results = {MSG_SPO2_ALG_NOT_OPEN,0,0,0,0,0,0};
    SPO2_CAL_SET_T cal=get_spo2_agc_status();
		hx3605_spo2_wear_msg_code_t spo2_wear_status = MSG_SPO2_INIT;
    int32_t *red_buf = &(spo2_s_dat.red_data[0]);
    int32_t *ir_buf = &(spo2_s_dat.ir_data[0]);
    uint8_t *count = &(spo2_s_dat.count);
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
      for(ii=0;ii<10;ii++)
      {
        gsen_fifo_x[ii] = vec_red_data[spo2_send_cnt1+ii];;
        gsen_fifo_y[ii] = vec_ir_data[spo2_send_cnt1+ii];;
        gsen_fifo_z[ii] = vec_green_data[spo2_send_cnt1+ii];;
      }
			spo2_send_cnt1 = spo2_send_cnt1+10;
      hx3605_spo2_alg_send_data(red_buf_vec, ir_buf_vec, green_buf_vec, *count, gsen_fifo_x, gsen_fifo_y, gsen_fifo_z);
    #else
			if(hx3605_spo2_read(&spo2_s_dat) == 0)
			{
					return;
			}
			for(ii=0;ii<*count;ii++)
			{
					/*DEBUG_PRINTF("%d/%d %d %d %d %d %d %d %d %d %d\r\n" ,1+ii,*count,\
					red_buf[ii],ir_buf[ii],cal.red_idac,cal.ir_idac,cal.red_cur,cal.ir_cur,gsen_fifo_x[ii],gsen_fifo_y[ii],gsen_fifo_z[ii]); */
			}
			spo2_wear_status = hx3605_spo2_get_wear_status();
			if(spo2_wear_status == MSG_SPO2_WEAR)
			{
				#ifdef GSENSER_DATA
				tyhx_spo2_alg_send_data(red_buf, ir_buf, cal.red_idac, cal.ir_idac, *count, gsen_fifo_x, gsen_fifo_y, gsen_fifo_z); 
				#else
				tyhx_spo2_alg_send_data(red_buf, ir_buf, cal.red_idac, cal.ir_idac, *count, gen_dummy, gen_dummy, gen_dummy); 
				#endif
			}
    #endif      

    alg_results = tyhx_spo2_alg_get_results(); 						
			
/////////////////////////////////////////////////////////////////////////////////////
#ifdef TYHX_DEMO
    oled_dis.refresh_time++;
		if(alg_results.data_cnt<256)
		{
			oled_dis.dis_mode = DIS_SPO2;    
		}
//		if(alg_results.data_cnt>=256 && alg_results.data_cnt<280)
//		{
//			if(alg_results.ir_quality<5)
//			{
//				oled_dis.dis_mode = DIS_WAIT;
//			}
//		}
    if(oled_dis.refresh_time >= 7) //330ms*3 = 990ms ~ 1s
    {
        oled_dis.refresh_flag = 1;
        oled_dis.refresh_time = 0;
        //oled_dis.dis_mode = DIS_SPO2;
				oled_dis.dis_data1 = alg_results.hr_result;
        oled_dis.dis_data2 = alg_results.spo2_result;
    }
#endif
    //SEGGER_RTT_printf(0,"oledata: %d,oledstatus: %d\r\n", alg_results.hr_result,alg_results.alg_status);

    #ifdef HRS_BLE_APP
    {
        rawdata_vector_t rawdata;
        for(ii=0;ii<*count;ii++)
        {
          #ifdef SPO2_DATA_CALI
            ir_data_cali = hx3605_ir_data_cali(ir_buf[ii]);
            red_data_cali = hx3605_red_data_cali(red_buf[ii]);
            rawdata.red_raw_data = red_data_cali;
            rawdata.ir_raw_data = ir_data_cali;
          #else
            rawdata.red_raw_data = red_buf[ii];
            rawdata.ir_raw_data = ir_buf[ii];
          #endif
            rawdata.vector_flag = SPO2_VECTOR_FLAG;
            rawdata.data_cnt = alg_results.data_cnt-*count+ii;
            rawdata.hr_result = alg_results.spo2_result;            
//            rawdata.gsensor_x = gsen_fifo_x[ii];
//            rawdata.gsensor_y = gsen_fifo_y[ii];
//            rawdata.gsensor_z = gsen_fifo_z[ii];
						rawdata.gsensor_x = 0;
            rawdata.gsensor_y = cal.red_idac;
            rawdata.gsensor_z = cal.ir_idac;
            rawdata.red_cur = cal.red_cur;
            rawdata.ir_cur = cal.ir_cur;
            ble_rawdata_vector_push(rawdata);                  
        }
    }
    #endif    
}
#endif

#ifdef HRV_ALG_LIB
hrv_sensor_data_t hrv_s_dat;
void hx3605_hrv_ppg_Int_handle(void)
{
    uint8_t        ii=0;
    hrv_results_t alg_hrv_results= {MSG_HRV_ALG_NOT_OPEN,0,0,0,0,0};
    int32_t *PPG_buf = &(hrv_s_dat.ppg_data[0]);
    uint8_t *count = &(hrv_s_dat.count);
	  hx3605_hrv_wear_msg_code_t wear_mode;

    #ifdef HRV_TESTVEC
		int32_t hrm_raw_data;
    hrm_raw_data = vec_data[vec_data_cnt];
    vec_data_cnt++;
    alg_hrv_results = hx3605_hrv_alg_send_data(hrm_raw_data, 0, 0);
    #else
		if(hx3605_hrv_read(&hrv_s_dat) == NULL)
		{
				return;
		}
		for(ii=0;ii<*count;ii++)
		{
				//DEBUG_PRINTF("%d/%d %d\r\n" ,1+ii,*count,PPG_buf[ii]);
		}
		wear_mode = hx3605_hrv_get_wear_status();
		if(wear_mode==MSG_HRV_WEAR)
		{
				alg_hrv_results = tyhx_hrv_alg_send_bufdata(PPG_buf, *count, 0);
		}
		#endif
		
#ifdef TYHX_DEMO
    //display part                                           
    oled_dis.refresh_time++;
    if(oled_dis.refresh_time >= 10)
    {
        oled_dis.refresh_flag = 1;
        oled_dis.refresh_time = 0;
        oled_dis.dis_mode = DIS_HRV;   
        oled_dis.dis_data1 = alg_hrv_results.hrv_result; //alg_hrv_results.hrv_peak;
				oled_dis.dis_data2 = alg_hrv_results.hrv_result;			
    }
#endif
	
    #ifdef HRS_BLE_APP
    {
        rawdata_vector_t rawdata;
        
        HRS_CAL_SET_T cal= get_hrs_agc_status();
        for(ii=0;ii<*count;ii++)
        {
            rawdata.vector_flag = HRS_VECTOR_FLAG;
            rawdata.data_cnt = 0;
            rawdata.hr_result = alg_hrv_results.hrv_result;           
            rawdata.red_raw_data = PPG_buf[ii];
            rawdata.ir_raw_data = 0;
            rawdata.gsensor_x = gsen_fifo_x[ii];
            rawdata.gsensor_y = gsen_fifo_y[ii];
            rawdata.gsensor_z = gsen_fifo_z[ii];
            rawdata.red_cur = cal.led_cur;
            rawdata.ir_cur = 0;
            ble_rawdata_vector_push(rawdata);   
        }
    }
    #endif  
}
#endif

void hx3605_wear_ppg_Int_handle(void)
{
	//DEBUG_INFO("hx3605_wear_ppg_Int_handle");
	
	#ifdef CHECK_TOUCH_LIB
	int32_t ir_buf[32]={0};
	uint8_t count = 0;
	uint8_t ii = 0;
	hx3605_wear_msg_code_t hx3605_check_mode_status = MSG_NO_WEAR;
	count = hx3605_read_fifo_data(ir_buf,1,1);
	hx3605_check_mode_status = hx3605_check_touch(ir_buf,count);
	for(ii=0;ii<count;ii++)
	{
		//DEBUG_PRINTF("%d/%d %d %d\r\n",ii+1,count,ir_buf[ii],hx3605_check_mode_status);
	}
	#endif
}

void hx3605_lab_test_Int_handle(void)
{
		#ifdef LAB_TEST
			#if defined(TIMMER_MODE)
			uint32_t data_buf[4];
			hx3605_lab_test_read_packet(data_buf);
			//DEBUG_PRINTF("%d %d %d %d\r\n", data_buf[0], data_buf[1], data_buf[2], data_buf[3]);
			#else
			uint8_t count = 0;
			int32_t buf[32];
			count = hx3605_read_fifo_data(buf, 2, 1);
			for (uint8_t i=0; i<count; i++) 
			{
				//DEBUG_PRINTF("%d/%d %d %d\r\n" ,1+i,count,buf[i*2],buf[i*2+1]);         
			}
			#endif
		#endif
}
