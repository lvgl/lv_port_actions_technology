
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>

#include "hx3690l.h"
#include "hx3690l_hrs_agc.h"
#include "tyhx_hrs_alg.h"

#ifdef TYHX_DEMO
#include "twi_master.h"
#include "SEGGER_RTT.h"
#endif

#ifdef HRS_ALG_LIB
extern const uint8_t  hx3690l_hrs_agc_idac;
extern const uint8_t  green_led_max_init;
extern uint8_t low_vbat_flag;

//HRS_INFRARED_THRES
extern const int32_t  hrs_ir_unwear_thres; 
extern const int32_t  hrs_ir_wear_thres; 

static uint8_t s_ppg_state = 0;
static uint8_t s_cal_state = 0;
//static int32_t s_buf[64] = {0}; 
static int32_t agc_buf[4] = {0};

static uint8_t cal_delay = CAL_DELAY_COUNT;
static HRS_CAL_SET_T  calReg;
//
static hrs_wear_msg_code_t hrs_wear_status = MSG_HRS_NO_WEAR;
static hrs_wear_msg_code_t hrs_wear_status_pre = MSG_HRS_NO_WEAR;

static uint8_t no_touch_cnt = 0;

void Init_hrs_PPG_Calibration_Routine(HRS_CAL_SET_T *calR)
{
    calR->flag = CAL_FLG_LED_DR|CAL_FLG_LED_DAC|CAL_FLG_AMB_DAC|CAL_FLG_RF;
    
    calR->LEDDAC = 0;   /* 0~127 = 0 ~ 32ua , step = 0.25ua */
    calR->AMBDAC = 0;   /* 0~127 = 0 ~ 32ua , step = 0.25ua */
    calR->RF = 0;       /* 0= 10K; 1= 20k; 2= 50k; 3= 100k; 4= 150k; 5= 200k; 6= 500k; 7= 1M*/
    calR->LED = HRS_CAL_INIT_LED;
    calR->state = hrsCalStart;
    calR->int_cnt = 0;
    calR->cur255_cnt =0;
    if(low_vbat_flag==1)
    {
       calR->led_max_cur = 128;
    }
    else
    {
       calR->led_max_cur = green_led_max_init;
    }
}

void Restart_hrs_PPG_Calibration_Routine(HRS_CAL_SET_T *calR)
{
    calR->flag = CAL_FLG_LED_DAC;
    calR->state = hrsCalLed;
    calR->int_cnt = 0;
}

hrs_wear_msg_code_t hx3690_hrs_wear_mode_check(int32_t infrared_data)
{
    if(infrared_data > hrs_ir_wear_thres)
    {
        if(no_touch_cnt < NO_TOUCH_CHECK_NUM)
        {
            no_touch_cnt++; 
        }  
        if(no_touch_cnt >= NO_TOUCH_CHECK_NUM)  
        {
            hrs_wear_status = MSG_HRS_WEAR;
        }   
    }
    else if(infrared_data < hrs_ir_unwear_thres)
    {
        if(no_touch_cnt>0)
        {
            no_touch_cnt--;
        }
        if(no_touch_cnt == 0)
        {
            hrs_wear_status = MSG_HRS_NO_WEAR;                
        }      
    }
    
   /* AGC_LOG("hrs wearstates: hrs_wear_status_pre=%d,hrs_wear_status=%d\r\n",\
            hrs_wear_status_pre,hrs_wear_status);*/

    if(hrs_wear_status_pre != hrs_wear_status)
    {
        hrs_wear_status_pre = hrs_wear_status;
        if(hrs_wear_status_pre == MSG_HRS_NO_WEAR)
        {
            hx3690l_hrs_low_power();                 
        }
        else if(hrs_wear_status_pre == MSG_HRS_WEAR)
        {
            tyhx_hrs_alg_open_deep();
            hx3690l_hrs_set_mode(PPG_INIT);
            hx3690l_hrs_set_mode(CAL_INIT);
        }  
    }    
    return hrs_wear_status;
}
void PPG_hrs_Calibration_Routine(HRS_CAL_SET_T *calR, int32_t led, int32_t amb)
{
    switch(calR->state)
    {
		case hrsCalStart:
			calR->state = hrsCalLedCur;
			break;
				
        case hrsCalLedCur:
			if(amb>1060000)
			{
				calR->AMBDAC = (amb-1060000)/2600;
			}
			else
			{
				calR->AMBDAC = 0;
			}
            if(led>amb+256)
            {             
				calR->led_step = (led-amb)/HRS_CAL_INIT_LED;
				calR->LED = 2600*(hx3690l_hrs_agc_idac+calR->AMBDAC)/calR->led_step;
				if(calR->LED>calR->led_max_cur)
				{
				calR->LED = calR->led_max_cur;
				}
				if(calR->LED != calR->led_max_cur)
				{
				calR->LEDDAC = (hx3690l_hrs_agc_idac*3)>>2;
				}
				else
				{
				calR->LEDDAC = hx3690l_hrs_agc_idac>>1;
				}
				calR->RF = 6;
				calR->flag = CAL_FLG_LED_DR|CAL_FLG_LED_DAC|CAL_FLG_AMB_DAC|CAL_FLG_RF;           
				calR->state = hrsCalLed;
            }          
            else
            {
				calR->state = hrsCalFinish;
            }
            break;
						
        case hrsCalLed:   
            if(led>1300000 && calR->LEDDAC<=120)
            {
				calR->LEDDAC = calR->LEDDAC + 2;
				calR->state = hrsCalLed;
            }
            else if(led<700000 && calR->LEDDAC>=2)
            {
				calR->LEDDAC = calR->LEDDAC - 2;
				calR->state = hrsCalLed;
            }
			else if(led<700000 && calR->LEDDAC>0)
			{
				calR->LEDDAC = calR->LEDDAC - 1;
				calR->state = hrsCalLed;
			}
            else
            {
              calR->state = hrsCalLed2;
            }    
            calR->flag = CAL_FLG_LED_DAC;            
            break;
						
       case hrsCalLed2:   
            if(led+calR->LEDDAC*130000 < (amb+calR->AMBDAC*130000)+((130000*hx3690l_hrs_agc_idac*3)>>2) && calR->LED < calR->led_max_cur)
            {
              calR->LED = calR->led_max_cur;
              calR->state = hrsCalLed;
              calR->flag = CAL_FLG_LED_DR;
            }
            else
            {
              calR->state = hrsCalFinish;
              calR->flag = 0;
            }              
            break;

        default:
            break;
        
    }
    /*AGC_LOG("AGC: led_drv=%d,ledDac=%d,ambDac=%d,ledstep=%d,rf=%d,\r\n",\
            calR->LED, calR->LEDDAC, calR->AMBDAC,calR->led_step,calR->RF);*/
}

HRS_CAL_SET_T PPG_hrs_agc(void)
{
    int32_t led_val, amb_val;
    calReg.work = false;
    if (!s_cal_state) 
    {
        return  calReg;
    } 
	#ifdef INT_MODE    
    calReg.int_cnt ++;
    if(calReg.int_cnt < 8)
    {
		return calReg;
    }
    calReg.int_cnt = 0;
	hx3690l_gpioint_cfg(false);
	#endif   
    calReg.work = true;       

    read_hrs_data_packet(agc_buf);
    led_val = agc_buf[0];
    amb_val = agc_buf[1];
    
    /*AGC_LOG("cal dat ch1=%d,ch2=%d,led_val=%d,amb_val=%d \r\n", \
    agc_buf[0], agc_buf[1], led_val, amb_val);*/
    
    PPG_hrs_Calibration_Routine(&calReg, led_val, amb_val);
    
    if (calReg.state == hrsCalFinish) 
	{
        hx3690l_hrs_set_mode(CAL_OFF);
		#if defined(TIMMER_MODE)
		#else
		hx3690l_gpioint_cfg(true);
		#endif
    } 
	else
	{
		hx3690l_hrs_updata_reg();
		#if defined(INT_MODE)
		hx3690l_gpioint_cfg(true);
		#endif 
    }
    return  calReg;
}

void hx3690l_hrs_cal_init(void)
{
	uint16_t sample_rate = 200;
	uint32_t prf_clk_num = 32000/sample_rate;
	uint8_t samp_copy_avg = 0;    /* 0=1, 1=2, 2=4 ,3=8, 4=16*/

	hx3690l_write_reg(0X1a, (uint8_t)prf_clk_num); 
	hx3690l_write_reg(0X1b, (uint8_t)(prf_clk_num>>8));
	hx3690l_write_reg(0X1c, (uint8_t)(prf_clk_num>>16));
	hx3690l_write_reg(0X29, samp_copy_avg);

	hx3690l_write_reg(0x13,0x10); //FIFO bypass mode enable
	hx3690l_write_reg(0x23,0x20); //phase4 convertion ready enable

	#if defined(INT_MODE)
	hx3690l_write_reg(0x24,0x00);
	#endif

	hx3690l_write_reg(0x51,0x02); //Chip reset
	hx3690l_delay_ms(5);          //Delay for reset time
	hx3690l_write_reg(0x51,0x00); //Chip state machine work normal
	hx3690l_delay_ms(10);         //Delay for reset time
}

void hx3690l_hrs_cal_off(void)
{
	uint16_t sample_rate = 25; 
	uint32_t prf_clk_num = 32000/sample_rate;
	uint8_t samp_copy_avg = 0;    /* 0=1, 1=2, 2=4 ,3=8, 4=16*/

	hx3690l_write_reg(0X1a, (uint8_t)prf_clk_num);
	hx3690l_write_reg(0X1b, (uint8_t)(prf_clk_num>>8));
	hx3690l_write_reg(0X1c, (uint8_t)(prf_clk_num>>16));
	hx3690l_write_reg(0X29, samp_copy_avg);
	hx3690l_write_reg(0x13,0x11); //FIFO mode enable
	hx3690l_write_reg(0x23,0x00); //phase convertion ready disable
	#if defined(INT_MODE) 
	hx3690l_write_reg(0x24,0x20); 
	#endif

	hx3690l_write_reg(0x51,0x02); //Chip reset
	hx3690l_delay_ms(5);          //Delay for reset time
	hx3690l_write_reg(0X13, 0x10);
	hx3690l_delay_ms(5);
	hx3690l_write_reg(0X13, 0x11);
	hx3690l_write_reg(0x51,0x00); //Chip state machine work normal
	hx3690l_delay_ms(5);
}

void read_hrs_data_packet(int32_t *buf) 
{
    uint8_t dataBuf[6];
    
    hx3690l_brust_read_reg(0x03, dataBuf, 3); 
    hx3690l_brust_read_reg(0x0c, dataBuf+3, 3);    
    
    for (uint8_t i=0; i<2; i++) 
    {
        buf[i] = (int32_t)(dataBuf[3*i]|(dataBuf[3*i+1]<<8)|(dataBuf[3*i+2]<<16));
    }
}

void read_hrs_ir_packet(int32_t *buf)
{
    uint8_t dataBuf[6];
    
    hx3690l_brust_read_reg(0x06, dataBuf, 6);
    
    for (uint8_t i=0; i<2; i++) 
    {
        buf[i] = (int32_t)(dataBuf[3*i]|(dataBuf[3*i+1]<<8)|(dataBuf[3*i+2]<<16));
    }
}

void hx3690l_hrs_read_data_packet(int32_t *buf) 
{
	uint8_t  databuf1[6] = {0};
	uint8_t  databuf2[6] = {0};
	uint32_t P1 = 0,P2 = 0 ,P3 = 0 ,P4 =0 ;

	hx3690l_brust_read_reg(0x03, databuf1, 6); 
	hx3690l_brust_read_reg(0x09, databuf2, 6); 		

	P1 = ((databuf1[0])|(databuf1[1]<<8)|(databuf1[2]<<16)); 
	P3 = ((databuf1[3])|(databuf1[4]<<8)|(databuf1[5]<<16)); 
	P4 = ((databuf2[0])|(databuf2[1]<<8)|(databuf2[2]<<16));	
	P2 = ((databuf2[3])|(databuf2[4]<<8)|(databuf2[5]<<16));
 
	buf[0] = P1;
	buf[1] = P2;
	buf[2] = P3;
	buf[3] = P4;
}

void hx3690l_hrs_low_power(void)
{   
	uint16_t sample_rate = 10;                        
	uint8_t data_avg_num = 0;         						
	uint32_t prf_clk_num = 32000/sample_rate;      
	uint8_t phase1_tia_res = 0;     	
	uint8_t phase1_offset_idac = 0; 
	uint8_t phase1_ldr_cur = 0; 
	uint8_t phase1_led_en = 0;   
	uint8_t phase2_led_en = 0;
	uint8_t phase3_led_en = 1;
	uint8_t phase4_led_en = 0; 
	uint8_t phase2_tia_res = 0; 	
	uint8_t phase3_tia_res = 0; 
	uint8_t phase4_tia_res = 0;    
	uint8_t phase3_ldr_cur = 64;   
	uint8_t led_on_time = 1;

	hx3690l_write_reg(0X1a, (uint8_t)prf_clk_num); 
	hx3690l_write_reg(0X1b, (uint8_t)(prf_clk_num>>8));
	hx3690l_write_reg(0X1c, (uint8_t)(prf_clk_num>>16));
	hx3690l_write_reg(0X3c, data_avg_num<<4 | data_avg_num );
	hx3690l_write_reg(0X3d, data_avg_num<<4 | data_avg_num );	    
	hx3690l_write_reg(0X2d, phase3_tia_res);
	hx3690l_write_reg(0X2e, phase4_tia_res);
	hx3690l_write_reg(0X2c, phase1_tia_res); 
	hx3690l_write_reg(0X2f, phase2_tia_res);
	hx3690l_write_reg(0X38, phase1_offset_idac);
	hx3690l_write_reg(0X30, phase1_ldr_cur);   
	hx3690l_write_reg(0X31, phase3_ldr_cur);    
	hx3690l_write_reg(0X1f, (led_on_time<<4| phase1_led_en<<3 | phase3_led_en<<2 | phase4_led_en<<1 | phase2_led_en) );	
	hx3690l_write_reg(0X69, 0xa2);	//bit<0>: rc_comb_en bits<1>=rc_rbp_en bits<7>= vcom_clamp_en bits<6:4>= LED_vdesl

  #ifdef INT_MODE
	hx3690l_write_reg(0X12,0x14); //fifo almostfull cfg ,max=0x40;
	hx3690l_write_reg(0x13,0x31); //FIFO bypass mode enable
	hx3690l_write_reg(0x23,0x00); //phase3 convertion ready disable
  #else
    hx3690l_write_reg(0x13,0x11); //FIFO bypass mode enable
    hx3690l_write_reg(0x23,0x00); //phase3 convertion ready disable
  #endif

    hx3690l_write_reg(0x51,0x02); //Chip reset
    hx3690l_delay_ms(5);             //Delay for reset time
    hx3690l_write_reg(0x51,0x00); //Chip state machine work normal
	hx3690l_delay_ms(5);
    calReg.LED = 0;

    AGC_LOG(" chip go to low power mode  \r\n" );   
}

void hx3690l_hrs_updata_reg(void)
{
    if (calReg.flag & CAL_FLG_LED_DR) 
    {
        hx3690l_write_reg(0X30, calReg.LED);     // phase1 led driver config
    }
    
    if (calReg.flag & CAL_FLG_LED_DAC) 
    {
        hx3690l_write_reg(0X38, calReg.LEDDAC);  // phase1 offset idac cfg
    }
    
    if (calReg.flag & CAL_FLG_AMB_DAC) 
    {
        hx3690l_write_reg(0X3b, calReg.AMBDAC);  // phase2 offset idac cfg
    }
    
    if (calReg.flag & CAL_FLG_RF) 
    {
        hx3690l_write_reg(0X2c, calReg.RF);    // phase1 tia feed back resister cfg
        hx3690l_write_reg(0X2f, calReg.RF);    // phase2 tia feed back resister cfg
    }
}

void hx3690l_hrs_set_mode(uint8_t mode_cmd)
{
    switch (mode_cmd) 
    {
        case PPG_INIT:
            hx3690l_hrs_ppg_init();
            #if defined(TIMMER_MODE)
            hx3690l_320ms_timer_cfg(true);
                #if defined(GSEN_40MS_TIMMER)
                hx3690l_40ms_timer_cfg(true);
                #endif
            #else
            hx3690l_gpioint_cfg(true);
            #endif
            s_ppg_state = 1;
            AGC_LOG("ppg init mode\r\n");
            break;

        case PPG_OFF:
            hx3690l_ppg_off();
            s_ppg_state = 0;
            AGC_LOG("ppg off mode\r\n");
            break;
				
        case PPG_LED_OFF:
            hx3690l_hrs_low_power();
            s_ppg_state = 0;
            AGC_LOG("ppg led off mode\r\n");
            break;

        case CAL_INIT:
            Init_hrs_PPG_Calibration_Routine(&calReg);
            hx3690l_hrs_cal_init();
            hx3690l_hrs_updata_reg();
            #if defined(TIMMER_MODE)
                #if defined(GSEN_40MS_TIMMER)
                #else
                hx3690l_40ms_timer_cfg(true);
                #endif
            #endif 
            s_cal_state = 1;
            AGC_LOG("cal init mode\r\n");   
            break;   
				
        case RECAL_INIT:        
            Restart_hrs_PPG_Calibration_Routine(&calReg);
            hx3690l_hrs_cal_init();
            hx3690l_hrs_updata_reg();
            #if defined(TIMMER_MODE)
                #if defined(GSEN_40MS_TIMMER)
                #else
                hx3690l_40ms_timer_cfg(true);
                #endif
            #endif 
            s_cal_state = 1;
            AGC_LOG("Recal init mode\r\n");
            break;

        case CAL_OFF:
            #if defined(TIMMER_MODE)
                #if defined(GSEN_40MS_TIMMER)
                #else
                hx3690l_40ms_timer_cfg(false);
                #endif
            #endif
            hx3690l_hrs_cal_off();
            s_cal_state = 0;
            AGC_LOG("cal off mode\r\n");
            break;

        default:
            break;
    }
}

SENSOR_ERROR_T hx3690l_hrs_enable(void)
{
    if (!hx3690l_chip_check()) 
    {
        AGC_LOG("hx3690l check id failed!\r\n");
        return SENSOR_OP_FAILED;
    }

    AGC_LOG("hx3690l check id success!\r\n");

    if (s_ppg_state) 
    {
        AGC_LOG("ppg already on!\r\n");
        return SENSOR_OP_FAILED;
    }
    if(!tyhx_hrs_alg_open())
    {
        AGC_LOG("hrs alg open fail,or dynamic ram not enough!\r\n");
    }
    
    hrs_wear_status = MSG_HRS_NO_WEAR;
    hrs_wear_status_pre = MSG_HRS_NO_WEAR;
    
    hx3690l_hrs_set_mode(PPG_INIT);

    AGC_LOG("hx3690l enable!\r\n");

    return SENSOR_OK;
}

void hx3690l_hrs_disable(void)
{
	#if defined(TIMMER_MODE)
	hx3690l_320ms_timer_cfg(false);
	hx3690l_40ms_timer_cfg(false);
	#elif defined(INT_MODE)
	hx3690l_gpioint_cfg(false);
	#endif	
	hx3690l_hrs_set_mode(PPG_OFF);
	s_ppg_state = 0;
	s_cal_state = 0;
	tyhx_hrs_alg_close();
	AGC_LOG("hx3690l disable!\r\n");
}

void hx3690l_hrs_data_reset(void)
{
	s_ppg_state = 0;
	s_cal_state = 0;
	tyhx_hrs_alg_close();
}

hrs_wear_msg_code_t hx3690_hrs_get_wear_status(void)
{
    return  hrs_wear_status;
}

HRS_CAL_SET_T get_hrs_agc_status(void)
{
	HRS_CAL_SET_T cal;
	cal.flag     =  calReg.flag;
	cal.int_cnt  =  calReg.int_cnt;
	cal.cur255_cnt = calReg.cur255_cnt;
	cal.led_idac =  calReg.led_idac;
	cal.LED      =  calReg.LED;     
	cal.LEDDAC   =  calReg.LEDDAC;  
	cal.AMBDAC   =  calReg.AMBDAC;  
	cal.RF       =  calReg.RF;      
	cal.led_step =  calReg.led_step;
	cal.state    =  calReg.state;
	cal.led_max_cur = calReg.led_max_cur;
	cal.work     =  calReg.work;
	return cal;
}

void hx3690l_hrs_read_fifo_data(uint8_t read_fifo_size,int32_t *buf)
{
    uint8_t data_flg = 127;
    int32_t data;
    uint8_t databuf[3];
    uint8_t ii=0;
    for(ii=0; ii<read_fifo_size; ii++) 
    {
        hx3690l_write_reg(0x17, 0x00); // write any walue to 0x17 will update a new data
        hx3690l_delay_us(100);
        hx3690l_brust_read_reg(0x15, databuf, 3); 
        data_flg = databuf[2]>>5;
        data = (int32_t)(databuf[0]|(databuf[1]<<8)|((databuf[2]&0x1f)<<16));

        if(ii==0)
		{
			if(data_flg ==3)
			{
				ii=3;
				buf[0] = 0;
				buf[1] = 0;
				buf[2] = 0;
			}
			if(data_flg ==2)
			{      
				ii=2;
				buf[0] = 0;
				buf[1] = 0;
			}
			if(data_flg ==1)
			{    
				ii=1;
				buf[0] = 0;
			}			
		}
		
        if(data_flg == 0) 
        {
            buf[ii]= data;
        } 
        else if(data_flg == 1)
        {
            buf[ii]= data;
        } 
        else if(data_flg == 2)
        {
            buf[ii]= data;
        } 
        else if(data_flg == 3) 
        {
            buf[ii]= data;
        }
    }
}

uint8_t hx3690l_hrs_read(ppg_sensor_data_t * s_dat)
{
    uint8_t i = 0;
	int32_t PPG_src_data = 0;
	int32_t Ir_src_data = 0;
	bool recal = false;
    uint8_t size = 0;
	uint8_t fifo_out_size = 0;
	int32_t *PPG_buf =  &(s_dat->green_data[0]);
	int32_t *ir_buf =  &(s_dat->ir_data[0]);  
	int32_t *s_buf =  &(s_dat->s_buf[0]);   
	s_dat->green_cur =  calReg.LED;
	int32_t ps_data[4];

    if (!s_ppg_state || s_cal_state) 
    {
        return 0;
    }    
    
    size = hx3690l_read_fifo_size();
    //AGC_LOG("ppg data size: %d\r\n", size_byte);
    
    if(size<2)
    {
        return 0;
    }
		
	hx3690l_hrs_read_data_packet(ps_data);
	Ir_src_data = ps_data[2]-ps_data[3];
    ir_buf[0] = Ir_src_data;
    
	hx3690_hrs_wear_mode_check(Ir_src_data);
	//AGC_LOG("%d\r\n", Ir_src_data);
    if (size && size <= 64) 
    {
        fifo_out_size = hx3690l_read_fifo_data(s_buf,size,2);
        s_dat->count = fifo_out_size;
        for (i=0; i<fifo_out_size; i++) 
        {
            PPG_src_data = s_buf[i*2] - s_buf[i*2+1];            
            if (s_buf[i*2]<500000 || s_buf[i*2]>1500000)            
            {
                recal = true;
               
                if(hrs_wear_status==MSG_HRS_NO_WEAR)
                {
                     recal = false;
                }                
            }

            PPG_buf[i] = PPG_src_data;

            AGC_LOG("%d/%d %d %d %d %d %d\r\n" ,1+i,size,PPG_buf[i],ir_buf[0],s_buf[i*2],s_buf[i*2+1],calReg.LED);         
        }

        if (recal) 
        {
            cal_delay--;

            if (cal_delay <= 0) 
            {
                cal_delay = CAL_DELAY_COUNT;
                hx3690l_hrs_set_mode(RECAL_INIT);
            }
        }
        else                       
        {
            cal_delay = CAL_DELAY_COUNT;
        }
    }    
    return 1;
}


#endif
