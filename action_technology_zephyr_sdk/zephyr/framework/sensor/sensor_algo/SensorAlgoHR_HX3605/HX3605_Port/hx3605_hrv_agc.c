#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>


#include "hx3605.h"
#include "hx3605_hrv_agc.h"
#include "tyhx_hrv_alg.h"

#ifdef TYHX_DEMO
#include "twi_master.h"
#include "SEGGER_RTT.h"
#endif

#ifdef HRV_ALG_LIB

extern const uint8_t  hx3605_hrs_agc_idac;
extern const uint8_t  green_led_max_init;
extern uint8_t low_vbat_flag;
extern uint8_t read_fifo_first_flg;

//hrv_INFRARED_THRES
extern const int32_t  hrv_ir_unwear_thres; 
extern const int32_t  hrv_ir_wear_thres; 

static uint8_t s_ppg_state = 0;
static uint8_t s_cal_state = 0; 
static int32_t agc_buf[4] = {0};

static uint8_t cal_delay = CAL_DELAY_COUNT;
static HRV_CAL_SET_T  calReg;
//
static hx3605_hrv_wear_msg_code_t hrv_wear_status = MSG_HRV_NO_WEAR;
static hx3605_hrv_wear_msg_code_t hrv_wear_status_pre = MSG_HRV_NO_WEAR;

static uint8_t no_touch_cnt = 0;

void hx3605_hrv_ppg_init(void) //20200615 ericy ppg fs=25hz, phase3 conversion ready interupt en
{
		uint16_t sample_rate = 125;                  /*config the data rate of chip alps2_fm ,uint is Hz*/
    uint32_t prf_clk_num = 32000/sample_rate;   /*period in clk num, num = Fclk/fs */
    uint8_t ps0_enable=1;       /*ps0_enable  , 1 mean enable ; 0 mean disable */
    uint8_t ps1_enable=1;       /*ps1_enable  , 1 mean enable ; 0 mean disable */   
    uint8_t ps0_osr = 3;    /* 0 = 128 ; 1 = 256 ; 2 = 512 ; 3 = 1024 */
    uint8_t ps1_osr = 1;    /* 0 = 128 ; 1 = 256 ; 2 = 512 ; 3 = 1024 */
    
    uint8_t ps1_cp_avg_num_sel= 0;
		uint8_t ps0_cp_avg_num_sel= 1;
		uint8_t ps1_avg_num_sel_i2c=0;
		uint8_t ps0_avg_num_sel_i2c=0;
     /***********led open enable***********/
    uint8_t dccancel_ps0_data1 =0;    //offset idac   BIT[9:0]  0 31.25nA, 1023 32uA, step 31.25nA
    uint8_t dccancel_ps0_data2 =0;    //offset idac   BIT[9:0]  0 31.25nA, 1023 32uA, step 31.25nA
    uint8_t dccancel_ps1_data1 =0;    //offset idac   BIT[9:0]  0 31.25nA, 1023 32uA, step 31.25nA
    uint8_t dccancel_ps1_data2 =0;    //offset idac   BIT[9:0]  0 31.25nA, 1023 32uA, step 31.25nA
    
    uint8_t ir_pden_ps0 =0;     //IR_PDEN_PS0
    uint8_t ext_pden_ps0 =0;    //EXT_PDEN_PS0
    uint8_t pddrive_ps0 =0;     //PDDRIVE_PS0  0-63
	
	  uint8_t ir_pden_ps1 =0;      //IR_PDEN_PS1
    uint8_t ext_pden_ps1 =0;     //EXT_PDEN_PS1
    uint8_t pddrive_ps1 =8;     //PDDRIVE_PS1  0-63
    uint8_t ps1_interval_i2c =0;    // config 0/5  
    
    uint8_t led_en_begin =2;       // 0 = 2 ; 1 = 4 ; 2 = 8 ; 3 = 16 ;
    uint8_t afe_reset = 1;        //* 0 = 32clk ; 1 = 64clk ; 2 = 128clk ; 3 = 256clk(d) ;
    uint8_t en2rst_delay =2;
		uint8_t init_wait_delay = 0; //* 0 = 800clk ; 1 = 1600clk ; 2 = 3200clk ; 3 = 6400clk(d) ;
                                    
		uint16_t thres_level =0;
    uint8_t thres_int =1;    //thres int enable
    uint8_t data_rdy_int =8;    //[3]:ps1_data2 [2]:ps1_data1 [1]:ps0_data2 [0]:ps0_data1
    
    uint8_t ldrsel_ps1 =IR_LED_SLE;      //ps1 LDR SELECT 01:ldr0-IR 02:ldr1-RLED  04:ldr2-GLED
    uint8_t ldrsel_ps0 =GREEN_LED_SLE;      //ps0 LDR SELECT 01:ldr0-IR 02:ldr1-RLED  04:ldr2-GLED
    /***********cap *********/
    uint8_t intcapsel_ps1 =1;   //01=4fp 02=8pf 03=12pf 04=16pf 05=20pf 06=24pf 07=28pf 08=32pf 09=36pf 10=40pf 11=44pf 12=48pf 13=52pf 14=56pf 15=60pf 
    uint8_t intcapsel_ps0 =1;   //01=4fp 02=8pf 03=12pf 04=16pf 05=20pf 06=24pf 07=28pf 08=32pf 09=36pf 10=40pf 11=44pf 12=48pf 13=52pf 14=56pf 15=60pf 
    
    uint8_t led_on_time  = 32;      /* 0 = 32clk2=8us ; 8 = 64clk=16us; 16=128clk=32us ; 32 = 256clk=64us ;
                                     64 = 512clk=128us ; 128= 1024clk=256us; 256= 2048clk=512us*/
    uint8_t force_adc_clk_sel=0;
    uint8_t force_adc_clk_cfg =0;
    uint8_t force_PEN =0;     //phase enable
    uint8_t force_PEN_cfg =0;
    uint8_t force_LED_EN =0;
    uint8_t force_LED_EN_cfg =0;         
    uint8_t force_CKAFEINT_sel =0; 
    uint8_t force_CKAFEINT_cfg =0;
    
    uint8_t PDBIASEN =0;
    uint8_t PDILOADEN =0;

		hx3605_write_reg(0x01, 0x01);
		hx3605_delay(5);
		hx3605_write_reg(0x01, 0x00);
		hx3605_delay(5);
		
    hx3605_write_reg(0X10, (ps1_enable<<5| ps0_enable<<4|ps1_osr<<2|ps0_osr));	//default 0x00
    hx3605_write_reg(0X11, (uint8_t)prf_clk_num);    // prf bit<7:0>6   default 0x00
    hx3605_write_reg(0X12, (uint8_t)(prf_clk_num>>8)); // prf bit<15:8>  default 0x03
    hx3605_write_reg(0X13, (ps1_interval_i2c));   //default 0x00
    hx3605_write_reg(0X14, (led_en_begin<<6|afe_reset<<4|en2rst_delay<<2|init_wait_delay));  //default 0x00   
    hx3605_write_reg(0X15, led_on_time); // led_en_num*8     //default 0x00
    hx3605_write_reg(0X16, (ps1_cp_avg_num_sel<<4)|(ps0_cp_avg_num_sel));  //default 0x00
    hx3605_write_reg(0X17, (ps1_avg_num_sel_i2c<<2)|(ps0_avg_num_sel_i2c));   //default 0x00 
    
    hx3605_write_reg(0X18, (uint8_t)dccancel_ps0_data1);     //default 0x00
    hx3605_write_reg(0X19, (uint8_t)(dccancel_ps0_data1>>8));   //default 0x00
    hx3605_write_reg(0X1a, (uint8_t)dccancel_ps0_data2);   //default 0x00
    hx3605_write_reg(0X1b, (uint8_t)(dccancel_ps0_data2>>8));  //default 0x00
    hx3605_write_reg(0X1c, (uint8_t)dccancel_ps1_data1);   //default 0x00
    hx3605_write_reg(0X1d, (uint8_t)(dccancel_ps1_data1>>8));   //default 0x00
    hx3605_write_reg(0X1e, (uint8_t)dccancel_ps1_data2 );   //default 0x00
    hx3605_write_reg(0X1f, (uint8_t)(dccancel_ps1_data2 >>8));   //default 0x00
    
    hx3605_write_reg(0X20, (ir_pden_ps0<<7|ir_pden_ps0<<6|pddrive_ps0));  //default 0x00
    hx3605_write_reg(0X21, (ir_pden_ps1<<7|ir_pden_ps1<<6|pddrive_ps1));  //default 0x00
    hx3605_write_reg(0X22, (ldrsel_ps1<<4|ldrsel_ps0));      //default 0x00
    hx3605_write_reg(0X23, (intcapsel_ps1<<4|intcapsel_ps0));   //default 0x00
    hx3605_write_reg(0X26, (thres_int<<4|data_rdy_int));   //default 0x0f
    
    //    hx3605_write_reg(0X34, 0X10);  //w_almost_full ENABLE   /default0x04
    
    hx3605_write_reg(0X69, (force_adc_clk_sel<<7|force_adc_clk_cfg<<6|force_PEN<<5|force_PEN_cfg<<4|force_LED_EN<<3|
                             force_LED_EN_cfg<<2|force_CKAFEINT_sel<<1|force_CKAFEINT_cfg));   //default 0x00
    hx3605_write_reg(0Xc2, 0x0b);                         
    hx3605_write_reg(0Xc3, (PDBIASEN<<5|PDILOADEN<<4));   //default 0x27
#ifdef INT_FLOATING
		hx3605_write_reg(0xc0, 0x08);
#endif     
		 
#if defined(TIMMER_MODE)
        hx3605_write_reg(0x2d,0x84);     //bits<3:0> fifo data sel, 0000 = p1;0001= p2;0010=p3;0011=p4;bits<7> fifo enble
				hx3605_write_reg(0x2e,0x20);			 //watermark
				hx3605_write_reg(0x2c,0x1f);     // int_width_i2c  
				hx3605_write_reg(0X27, 0x00);   // int sel,01=prf int,04=enable almost full int
#else ///////////INT Mode
				hx3605_write_reg(0x2d,0x00);     //bits<3:0> fifo data sel, 0000 = p1;0001= p2;0010=p3;0011=p4;bits<7> fifo enble
				hx3605_write_reg(0x2c,0x1f);     // int_width_i2c  
				hx3605_write_reg(0X27, 0x00);   // int sel,01=prf ,04=enable almost full
#endif
        
    hx3605_write_reg(0X68, 0X01);  //soft reset
    hx3605_delay(5); 
    hx3605_write_reg(0X68, 0X00);  //release
		hx3605_delay(5);
		read_fifo_first_flg = 1;
}

hx3605_hrv_wear_msg_code_t hx3605_hrv_wear_mode_check(int32_t infrared_data)
{ 
    if(infrared_data > hrv_ir_wear_thres)
    {
        if(no_touch_cnt < NO_TOUCH_CHECK_NUM)
        {
            no_touch_cnt++;
        }
        if(no_touch_cnt >= NO_TOUCH_CHECK_NUM)  
        {
            hrv_wear_status = MSG_HRV_WEAR;
        }   
    }
    else if(infrared_data < hrv_ir_unwear_thres)
    {
        if(no_touch_cnt>0)
        {
            no_touch_cnt--;
        }
        if(no_touch_cnt == 0)
        {
            hrv_wear_status = MSG_HRV_NO_WEAR;                
        }      
    }

    if(hrv_wear_status_pre != hrv_wear_status)
    {
        hrv_wear_status_pre = hrv_wear_status;
        if(hrv_wear_status_pre == MSG_HRV_NO_WEAR)
        {
            hx3605_hrv_low_power();                 
        }
        else if(hrv_wear_status_pre == MSG_HRV_WEAR)
        {
            tyhx_hrv_alg_open_deep();
            hx3605_hrv_set_mode(PPG_INIT);
            hx3605_hrv_set_mode(CAL_INIT);
        }  
    }
    
    return hrv_wear_status;
}

void Init_hrv_PPG_Calibration_Routine(HRV_CAL_SET_T *calR,uint8_t led)
{
    calR->flag = CAL_FLG_LED_DR|CAL_FLG_LED_DAC|CAL_FLG_AMB_DAC|CAL_FLG_RF;
    
    calR->led_idac   = 0;   
    calR->amb_idac   = 0; 
    calR->ontime     = 5;
    calR->led_cur    = HRV_CAL_INIT_LED;
    calR->state      = hrvCalStart;
    calR->int_cnt    = 0;
    calR->cur255_cnt = 0;
    if(low_vbat_flag==1)
    {
       calR->led_max_cur = green_led_max_init>>1;
    }
    else
    {
       calR->led_max_cur = green_led_max_init;
    }
}

void Restart_hrv_PPG_Calibration_Routine(HRV_CAL_SET_T *calR)
{
    calR->flag = CAL_FLG_LED_DR|CAL_FLG_LED_DAC|CAL_FLG_AMB_DAC|CAL_FLG_RF;
    calR->state = hrvCalLed;
    calR->int_cnt = 0;
}

void PPG_hrv_Calibration_Routine(HRV_CAL_SET_T *calR, int32_t led, int32_t amb)
{
    switch(calR->state)
    {
				case hrvCalStart:
					  calR->state = hrvLedCur;
            break;
				
        case hrvLedCur:
						if(amb>280000)
						{
							calR->amb_idac = (amb-280000)/10000;
						}
						else
						{
							calR->amb_idac = 0;
						}
            if(led>amb+128)
            {             
              calR->led_step = (led-amb)/HRV_CAL_INIT_LED;
              calR->led_cur = 10000*(hx3605_hrs_agc_idac+calR->amb_idac)/calR->led_step;
              if(calR->led_cur>calR->led_max_cur)
              {
                calR->led_cur = calR->led_max_cur;
              }
              calR->led_idac = 0;
              calR->ontime = 32;
              calR->flag = CAL_FLG_LED_DR|CAL_FLG_LED_DAC|CAL_FLG_AMB_DAC|CAL_FLG_RF;           
              calR->state = hrvCalLed;
            }          
            else
            {
							calR->led_cur = calR->led_max_cur;
              calR->led_idac = 0;
              calR->ontime = 32;
              calR->flag = CAL_FLG_LED_DR|CAL_FLG_LED_DAC|CAL_FLG_AMB_DAC|CAL_FLG_RF;
              calR->state = hrvCalLed;
            }
            break;
						
        case hrvCalLed:   
            if(led>320000 && calR->led_idac<=120)
            {
              calR->led_idac = calR->led_idac + 1;
              calR->state = hrvCalLed;
            }
            else if(led<200000 && calR->led_idac>=1)
            {
              calR->led_idac = calR->led_idac - 1;
              calR->state = hrvCalLed;
            }
            else
            {
							calR->state = hrvCalFinish;
            }    
            calR->flag = CAL_FLG_LED_DAC;            
            break;
				
        default:
            break;
        
    }
    AGC_LOG("AGC: led_drv=%d,ledDac=%d,ambDac=%d,ledstep=%d,ontime=%d,state=%d\r\n",\
            calR->led_cur, calR->led_idac, calR->amb_idac,calR->led_step,calR->ontime,calR->state);
}

HRV_CAL_SET_T PPG_hrv_agc(void)
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
		hx3605_gpioint_cfg(false);
		#endif   
    calReg.work = true;       

    read_data_packet(agc_buf);
    led_val = agc_buf[0];
    amb_val = agc_buf[1];
    
    AGC_LOG("cal dat led_val=%d,amb_val=%d \r\n", led_val, amb_val);
    
    PPG_hrv_Calibration_Routine(&calReg, led_val, amb_val);
    
    if (calReg.state == hrvCalFinish) 
		{
        hx3605_hrv_set_mode(CAL_OFF);
				#if defined(TIMMER_MODE)
				#else
				hx3605_gpioint_cfg(true);
				#endif
    } 
		else
		{
        hx3605_hrv_updata_reg();
				#if defined(INT_MODE)
				hx3605_gpioint_cfg(true);
				#endif 
    }
    return  calReg;
}



void hx3605_hrv_cal_init(void) // 20200615 ericy afe cali online
{
    uint16_t sample_rate = 100;                      /*config the data rate of chip alps2_fm ,uint is Hz*/
    uint32_t prf_clk_num = 32000/sample_rate;        /*period in clk num, num = Fclk/fs */
	  uint8_t ps1_cp_avg_num_sel= 0;
		uint8_t ps0_cp_avg_num_sel= 0;
		uint8_t thres_int =0;    //thres int enable
		uint8_t data_rdy_int =8;    //[3]:ps1_data2 [2]:ps1_data1 [1]:ps0_data2 [0]:ps0_data1
	
    hx3605_write_reg(0X11, (uint8_t)prf_clk_num);    // prf bit<7:0>6   default 0x00
    hx3605_write_reg(0X12, (uint8_t)(prf_clk_num>>8)); // prf bit<15:8>  default 0x03
		hx3605_write_reg(0X16, (ps1_cp_avg_num_sel<<4)|(ps0_cp_avg_num_sel));  //default 0x00
	
		hx3605_write_reg(0x2d,0x00);     //bits<3:0> fifo data sel, 0000 = p1;0001= p2;0010=p3;0011=p4;bits<7> fifo enble
		hx3605_write_reg(0X26, (thres_int<<4|data_rdy_int));   //default 0x0f
  
    hx3605_write_reg(0X68, 0X01);  //soft reset
    hx3605_delay(5); 
    hx3605_write_reg(0X68, 0X00);  //release
		hx3605_delay(5);             //Delay for reset time
}

void hx3605_hrv_cal_off(void) // 20200615 ericy afe cali offline
{
    uint16_t sample_rate = 125;                      /*config the data rate of chip alps2_fm ,uint is Hz*/
    uint32_t prf_clk_num = 32000/sample_rate;        /*period in clk num, num = Fclk/fs */
		uint8_t ps1_cp_avg_num_sel= 0;
		uint8_t ps0_cp_avg_num_sel= 1;
	
	  hx3605_write_reg(0X11, (uint8_t)prf_clk_num);    // prf bit<7:0>6   default 0x00
    hx3605_write_reg(0X12, (uint8_t)(prf_clk_num>>8)); // prf bit<15:8>  default 0x03
		hx3605_write_reg(0X16, (ps1_cp_avg_num_sel<<4)|(ps0_cp_avg_num_sel));  //default 0x00
	
		hx3605_write_reg(0x2d,0x84);     //bits<3:0> fifo data sel, 0000 = p1;0001= p2;0010=p3;0011=p4;bits<7> fifo enble

    hx3605_write_reg(0X68, 0X01);  //soft reset
    hx3605_delay(5); 
    hx3605_write_reg(0X68, 0X00);  //release
		hx3605_delay(5);
		read_fifo_first_flg = 1;
}

void hx3605_hrv_low_power(void)
{   
    uint16_t sample_rate = 10;                      /*config the data rate of chip alps2_fm ,uint is Hz*/
    uint32_t prf_clk_num = 32000/sample_rate;        /*period in clk num, num = Fclk/fs */
	  uint8_t ps0_enable=0;       /*ps0_enable  , 1 mean enable ; 0 mean disable */
    uint8_t ps1_enable=1;       /*ps1_enable  , 1 mean enable ; 0 mean disable */   
    uint8_t ps0_osr = 3;    /* 0 = 128 ; 1 = 256 ; 2 = 512 ; 3 = 1024 */
    uint8_t ps1_osr = 1;    /* 0 = 128 ; 1 = 256 ; 2 = 512 ; 3 = 1024 */
	  uint8_t ps1_cp_avg_num_sel= 0;
		uint8_t ps0_cp_avg_num_sel= 0;
		uint8_t thres_int =1;    //thres int enable
		uint8_t data_rdy_int =0;    //[3]:ps1_data2 [2]:ps1_data1 [1]:ps0_data2 [0]:ps0_data1	
		uint8_t led_on_time  = 5;
		uint8_t ldrsel_ps1 =IR_LED_SLE;      //ps1 LDR SELECT 01:ldr0-IR 02:ldr1-RLED  04:ldr2-GLED
    uint8_t ldrsel_ps0 =0;      //ps0 LDR SELECT 01:ldr0-IR 02:ldr1-RLED  04:ldr2-GLED
		uint8_t ps1_interval_i2c = 0;    // config 0/5
	
	  uint8_t dccancel_ps0_data1 =0;    //offset idac   BIT[9:0]  0 31.25nA, 1023 32uA, step 31.25nA
    uint8_t dccancel_ps0_data2 =0;    //offset idac   BIT[9:0]  0 31.25nA, 1023 32uA, step 31.25nA
    uint8_t dccancel_ps1_data1 =0;    //offset idac   BIT[9:0]  0 31.25nA, 1023 32uA, step 31.25nA
    uint8_t dccancel_ps1_data2 =0;    //offset idac   BIT[9:0]  0 31.25nA, 1023 32uA, step 31.25nA
    uint8_t ir_pden_ps0 =0;     //IR_PDEN_PS0
//    uint8_t ext_pden_ps0 =0;    //EXT_PDEN_PS0
    uint8_t pddrive_ps0 =0;     //PDDRIVE_PS0  0-63
	
	  hx3605_write_reg(0X11, (uint8_t)prf_clk_num);    // prf bit<7:0>6   default 0x00
    hx3605_write_reg(0X12, (uint8_t)(prf_clk_num>>8)); // prf bit<15:8>  default 0x03
		hx3605_write_reg(0X13, (ps1_interval_i2c));   //default 0x00
		hx3605_write_reg(0X10, (ps1_enable<<5| ps0_enable<<4|ps1_osr<<2|ps0_osr));	//default 0x00
		hx3605_write_reg(0X16, (ps1_cp_avg_num_sel<<4)|(ps0_cp_avg_num_sel));  //default 0x00
		hx3605_write_reg(0X20, (ir_pden_ps0<<7|ir_pden_ps0<<6|pddrive_ps0));  //default 0x00
		hx3605_write_reg(0X18, (uint8_t)dccancel_ps0_data1);     //default 0x00
    hx3605_write_reg(0X19, (uint8_t)(dccancel_ps0_data1>>8));   //default 0x00
    hx3605_write_reg(0X1a, (uint8_t)dccancel_ps0_data2);   //default 0x00
    hx3605_write_reg(0X1b, (uint8_t)(dccancel_ps0_data2>>8));  //default 0x00
    hx3605_write_reg(0X1c, (uint8_t)dccancel_ps1_data1);   //default 0x00
    hx3605_write_reg(0X1d, (uint8_t)(dccancel_ps1_data1>>8));   //default 0x00
    hx3605_write_reg(0X1e, (uint8_t)dccancel_ps1_data2 );   //default 0x00
    hx3605_write_reg(0X1f, (uint8_t)(dccancel_ps1_data2 >>8));   //default 0x00
		hx3605_write_reg(0X22, (ldrsel_ps1<<4|ldrsel_ps0));      //default 0x00
		hx3605_write_reg(0X15, led_on_time);
		
		hx3605_write_reg(0X28, 0x02);
//		hx3605_write_reg(0X29, (uint8_t)threl_value);
//		hx3605_write_reg(0X2a, (uint8_t)(threl_value>>8));
		hx3605_write_reg(0X2b, 0x18);  

		hx3605_write_reg(0x2d,0x84);     //bits<3:0> fifo data sel, 0000 = p1;0001= p2;0010=p3;0011=p4;bits<7> fifo enble
		hx3605_write_reg(0X26, (thres_int<<4|data_rdy_int));   //default 0x0f
    hx3605_write_reg(0X68, 0X01);  //soft reset
    hx3605_delay(5); 
    hx3605_write_reg(0X68, 0X00);  //release
		hx3605_delay(5);
    calReg.led_cur = 0;
//		hx3605_ppg_timer_cfg(false);
//    hx3605_agc_timer_cfg(false);
//		hx3605_gpioint_cfg(true);
    AGC_LOG(" chip go to low power mode  \r\n" ); 
}

void hx3605_hrv_updata_reg(void)
{
    if (calReg.flag & CAL_FLG_LED_DR) 
    {
				hx3605_write_reg(0X20, calReg.led_cur);
    }
    
    if (calReg.flag & CAL_FLG_LED_DAC) 
    {
			  hx3605_write_reg(0X18, (uint8_t)calReg.led_idac);  
				hx3605_write_reg(0X19, (uint8_t)(calReg.led_idac>>8));
    }
    
    if (calReg.flag & CAL_FLG_AMB_DAC) 
    {
				hx3605_write_reg(0X1a, (uint8_t)calReg.amb_idac); 
				hx3605_write_reg(0X1b, (uint8_t)(calReg.amb_idac>>8));
    }
    
    if (calReg.flag & CAL_FLG_RF) 
    {
				hx3605_write_reg(0X15, calReg.ontime); 
    }
}

void hx3605_hrv_set_mode(uint8_t mode_cmd)
{
    switch (mode_cmd) 
    {
        case PPG_INIT:
            hx3605_hrv_ppg_init();
						#if defined(TIMMER_MODE)
							hx3605_ppg_timer_cfg(true);
							#if defined(GSEN_40MS_TIMMER)
							hx3605_agc_timer_cfg(true);
							#endif
						#else
							hx3605_gpioint_cfg(true);
						#endif
            s_ppg_state = 1;

            AGC_LOG("ppg init mode\r\n");
            break;

        case PPG_OFF:
            hx3605_ppg_off();
            s_ppg_state = 0;
            AGC_LOG("ppg off mode\r\n");
            break;
				
        case PPG_LED_OFF:
            hx3605_hrv_low_power();
            s_ppg_state = 0;
            AGC_LOG("ppg led off mode\r\n");
            break;

        case CAL_INIT:
            Init_hrv_PPG_Calibration_Routine(&calReg, 64);
            hx3605_hrv_cal_init();
            hx3605_hrv_updata_reg();
            #if defined(TIMMER_MODE)
							#if defined(GSEN_40MS_TIMMER)
							#else
							hx3605_agc_timer_cfg(true);
							#endif
            #endif 
						//AGC_LOG("init  %x,%x \r\n", hx3605_read_reg(0x23),hx3605_read_reg(0x24));
            s_cal_state = 1;
            AGC_LOG("cal init mode\r\n");   
            break;   
				
        case RECAL_INIT:        
            Restart_hrv_PPG_Calibration_Routine(&calReg);
            hx3605_hrv_cal_init();
            hx3605_hrv_updata_reg();
            #if defined(TIMMER_MODE)
							#if defined(GSEN_40MS_TIMMER)
							#else
							hx3605_agc_timer_cfg(true);
							#endif
            #endif 
            s_cal_state = 1;
            AGC_LOG("Recal init mode\r\n");
            break;

        case CAL_OFF:
            #if defined(TIMMER_MODE)
							#if defined(GSEN_40MS_TIMMER)
							#else
							hx3605_agc_timer_cfg(false);
							#endif
            #endif
            hx3605_hrv_cal_off();
            s_cal_state = 0;
            AGC_LOG("cal off mode\r\n");
            break;

        default:
            break;
    }
}

SENSOR_ERROR_T hx3605_hrv_enable(void)
{
    if (!hx3605_chip_check()) 
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
    if(!tyhx_hrv_alg_open())
    {
        AGC_LOG("hrv alg open fail,or dynamic ram not enough!\r\n");
    }
    
    hrv_wear_status = MSG_HRV_INIT;
    hrv_wear_status_pre = MSG_HRV_INIT;
    
    hx3605_hrv_set_mode(PPG_INIT);

    AGC_LOG("hx3690l enable!\r\n");

    return SENSOR_OK;
}


void hx3605_hrv_disable(void)
{
    hx3605_ppg_timer_cfg(false);
    hx3605_agc_timer_cfg(false);

    hx3605_hrv_set_mode(PPG_OFF);
		s_ppg_state = 0;
		s_cal_state = 0;
    tyhx_hrv_alg_close();

    AGC_LOG("hx3690l disable!\r\n");
}

void hx3605_hrv_data_reset(void)
{
		s_ppg_state = 0;
		s_cal_state = 0;
		tyhx_hrv_alg_close();
}

hx3605_hrv_wear_msg_code_t hx3605_hrv_get_wear_status(void)
{
    return  hrv_wear_status;
}

hx3605_hrv_wear_msg_code_t hx3605_hrv_get_wear_status_pre(void)
{
    return  hrv_wear_status_pre;
}

HRV_CAL_SET_T get_hrv_agc_status(void)
{
    HRV_CAL_SET_T cal;
    cal.flag     = calReg.flag;
    cal.int_cnt  = calReg.int_cnt;
    cal.led_cur  = calReg.led_cur;     
    cal.led_idac = calReg.led_idac;  
    cal.amb_idac = calReg.amb_idac;  
    cal.ontime   = calReg.ontime;    
    cal.led_step = calReg.led_step;
    cal.state    = calReg.state;
    return cal;
}

uint8_t hx3605_hrv_read(hrv_sensor_data_t * s_dat)
{
    int32_t PPG_src_data;
    int32_t Ir_src_data;
    bool recal = false;
//    uint8_t size = 0;
//    uint8_t size_byte = 0;
    int32_t *PPG_buf =  &(s_dat->ppg_data[0]);
	  int32_t *ir_buf =  &(s_dat->ir_data[0]);  
    int32_t *s_buf =  &(s_dat->s_buf[0]);   
    s_dat->agc_green =  calReg.led_cur;
		int32_t ps_data[4]={0};
		int32_t ps_green_data = 0;
		uint8_t data_count = 0;
    if (!s_ppg_state || s_cal_state) 
    {
        return NULL;
    }    
		
		read_data_packet(ps_data);
		Ir_src_data = ps_data[2]-ps_data[3];
		ps_green_data = ps_data[0];
		hx3605_hrv_wear_mode_check(Ir_src_data);
		//DEBUG_PRINTF("%d %d %d %d\r\n" ,ps_data[0],ps_data[1],ps_data[2],ps_data[3]);
		
		data_count = hx3605_read_fifo_data(s_buf,1,1); 
		//DEBUG_PRINTF("ppg data size: %d %d\r\n", data_count,hrs_phase_num);
		s_dat->count =  data_count; 
		for (uint8_t i=0; i<data_count; i++) 
		{
				PPG_src_data = s_buf[i];           
				PPG_buf[i] = 2*PPG_src_data;
				ir_buf[i] = Ir_src_data;
		}
		
		if (ps_green_data<150000 || ps_green_data>400000)            
		{
				recal = true;
			 
				if(hrv_wear_status==MSG_HRV_NO_WEAR)
				{
						 recal = false;
				}                
		}
		if (recal) 
		{
				cal_delay--;

				if (cal_delay <= 0) 
				{
						cal_delay = CAL_DELAY_COUNT;
						hx3605_hrv_set_mode(RECAL_INIT);
				}
		}
		else                       
		{
				cal_delay = CAL_DELAY_COUNT;
		}   
    return 1;
}

#endif
